class GC_AdminSlotToggleAction : GC_ContextMenuAction
{
	protected bool m_bPin;

	//------------------------------------------------------------------------------------------------
	void GC_AdminSlotToggleAction(bool pin)
	{
		m_bPin = pin;
	}

	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && GC_LobbyManager.GetManager() && context.GetTargetType() == GC_EContextTargetType.SLOT && SCR_Global.IsAdmin(context.GetRequesterPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context) || !context.GetSlot())
			return false;
		GC_Slot slot = context.GetSlot();
		if (GC_LobbyManager.GetManager().GetSlot(slot.GetID()) != slot || slot.GetUUID() != context.GetPlayerIdentity())
			return false;
		return !m_bPin || slot.HasClaim();
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		if (!context || !context.GetSlot())
			return string.Empty;
		GC_Slot slot = context.GetSlot();
		if (m_bPin)
		{
			if (slot.IsAssignmentPinned())
				return "Unpin assignment";
			return "Pin assignment";
		}
		if (slot.IsAdminLocked())
			return "Unlock slot";
		return "Lock slot";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		GC_Slot slot = context.GetSlot();
		GC_EAdminSlotAction action = GC_EAdminSlotAction.LOCK;
		if (slot.IsAdminLocked())
			action = GC_EAdminSlotAction.UNLOCK;
		if (m_bPin)
		{
			action = GC_EAdminSlotAction.PIN;
			if (slot.IsAssignmentPinned())
				action = GC_EAdminSlotAction.UNPIN;
		}
		controller.RequestAdminSlot(slot.GetID(), context.GetPlayerIdentity(), action);
	}
}
