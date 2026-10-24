class GC_AdminReserveSlotAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Reserve slot...";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		return context && manager && context.GetTargetType() == GC_EContextTargetType.SLOT
			&& SCR_Global.IsAdmin(context.GetRequesterPlayerId())
			&& !GC_LobbyPhase.IsAtOrAfter(manager.GetLobbyPhase(), SCR_EGameModeState.GAME);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context) || !context.GetSlot())
			return false;
		GC_Slot slot = context.GetSlot();
		return GC_LobbyManager.GetManager().GetSlot(slot.GetID()) == slot
			&& slot.GetUUID() == context.GetPlayerIdentity() && !slot.IsKIA();
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (CanExecute(context))
			GC_SlotReservationMenu.OpenForSlot(context.GetSlot());
	}
}

