#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref array<ref GC_Slot> m_aGCSlotAccessBefore = {};

	//------------------------------------------------------------------------------------------------
	//! Call on each inspected peer before a production request. Never changes live state.
	string GC_TestSlotAccessRemember()
	{
		m_aGCSlotAccessBefore.Clear();
		foreach (GC_Slot slot : GetSlots())
		{
			GC_Slot snapshot = new GC_Slot();
			snapshot.CopyFrom(slot);
			snapshot.SetID(slot.GetID());
			m_aGCSlotAccessBefore.Insert(snapshot);
		}
		return string.Format("REMEMBERED slots=%1 newSlotsLocked=%2", m_aGCSlotAccessBefore.Count(), ShouldNewSlotsStartLocked());
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestSlotAccessClaim(GC_Slot slot)
	{
		return string.Format("%1|%2|%3|%4|%5|%6", slot.GetPlayerId(), slot.GetUUID(),
			slot.GetCachedPlayerName(), slot.GetState(), slot.IsAwaitingInitialJoin(), slot.GetEntityRplId());
	}

	//------------------------------------------------------------------------------------------------
	//! kind: locks, clear, unchanged. Run after correlated result and peer replication.
	string GC_TestSlotAccessCheck(string kind, bool locked)
	{
		if (kind != "locks" && kind != "clear" && kind != "unchanged")
			return "UNAVAILABLE check kind";
		if (m_aGCSlotAccessBefore.IsEmpty() || m_aGCSlotAccessBefore.Count() != GetSlots().Count())
			return "FAIL missing baseline or changed roster";
		int cleared;
		foreach (GC_Slot before : m_aGCSlotAccessBefore)
		{
			GC_Slot slot = GetSlot(before.GetID());
			if (!slot || slot.GetGroupRplId() != before.GetGroupRplId()
				|| slot.GetGroupSlotIndex() != before.GetGroupSlotIndex()
				|| slot.GetAuthoredPrefab() != before.GetAuthoredPrefab()
				|| slot.GetResolvedPrefab() != before.GetResolvedPrefab())
				return "FAIL slot identity changed";
			bool expectedLock = before.IsAdminLocked();
			if (kind == "locks")
				expectedLock = locked;
			if (slot.IsAdminLocked() != expectedLock)
				return "FAIL lock slot=" + slot.GetID().ToString();
			if (kind == "clear" && before.IsAwaitingInitialJoin() && before.GetPlayerId() <= 0 && !before.IsKIA())
			{
				if (slot.HasClaim() || slot.IsAwaitingInitialJoin() || slot.GetEntity() || slot.GetState() != GC_SlotState.FREE)
					return "FAIL reservation not released slot=" + slot.GetID().ToString();
				cleared++;
			}
			else if (GC_TestSlotAccessClaim(slot) != GC_TestSlotAccessClaim(before))
				return "FAIL claim changed slot=" + slot.GetID().ToString();
		}
		return string.Format("PASS %1 slots=%2 cleared=%3", kind, GetSlots().Count(), cleared);
	}

	//------------------------------------------------------------------------------------------------
	//! Use a real connected logged-out player or unresolved ID; never grants test roles.
	string GC_TestSlotAccessReject(int requesterId)
	{
		if (!Replication.IsServer() || SCR_Global.IsAdmin(requesterId))
			return "UNAVAILABLE authority and non-admin requester required";
		GC_TestSlotAccessRemember();
		int oldPolicy = m_iNewSlotLockOverride;
		string lockResult = ExecuteAdminGroupLock(requesterId, RplId.Invalid(), false);
		string output;
		string clearResult = ExecuteSlotReservation(requesterId, "clear-all", -1, UUID.NULL_UUID, "", "", "", output);
		if (lockResult != "rejected-not-logged-in"
			|| clearResult != "Sign in as an administrator to manage reservations."
			|| oldPolicy != m_iNewSlotLockOverride)
			return "FAIL authorization result/policy";
		return GC_TestSlotAccessCheck("unchanged", false);
	}
}
#endif
