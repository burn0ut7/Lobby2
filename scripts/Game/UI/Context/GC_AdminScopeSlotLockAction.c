//! Explicit intents let admins normalize a group or faction with mixed locks.
class GC_AdminScopeSlotLockAction : GC_ContextMenuAction
{
	protected bool m_bLocked;

	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(context.GetRequesterPlayerId())
			&& (context.GetTargetType() == GC_EContextTargetType.GROUP
				|| context.GetTargetType() == GC_EContextTargetType.FACTION);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		foreach (GC_Slot slot : manager.GetSlots())
		{
			if (!slot.CanAdminLock() || slot.IsAdminLocked() == m_bLocked)
				continue;
			SCR_AIGroup group = slot.GetGroup();
			if (!group)
				continue;
			if (context.GetTargetType() == GC_EContextTargetType.GROUP && group == context.GetGroup())
				return true;
			if (context.GetTargetType() == GC_EContextTargetType.FACTION && group.GetFaction()
				&& group.GetFaction().GetFactionKey() == context.GetFactionKey())
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		string verb = "Unlock";
		if (m_bLocked)
			verb = "Lock";
		if (context.GetTargetType() == GC_EContextTargetType.FACTION)
			return verb + " faction slots";
		return verb + " group slots";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		if (context.GetTargetType() == GC_EContextTargetType.FACTION)
			controller.RequestAdminScopedSlotLock(GC_ESlotLockScope.FACTION, RplId.Invalid(), context.GetFactionKey(), m_bLocked);
		else
			controller.RequestAdminScopedSlotLock(GC_ESlotLockScope.GROUP, Replication.FindItemId(context.GetGroup()), string.Empty, m_bLocked);
	}

	//------------------------------------------------------------------------------------------------
	void GC_AdminScopeSlotLockAction(bool locked)
	{
		m_bLocked = locked;
	}
}
