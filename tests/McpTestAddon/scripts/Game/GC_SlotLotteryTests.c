//! Optional live tests. All mutations use production authorization and handlers.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestLotterySnapshot()
	{
		PlayerController local = GetGame().GetPlayerController();
		int playerId;
		if (local)
			playerId = local.GetPlayerId();
		string result = string.Format("phase=%1 local=%2 admin=%3 active=%4", GetLobbyPhase(), playerId,
			SCR_Global.IsAdmin(playerId), m_aSlotLotteries.Count());
		foreach (GC_SlotLottery lottery : m_aSlotLotteries)
		{
			result += string.Format("\nlottery=%1 label=%2 remaining=%3 slots=%4 entrants=%5",
				lottery.m_iId, lottery.m_sLabel, lottery.GetSecondsRemaining(), lottery.m_aSlotIds.Count(), lottery.m_aEntrants.Count());
			foreach (int slotId : lottery.m_aSlotIds)
				result += " slot=" + slotId.ToString();
		}
		foreach (GC_Slot slot : GetSlots())
		{
			if (slot.GetID() < 12 || slot.HasClaim())
				result += string.Format("\nslot=%1 player=%2 state=%3 adminLocked=%4", slot.GetID(), slot.GetPlayerId(), slot.GetState(), slot.IsAdminLocked());
		}
		return result;
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	string GC_TestLotteryContext(string scope, int slotId, bool execute, bool start)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !slot.GetGroup() || !slot.GetGroup().GetFaction())
			return "UNAVAILABLE slot/group";
		ref GC_ContextMenuContext context;
		if (scope == "slot")
			context = GC_ContextMenuContext.ForSlot(slot);
		else if (scope == "group")
			context = GC_ContextMenuContext.ForGroup(slot.GetGroup());
		else if (scope == "faction")
			context = GC_ContextMenuContext.ForFaction(slot.GetGroup().GetFaction().GetFactionKey());
		else
			return "UNAVAILABLE scope";
		GC_ContextMenuComponent.Open(context, GC_TestMenuRoot());
		ref GC_SlotLotteryAction action = new GC_SlotLotteryAction(start);
		bool available = action.CanExecute(context);
		if (execute && available)
			action.Execute(context);
		return string.Format("available=%1 dispatched=%2", available, execute && available);
	}
#endif

	//------------------------------------------------------------------------------------------------
	string GC_TestLotteryEntry(int lotteryId, bool enter)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		controller.RequestEnterSlotLottery(lotteryId, enter);
		return "DISPATCHED production entry request";
	}

	//------------------------------------------------------------------------------------------------
	//! Five independent starts; read snapshots and wait for the real 30-second deadlines.
	string GC_TestLotteryFive()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_aSlotLotteries.IsEmpty())
			return "UNAVAILABLE controller or existing lotteries";
		int count;
		foreach (GC_Slot slot : GetSlots())
		{
			if (slot.HasClaim() || slot.GetState() != GC_SlotState.FREE)
				continue;
			controller.RequestStartSlotLottery("", RplId.Invalid(), slot.GetID());
			count++;
			if (count == 5)
				break;
		}
		return string.Format("DISPATCHED starts=%1", count);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLotteryGuards(int requesterId)
	{
		if (!Replication.IsServer() || SCR_Global.IsAdmin(requesterId) || GetSlots().IsEmpty())
			return "UNAVAILABLE authority/non-admin/slots";
		int before = m_aSlotLotteries.Count();
		int sequence = m_iLotterySequence;
		string result = StartSlotLottery(requesterId, "", RplId.Invalid(), GetSlots()[0].GetID());
		if (before != m_aSlotLotteries.Count() || sequence != m_iLotterySequence || result != "You must be logged in as an admin.")
			return "FAIL authorization changed lottery state";
		return "PASS unauthorized start rejected without mutation";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLotteryPoolGuards(int lotteryId)
	{
		GC_SlotLottery lottery = GetSlotLottery(lotteryId);
		PlayerController controller = GetGame().GetPlayerController();
		if (!lottery || !controller || !Replication.IsServer() || lottery.GetSecondsRemaining() < 5)
			return "UNAVAILABLE active lottery/local authority";
		int playerId = controller.GetPlayerId();
		int count = m_aSlotLotteries.Count();
		int slotId = lottery.m_aSlotIds[0];
		string overlap = StartSlotLottery(playerId, "", RplId.Invalid(), slotId);
		if (overlap != "This selection overlaps a running lottery." || count != m_aSlotLotteries.Count())
			return "FAIL overlapping start";
		if (TryAssignPlayerToSlot(playerId, slotId))
			return "FAIL direct claim during lottery";
		string entry = EnterSlotLottery(playerId, lotteryId, true);
		int entries = lottery.m_aEntrants.Count();
		string duplicate = EnterSlotLottery(playerId, lotteryId, true);
		if (entries != 1 || lottery.m_aEntrants.Count() != entries || duplicate != "You are already entered in this lottery.")
			return "FAIL duplicate entry: " + entry;
		string stale = EnterSlotLottery(playerId, lotteryId + 1000, true);
		if (stale != "This lottery has closed." || lottery.m_aEntrants.Count() != entries)
			return "FAIL stale lottery request";
		return "PASS overlap/direct-claim/duplicate/stale guards";
	}
}
