#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Exercises authority policy in a disposable pre-match test session.
	//! Stops on failure without replay or cleanup mutations.
	string GC_TestLockedFactionUnslot(int sameSideSlotId, int otherSideSlotId)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller)
			return "UNAVAILABLE authority/controller";
		if (GetLobbyPhase() != SCR_EGameModeState.SLOTTING && GetLobbyPhase() != SCR_EGameModeState.BRIEFING)
			return "UNAVAILABLE requires pre-match slotting or briefing";
		if (IsFactionBalanceEnforced())
			return "UNAVAILABLE disable faction balance to isolate faction locking";

		int playerId = controller.GetPlayerId();
		GC_Slot originalSlot = GetSlotByPlayerId(playerId);
		GC_Slot sameSideSlot = GetSlot(sameSideSlotId);
		GC_Slot otherSideSlot = GetSlot(otherSideSlotId);
		Faction faction = GetPlayerFaction(playerId);
		if (!originalSlot || !sameSideSlot || !otherSideSlot || !faction || !IsFactionSwitchLocked())
			return "UNAVAILABLE requires occupied slot, faction lock and two target slots";
		if (!sameSideSlot.GetGroup() || sameSideSlot.GetGroup().GetFaction() != faction
			|| !otherSideSlot.GetGroup() || otherSideSlot.GetGroup().GetFaction() == faction)
			return "UNAVAILABLE target factions";
		if (originalSlot == sameSideSlot || !CanPlayerRequestSlot(playerId, originalSlot)
			|| !CanPlayerRequestSlot(playerId, sameSideSlot) || CanPlayerRequestSlot(playerId, otherSideSlot))
			return "UNAVAILABLE requires releasable claim, available same-side slot and locked other side";
		if (otherSideSlot.HasClaim() || otherSideSlot.GetState() != GC_SlotState.FREE || otherSideSlot.IsAdminLocked())
			return "UNAVAILABLE other-side target must otherwise be available";

		if (!TryAssignPlayerToSlot(playerId, originalSlot.GetID()))
			return "FAIL own-slot request rejected";
		if (GetSlotByPlayerId(playerId) || originalSlot.HasClaim() || GetPlayerFaction(playerId) != faction)
			return "FAIL unslot did not release claim and preserve native faction";
		if (TryAssignPlayerToSlot(playerId, otherSideSlotId) || GetSlotByPlayerId(playerId)
			|| GetPlayerFaction(playerId) != faction || otherSideSlot.HasClaim())
			return "FAIL cross-faction request changed state after unslot";
		if (!TryAssignPlayerToSlot(playerId, sameSideSlotId) || GetSlotByPlayerId(playerId) != sameSideSlot
			|| GetPlayerFaction(playerId) != faction)
			return "FAIL same-side selection after unslot";
		if (!TryAssignPlayerToSlot(playerId, originalSlot.GetID()) || GetSlotByPlayerId(playerId) != originalSlot
			|| sameSideSlot.HasClaim() || GetPlayerFaction(playerId) != faction)
			return "FAIL direct same-side switch";

		return "PASS unslot preserves faction, cross-side rejected, same-side reselection and direct switch";
	}
}
#endif
