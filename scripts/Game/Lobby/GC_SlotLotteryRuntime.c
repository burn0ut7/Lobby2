//! Owns draw state, deadlines and cancellation. The manager retains slot policy,
//! authenticated entry points and transport; no native membership is stored here.
class GC_SlotLotteryRuntime
{
	protected GC_LobbyManager m_Manager;
	protected bool m_bStopped;
	protected ref array<ref GC_SlotLottery> m_aSlotLotteries = {};
	protected int m_iLotterySequence;
	ref ScriptInvoker<GC_SlotLottery> m_OnChanged = new ScriptInvoker<GC_SlotLottery>();
	ref ScriptInvoker<GC_SlotLottery, string> m_OnAnnouncement = new ScriptInvoker<GC_SlotLottery, string>();

	//------------------------------------------------------------------------------------------------
	void GC_SlotLotteryRuntime(GC_LobbyManager manager)
	{
		m_Manager = manager;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsActive()
	{
		return !m_bStopped && m_Manager && GC_LobbyManager.GetManager() == m_Manager;
	}

	//! A copied collection for diagnostics; callers do not own the registry.
	array<ref GC_SlotLottery> GetSnapshot()
	{
		array<ref GC_SlotLottery> snapshot = {};
		foreach (GC_SlotLottery lottery : m_aSlotLotteries)
			snapshot.Insert(lottery);
		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	GC_SlotLottery GetSlotLottery(int lotteryId = 0)
	{
		for (int i = m_aSlotLotteries.Count() - 1; i >= 0; i--)
		{
			ref GC_SlotLottery lottery = m_aSlotLotteries[i];
			if (lotteryId == lottery.m_iId || (lotteryId == 0 && lottery.GetSecondsRemaining() > 0))
				return lottery;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	GC_SlotLottery GetLotteryForSlot(int slotId)
	{
		foreach (GC_SlotLottery lottery : m_aSlotLotteries)
		{
			if (lottery.m_aSlotIds.Contains(slotId))
				return lottery;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	string StartSlotLottery(int requesterId, string factionKey, RplId groupId, int slotId)
	{
		if (!IsActive() || !Replication.IsServer())
			return "Lottery entry is unavailable.";
		string outcome;
		SCR_EGameModeState phase = m_Manager.GetLobbyPhase();
		array<int> slots = {};
		m_Manager.CollectLotterySlots(factionKey, groupId, slotId, slots);
		bool overlap;
		foreach (int selectedSlotId : slots)
		{
			if (GetLotteryForSlot(selectedSlotId))
				overlap = true;
		}
		if (overlap)
			outcome = "This selection overlaps a running lottery.";
		else if (m_aSlotLotteries.Count() >= 64)
			outcome = "The maximum number of simultaneous lotteries is running.";
		else if (phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
			outcome = "Lotteries are available during slotting and briefing.";
		else if (slots.IsEmpty())
			outcome = "There are no empty slots in this selection.";
		else if (slots.Count() > 512)
			outcome = "A lottery supports up to 512 slots. Select a smaller group.";
		else
		{
			GC_Slot firstSlot = m_Manager.GetSlot(slots[0]);
			SCR_AIGroup group = m_Manager.ResolveSlotGroup(firstSlot);
			string label = WidgetManager.Translate(group.GetFaction().GetFactionName());
			if (slotId >= 0)
			{
				string roleName = string.Format("Slot %1", slotId);
				SCR_UIInfo role = GC_Slot.GetPrefabRoleInfo(firstSlot.GetAuthoredPrefab());
				if (role && !role.GetName().IsEmpty())
					roleName = WidgetManager.Translate(role.GetName());
				label = string.Format("%1 - %2 - %3", label, GC_GroupCallsignHelper.GetDisplayName(group), roleName);
			}
			else if (groupId != RplId.Invalid())
			{
				label = string.Format("%1 - %2", label, GC_GroupCallsignHelper.GetDisplayName(group));
			}
			ref GC_SlotLottery lottery = new GC_SlotLottery();
			m_iLotterySequence++;
			lottery.m_iId = m_iLotterySequence;
			lottery.m_sLabel = label;
			lottery.m_aSlotIds.Copy(slots);
			lottery.m_aEntrants.Clear();
			ChimeraWorld world = GetGame().GetWorld();
			lottery.m_StartedAt = world.GetServerTimestamp();
			lottery.m_iRequesterId = requesterId;
			m_aSlotLotteries.Insert(lottery);
			m_OnChanged.Invoke(lottery);
			GetGame().GetCallqueue().CallLater(FinishSlotLottery, 30000, false, lottery.m_iId);
			outcome = string.Format("%1\n\nType /join to enter the latest lottery.\nOr right-click a lottery slot and choose Enter lottery.", label);
			m_OnAnnouncement.Invoke(lottery, outcome);
			outcome = "started";
		}
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	bool CanEnterSlotLottery(int playerId, int lotteryId)
	{
		ref GC_SlotLottery lottery = GetSlotLottery(lotteryId);
		if (!IsActive() || !lottery || lottery.m_aSlotIds.IsEmpty() || lottery.GetSecondsRemaining() <= 0)
			return false;
		foreach (int slotId : lottery.m_aSlotIds)
		{
			if (m_Manager.CanUseLotterySlot(playerId, slotId))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	string EnterSlotLottery(int playerId, int expectedId, bool enter)
	{
		if (!Replication.IsServer() || !IsActive()
			|| !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return "Lottery entry is unavailable.";
		ref GC_SlotLottery lottery = GetSlotLottery(expectedId);
		if (!lottery || lottery.m_aSlotIds.IsEmpty()
			|| lottery.GetSecondsRemaining() <= 0)
			return "This lottery has closed.";
		string identity = m_Manager.GetPlayerVoiceIdentity(playerId);
		if (identity.IsEmpty())
			return "Your player identity is not available yet.";
		if (!enter)
		{
			lottery.m_aEntrants.RemoveItem(identity);
			m_OnChanged.Invoke(lottery);
			return "You have left the lottery.";
		}
		if (lottery.m_aEntrants.Contains(identity))
			return "You are already entered in this lottery.";
		if (!CanEnterSlotLottery(playerId, lottery.m_iId))
			return "You are not eligible for the slots in this lottery.";
		if (lottery.m_aEntrants.Count() >= 256)
			return "This lottery has reached its entry limit.";
		lottery.m_aEntrants.Insert(identity);
		m_OnChanged.Invoke(lottery);
		return string.Format("Entered %1 lottery. Your role will be chosen randomly.", lottery.m_sLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected void FinishSlotLottery(int lotteryId)
	{
		ref GC_SlotLottery lottery = GetSlotLottery(lotteryId);
		if (!Replication.IsServer() || !IsActive() || !lottery || lottery.m_aSlotIds.IsEmpty())
			return;
		if (lottery.GetSecondsRemaining() > 0)
		{
			GetGame().GetCallqueue().CallLater(FinishSlotLottery, 1000, false, lotteryId);
			return;
		}
		array<int> slots = {};
		slots.Copy(lottery.m_aSlotIds);
		array<string> entrants = {};
		entrants.Copy(lottery.m_aEntrants);
		int assigned;
		// Random removal samples both lists without replacement. No entry carries a chosen role.
		while (!entrants.IsEmpty() && !slots.IsEmpty())
		{
			int entrantIndex = Math.RandomInt(0, entrants.Count());
			string identity = entrants[entrantIndex];
			entrants.Remove(entrantIndex);
			int playerId = m_Manager.GetConnectedPlayerIdByIdentity(identity);
			if (playerId <= 0)
				continue;
			array<int> eligible = {};
			foreach (int slotId : slots)
			{
				if (m_Manager.CanUseLotterySlot(playerId, slotId))
					eligible.Insert(slotId);
			}
			if (eligible.IsEmpty())
				continue;
			int selectedSlotId = eligible[Math.RandomInt(0, eligible.Count())];
			slots.RemoveItem(selectedSlotId);
			bool applied = m_Manager.TryAssignLotteryWinner(playerId, lottery.m_iId, selectedSlotId);
			GC_AdminActionLogger.Log(lottery.m_iRequesterId, "lottery-draw",
				string.Format("lottery=%1 player=%2 slot=%3", lottery.m_iId, playerId, selectedSlotId), applied.ToString());
			if (applied)
				assigned++;
		}
		string result = string.Format("%1 lottery finished: %2 player(s) assigned.", lottery.m_sLabel, assigned);
		ClearSlotLottery(lottery);
		m_OnAnnouncement.Invoke(lottery, result);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearSlotLottery(GC_SlotLottery lottery)
	{
		m_aSlotLotteries.RemoveItem(lottery);
		lottery.m_aSlotIds.Clear();
		lottery.m_aEntrants.Clear();
		m_OnChanged.Invoke(lottery);
	}

	//------------------------------------------------------------------------------------------------
	void CancelAllForPhase()
	{
		if (!IsActive() || !Replication.IsServer())
			return;
		GetGame().GetCallqueue().Remove(FinishSlotLottery);
		while (!m_aSlotLotteries.IsEmpty())
		{
			ref GC_SlotLottery lottery = m_aSlotLotteries[0];
			GC_AdminActionLogger.Log(lottery.m_iRequesterId, "lottery-cancel", lottery.m_sLabel, "phase-changed");
			ClearSlotLottery(lottery);
			m_OnAnnouncement.Invoke(lottery, string.Format("%1 lottery cancelled because the mission phase changed.", lottery.m_sLabel));
		}
	}

	//------------------------------------------------------------------------------------------------
	void CancelForSlots(array<int> slotIds)
	{
		if (!IsActive() || !Replication.IsServer())
			return;
		foreach (int slotId : slotIds)
		{
			ref GC_SlotLottery lottery = GetLotteryForSlot(slotId);
			if (!lottery)
				continue;
			ClearSlotLottery(lottery);
			m_OnAnnouncement.Invoke(lottery, string.Format("%1 lottery cancelled because its slots were purged.", lottery.m_sLabel));
		}
	}

	//------------------------------------------------------------------------------------------------
	bool Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_aSlotLotteries.Count());
		foreach (GC_SlotLottery lottery : m_aSlotLotteries)
		{
			if (!lottery.Save(writer))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Load(ScriptBitReader reader)
	{
		int count;
		if (!reader.ReadInt(count) || count < 0 || count > 64)
			return false;
		m_aSlotLotteries.Clear();
		for (int index; index < count; index++)
		{
			ref GC_SlotLottery lottery = new GC_SlotLottery();
			if (!lottery.Load(reader))
				return false;
			m_aSlotLotteries.Insert(lottery);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool ApplySnapshot(int id, WorldTimestamp startedAt, string label, array<int> slots, array<string> entrants)
	{
		ref GC_SlotLottery lottery = GetSlotLottery(id);
		if (slots.IsEmpty())
		{
			if (lottery)
				m_aSlotLotteries.RemoveItem(lottery);
		}
		else if (!lottery)
		{
			lottery = new GC_SlotLottery();
			m_aSlotLotteries.Insert(lottery);
		}
		if (!lottery)
			return false;
		lottery.m_iId = id;
		lottery.m_StartedAt = startedAt;
		lottery.m_sLabel = label;
		lottery.m_aSlotIds.Copy(slots);
		lottery.m_aEntrants.Copy(entrants);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bStopped = true;
		if (GetGame())
			GetGame().GetCallqueue().Remove(FinishSlotLottery);
		m_aSlotLotteries.Clear();
	}
}
