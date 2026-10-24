//! A player-row permission change; the player keeps their current slot and faction.
class GC_AdminRemoveFactionLockAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Remove faction lock";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !context
			|| !SCR_Global.IsAdmin(context.GetRequesterPlayerId())
			|| manager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return false;

		if (context.GetTargetType() == GC_EContextTargetType.SLOT)
		{
			GC_Slot slot = context.GetSlot();
			if (!slot || manager.GetSlot(slot.GetID()) != slot
				|| slot.GetPlayerId() != context.GetPlayerId() || slot.GetUUID() != context.GetPlayerIdentity())
				return false;
		}
		else if (context.GetTargetType() != GC_EContextTargetType.PLAYER)
			return false;

		return !context.GetPlayerIdentity().IsEmpty()
			&& manager.GetPlayerVoiceIdentity(context.GetPlayerId()) == context.GetPlayerIdentity()
			&& GetGame().GetPlayerManager().IsPlayerConnected(context.GetPlayerId())
			&& manager.IsPlayerFactionLocked(context.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		return IsVisible(context);
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.RequestAdminRemoveFactionLock(context.GetPlayerId(), context.GetPlayerIdentity());
	}
}
