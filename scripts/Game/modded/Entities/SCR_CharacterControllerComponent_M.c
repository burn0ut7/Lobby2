//! Shared engine hook: no persistent state and no change outside an active
//! Lobby2 freeze. Preserve the inherited control pipeline in every mode.
modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && GC_LobbyGameMode.GetGameMode())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
				controller.GC_FilterRestrictedActions(am);
		}
		super.OnPrepareControls(owner, am, dt, player);
	}

	//------------------------------------------------------------------------------------------------
	override bool SCR_GetDisableMovementControls()
	{
		if (GC_LobbyGameMode.GetGameMode())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetOwner());
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller && (controller.GC_IsSetupFrozen() || controller.GC_AreAdminControlsFrozen()))
				return true;
		}
		return super.SCR_GetDisableMovementControls();
	}

	//------------------------------------------------------------------------------------------------
	override bool SCR_GetDisableViewControls()
	{
		if (GC_LobbyGameMode.GetGameMode())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetOwner());
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller && controller.GC_AreAdminControlsFrozen())
				return true;
		}
		return super.SCR_GetDisableViewControls();
	}
}
