//! Slot-targeted actions retain the captured claim for server stale-target checks.
class GC_AdminSlotPlayerAction : GC_ContextMenuAction
{
	protected GC_EAdminSlotAction m_eAction;

	//------------------------------------------------------------------------------------------------
	void GC_AdminSlotPlayerAction(GC_EAdminSlotAction action)
	{
		m_eAction = action;
	}

	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		if (!context || !GC_LobbyManager.GetManager()
			|| !SCR_Global.IsAdmin(context.GetRequesterPlayerId()))
			return false;
		if (m_eAction == GC_EAdminSlotAction.HEAL_OR_RESPAWN)
			return context.GetSlot() != null;
		return context.GetTargetType() == GC_EContextTargetType.SLOT;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_Slot slot = context.GetSlot();
		if (!slot || manager.GetSlot(slot.GetID()) != slot || slot.GetUUID() != context.GetPlayerIdentity())
			return false;
		if (m_eAction == GC_EAdminSlotAction.FREE)
			return slot.HasClaim();
		return m_eAction == GC_EAdminSlotAction.HEAL_OR_RESPAWN
			&& manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME
			&& (slot.IsKIA() || slot.GetEntityRplId() != RplId.Invalid());
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		if (m_eAction == GC_EAdminSlotAction.FREE)
			return "Remove player";
		if (context.GetSlot() && context.GetSlot().IsKIA())
			return "Respawn slot";
		return "Heal";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.RequestAdminSlot(context.GetSlot().GetID(), context.GetPlayerIdentity(), m_eAction);
	}
}
