#ifdef WORKBENCH
modded class GC_GroupHeaderContextComponent
{
	bool GC_TestMatchesGroup(SCR_AIGroup group)
	{
		return m_Group == group;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Open through right-click handlers or activate the actual popup button by label.
	string GC_TestScopeLockUI(string target, int slotId, string action)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !slot.GetGroup() || !slot.GetGroup().GetFaction())
			return "UNAVAILABLE target slot/group/faction";
		array<Widget> widgets = {};
		GC_TestCollect(GetGame().GetWorkspace(), widgets);
		foreach (Widget widget : widgets)
		{
			if (!widget.IsVisibleInHierarchy())
				continue;
			if (action != "open")
			{
				GC_ContextMenuActionButtonComponent button = GC_ContextMenuActionButtonComponent.Cast(widget.FindHandler(GC_ContextMenuActionButtonComponent));
				TextWidget label = TextWidget.Cast(widget.FindAnyWidget("ContextActionLabel"));
				if (button && label && label.GetText() == action)
					return GC_TestDispatchClick(widget);
				continue;
			}
			GC_FactionButtonComponent faction = GC_FactionButtonComponent.Cast(widget.FindHandler(GC_FactionButtonComponent));
			GC_GroupHeaderContextComponent group = GC_GroupHeaderContextComponent.Cast(widget.FindHandler(GC_GroupHeaderContextComponent));
			GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
			if (target == "faction" && faction && faction.GetFactionKey() == slot.GetGroup().GetFaction().GetFactionKey())
				faction.OnClick(widget, 0, 0, 1);
			else if (target == "group" && group && group.GC_TestMatchesGroup(slot.GetGroup()))
				group.OnMouseButtonDown(widget, 0, 0, 1);
			else if (target == "slot" && slotButton && slotButton.GetSlot() == slot)
				slotButton.OnClick(widget, 0, 0, 1);
			else
				continue;
			return "DISPATCHED right-click " + target;
		}
		return "UNAVAILABLE visible target/action";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestScopeLockSnapshot()
	{
		string result;
		foreach (GC_Slot slot : GetSlots())
		{
			SCR_AIGroup group = slot.GetGroup();
			string faction;
			if (group && group.GetFaction())
				faction = group.GetFaction().GetFactionKey();
			result += string.Format("slot=%1 group=%2 faction=%3 eligible=%4 locked=%5 player=%6;\n",
				slot.GetID(), slot.GetGroupRplId(), faction, slot.CanAdminLock(), slot.IsAdminLocked(), slot.GetPlayerId());
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestScopeLockReject(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!Replication.IsServer() || !slot || !slot.GetGroup() || !slot.GetGroup().GetFaction())
			return "UNAVAILABLE authority/target";
		string before = GC_TestScopeLockSnapshot();
		string groupResult = ExecuteAdminSlotLocks(-1, GC_ESlotLockScope.GROUP, slot.GetGroupRplId(), string.Empty, !slot.IsAdminLocked());
		string factionResult = ExecuteAdminSlotLocks(-1, GC_ESlotLockScope.FACTION, RplId.Invalid(), slot.GetGroup().GetFaction().GetFactionKey(), !slot.IsAdminLocked());
		int requester = GetGame().GetPlayerController().GetPlayerId();
		string missingGroup = ExecuteAdminSlotLocks(requester, GC_ESlotLockScope.GROUP, RplId.Invalid(), string.Empty, true);
		string missingFaction = ExecuteAdminSlotLocks(requester, GC_ESlotLockScope.FACTION, RplId.Invalid(), string.Empty, true);
		return string.Format("group=%1 faction=%2 missingGroup=%3 missingFaction=%4 unchanged=%5",
			groupResult, factionResult, missingGroup, missingFaction, before == GC_TestScopeLockSnapshot());
	}
}
#endif
