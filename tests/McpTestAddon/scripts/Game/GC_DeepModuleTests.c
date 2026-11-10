//! Explicit integration checks through production authorization and mutation paths.
modded class GC_LobbyManager
{
	protected int m_iGC_ModuleLotteryId;
	protected int m_iGC_ModuleLotterySlot = -1;

	//------------------------------------------------------------------------------------------------
	string GC_TestModuleReservations()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId())
			|| GC_LobbyPhase.IsAtOrAfter(GetLobbyPhase(), SCR_EGameModeState.GAME))
			return "UNAVAILABLE authenticated local authority before GAME";
		int requester = controller.GetPlayerId();
		string output;
		string result = ExecuteSlotReservation(requester, "export", -1, "", "", "", "", output);
		JsonLoadContext reader = new JsonLoadContext(false);
		array<ref map<string, string>> rows;
		string world;
		if (!reader.LoadFromString(output) || !reader.ReadValue("world", world) || !reader.ReadValue("slots", rows))
			return "FAIL export: " + result;
		array<ref map<string, string>> selected = {};
		array<GC_Slot> slots = {};
		foreach (map<string, string> row : rows)
		{
			GC_Slot slot = GetSlot(row.Get("slotNumber").ToInt());
			if (!slot || slot.HasClaim() || slot.IsKIA())
				continue;
			selected.Insert(row);
			slots.Insert(slot);
			if (selected.Count() == 2)
				break;
		}
		if (selected.Count() != 2)
			return "UNAVAILABLE two free authored slots";
		string firstIdentity = "11111111-1111-4111-8111-111111111111";
		string secondIdentity = "22222222-2222-4222-8222-222222222222";
		if (GetSlotByUUID(firstIdentity) || GetSlotByUUID(secondIdentity))
			return "UNAVAILABLE test identities already claimed";
		selected[0].Set("armaUid", firstIdentity);
		selected[0].Set("playerName", "Module test one");
		selected[1].Set("armaUid", "invalid-last-row");
		selected[1].Set("playerName", "Module test two");
		string input = GC_ModuleReservationJSON(world, selected);
		result = ExecuteSlotReservation(requester, "import", -1, "", "", "", input, output);
		if (slots[0].HasClaim() || slots[1].HasClaim() || !result.Contains("No reservations were changed"))
			return "FAIL invalid final row mutated batch: " + result;
		selected[1].Set("armaUid", firstIdentity);
		result = ExecuteSlotReservation(requester, "import", -1, "", "", "", GC_ModuleReservationJSON(world, selected), output);
		if (slots[0].HasClaim() || slots[1].HasClaim() || !result.Contains("same Arma UID"))
			return "FAIL duplicate UID mutated batch: " + result;
		selected[1].Set("armaUid", secondIdentity);
		input = GC_ModuleReservationJSON(world, selected);
		result = ExecuteSlotReservation(-1, "import", -1, "", "", "", input, output);
		if (slots[0].HasClaim() || slots[1].HasClaim() || !result.Contains("Sign in"))
			return "FAIL unauthorized import: " + result;
		result = ExecuteSlotReservation(requester, "import", -1, "", "", "", input, output);
		if (slots[0].GetUUID() != firstIdentity || slots[1].GetUUID() != secondIdentity)
			return "FAIL accepted batch: " + result;
		result = ExecuteSlotReservation(requester, "import", -1, "", "", "", input, output);
		if (!result.Contains("Reserved 0 slots"))
			return "FAIL idempotency: " + result;
		selected[0].Set("armaUid", "");
		selected[1].Set("armaUid", "");
		result = ExecuteSlotReservation(requester, "import", -1, "", "", "", GC_ModuleReservationJSON(world, selected), output);
		if (slots[0].GetUUID() != firstIdentity || slots[1].GetUUID() != secondIdentity)
			return "FAIL blank UID removed a claim";
		ExecuteSlotReservation(requester, "export", -1, "", "", "", "", output);
		if (!output.Contains(firstIdentity) || !output.Contains(secondIdentity)
			|| !output.Contains(selected[0].Get("slotKey")) || !output.Contains(selected[1].Get("slotKey")))
			return "FAIL export roundtrip";
		foreach (GC_Slot reserved : slots)
			ExecuteSlotReservation(requester, "clear", reserved.GetID(), reserved.GetUUID(), "", "", "", output);
		if (slots[0].HasClaim() || slots[1].HasClaim())
			return "FAIL cleanup";
		return string.Format("PASS export/import roundtrip rows=%1; invalid-last-row atomicity; duplicate UID; unauthorized rejection; idempotency; blank no-op; cleanup", rows.Count());
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_ModuleReservationJSON(string world, array<ref map<string, string>> rows)
	{
		JsonSaveContext writer = new JsonSaveContext();
		writer.WriteValue("version", 1);
		writer.WriteValue("world", world);
		writer.WriteValue("slots", rows);
		return writer.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	//! Start through production policy, then call check after the real 30-second deadline.
	string GC_TestModuleLottery(string action)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE authenticated local authority";
		int playerId = controller.GetPlayerId();
		if (action == "start")
		{
			m_iGC_ModuleLotterySlot = -1;
			if (GetLobbyPhase() == SCR_EGameModeState.PREGAME && !TryAdvanceLobbyPhase(playerId, SCR_EGameModeState.SLOTTING))
				return "FAIL slotting transition";
			foreach (GC_Slot candidate : GetSlots())
			{
				if (CanUseLotterySlot(playerId, candidate.GetID()) && !GetLotteryForSlot(candidate.GetID()))
				{
					m_iGC_ModuleLotterySlot = candidate.GetID();
					break;
				}
			}
			if (m_iGC_ModuleLotterySlot < 0)
				return "UNAVAILABLE eligible free slot";
			string result = StartSlotLottery(playerId, "", RplId.Invalid(), m_iGC_ModuleLotterySlot);
			GC_SlotLottery lottery = GetLotteryForSlot(m_iGC_ModuleLotterySlot);
			if (!lottery)
				return "FAIL start: " + result;
			m_iGC_ModuleLotteryId = lottery.m_iId;
			result = GC_LobbyManager.Cast(this).GC_TestLotteryPoolGuards(m_iGC_ModuleLotteryId);
			return string.Format("%1; waiting lottery=%2 slot=%3", result, m_iGC_ModuleLotteryId, m_iGC_ModuleLotterySlot);
		}
		GC_Slot slot = GetSlot(m_iGC_ModuleLotterySlot);
		if (action == "check")
		{
			if (GetSlotLottery(m_iGC_ModuleLotteryId))
				return "WAIT lottery still present";
			if (!slot || slot.GetPlayerId() != playerId || GetPlayerGroup(playerId) != ResolveSlotGroup(slot))
				return "FAIL delayed assignment/native membership";
			return "PASS delayed draw assigned winner through native group membership";
		}
		if (action == "purge")
		{
			if (!slot || !GetSlotLottery(m_iGC_ModuleLotteryId))
				return "UNAVAILABLE active draw";
			string purged = ExecuteAdminSlotLocks(playerId, GC_ESlotLockScope.GROUP, slot.GetGroupRplId(), "", true, true);
			if (GetSlotLottery(m_iGC_ModuleLotteryId) || slot.HasClaim() || !slot.IsAdminLocked())
				return "FAIL purge cancellation: " + purged;
			return "PASS purge cancelled draw and locked unclaimed slot";
		}
		if (action == "phase")
		{
			if (!GetSlotLottery(m_iGC_ModuleLotteryId))
				return "UNAVAILABLE active draw";
			if (!TryAdvanceLobbyPhase(playerId, GC_LobbyPhase.GetNext(GetLobbyPhase())))
				return "FAIL phase transition";
			if (!GetLotteryRuntime().GetSnapshot().IsEmpty())
				return "FAIL phase cancellation";
			return "PASS phase transition cancelled all draws";
		}
		return "UNAVAILABLE action";
	}
}
