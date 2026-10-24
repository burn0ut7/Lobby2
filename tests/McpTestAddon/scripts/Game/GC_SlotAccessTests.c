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
			if (kind == "locks" && before.CanAdminLock())
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
	//------------------------------------------------------------------------------------------------
	//! Real requester authorization; does not grant a role or bypass the production handler.
	string GC_TestRatioReject(int requesterId)
	{
		if (!Replication.IsServer() || SCR_Global.IsAdmin(requesterId))
			return "UNAVAILABLE authority and logged-out requester required";
		int revision = GetFactionRulesRevision();
		int lockMode = GetFactionLockOverride();
		bool balance = IsFactionBalanceEnforced();
		GC_TestSlotAccessRemember();
		string outcome = ExecuteAdminFactionRules(requesterId, revision, lockMode, !balance);
		if (outcome != "rejected-not-logged-in" || revision != GetFactionRulesRevision()
			|| lockMode != GetFactionLockOverride() || balance != IsFactionBalanceEnforced())
			return "FAIL ratio rejection mutated rules";
		return GC_TestSlotAccessCheck("unchanged", false);
	}

	//------------------------------------------------------------------------------------------------
	//! Inspect actual widgets; use GC_TestClick for the production button handler.
	string GC_TestRatioUI()
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE lobby menu";
		Widget ratio = root.FindAnyWidget("FactionRatioButton");
		Widget locks = root.FindAnyWidget("AllSlotLocksButton");
		TextWidget status = TextWidget.Cast(root.FindAnyWidget("AllSlotLocksStatus"));
		if (!ratio || !locks || !status)
			return "UNAVAILABLE lobby controls";
		return string.Format("ratioVisible=%1 ratioEnabled=%2 lockVisible=%3 lockEnabled=%4 ratioOn=%5 revision=%6 status=%7",
			GC_TestVisible(ratio), ratio.IsEnabled(), GC_TestVisible(locks), locks.IsEnabled(),
			IsFactionBalanceEnforced(), GetFactionRulesRevision(), status.GetText());
	}


	//------------------------------------------------------------------------------------------------
	//! Read the rendered icon and its sibling layers without changing UI state.
	string GC_TestSlotIconLayers(string widgetName)
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget icon = root.FindAnyWidget(widgetName);
		if (!icon)
			return "UNAVAILABLE icon";
		string result;
		Widget widget = icon.GetParent().GetChildren();
		while (widget)
		{
			result += string.Format("name=%1 type=%2 z=%3 opacity=%4 color=%5 visible=%6\n",
				widget.GetName(), widget.Type(), widget.GetZOrder(), widget.GetOpacity(), widget.GetColor().PackToInt(), widget.IsVisible());
			widget = widget.GetSibling();
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Temporary registered lifecycle fixtures; production admin handlers retain authorization.
	//! Restores all slot records and the new-slot policy before returning.
	string GC_TestEmptySlotLocks()
	{
		int requester = GetGame().GetPlayerController().GetPlayerId();
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(requester))
			return "UNAVAILABLE authenticated host required";
		array<GC_Slot> fixtures = {};
		foreach (GC_Slot candidate : GetSlots())
		{
			if (candidate.CanAdminLock() && fixtures.Count() < 3)
				fixtures.Insert(candidate);
		}
		if (fixtures.Count() != 3)
			return "UNAVAILABLE three empty slots required";
		ref array<ref GC_Slot> original = {};
		foreach (GC_Slot slot : GetSlots())
		{
			GC_Slot saved = new GC_Slot();
			saved.CopyFrom(slot);
			original.Insert(saved);
		}
		int policy = m_iNewSlotLockOverride;
		fixtures[0].ReserveIdentity("00000000-0000-0000-0000-000000000001", "Reserved test");
		fixtures[1].MarkDisconnected();
		fixtures[2].MarkKIA();
		string failures;
		foreach (GC_Slot target : GetSlots())
		{
			if (target.CanAdminLock())
				continue;
			if (target.IsAdminLocked())
				failures += "transition retained lock; ";
			string outcome = ExecuteAdminSlotAction(requester, target.GetID(), target.GetUUID(), GC_EAdminSlotAction.LOCK);
			if (outcome != "rejected-slot-not-empty" || target.IsAdminLocked())
				failures += "nonempty lock accepted; ";
		}
		GC_TestSlotAccessRemember();
		string lockResult = ExecuteAdminGroupLock(requester, RplId.Invalid(), true);
		string lockCheck = GC_TestSlotAccessCheck("locks", true);
		GC_TestSlotAccessRemember();
		string unlockResult = ExecuteAdminGroupLock(requester, RplId.Invalid(), false);
		string unlockCheck = GC_TestSlotAccessCheck("locks", false);
		string rejectCheck = GC_TestSlotAccessReject(-999);
		foreach (GC_Slot before : original)
			GetSlot(before.GetID()).CopyFrom(before);
		m_iNewSlotLockOverride = policy;
		if (!failures.IsEmpty() || lockResult != "applied" || unlockResult != "applied"
			|| !lockCheck.StartsWith("PASS") || !unlockCheck.StartsWith("PASS") || !rejectCheck.StartsWith("PASS"))
			return string.Format("FAIL %1 lock=%2/%3 unlock=%4/%5 reject=%6", failures, lockResult, lockCheck, unlockResult, unlockCheck, rejectCheck);
		return "PASS empty-only bulk lock/unlock; nonempty single locks rejected; transitions clear locks; nonadmin unchanged; originals restored";
	}

}
#endif
