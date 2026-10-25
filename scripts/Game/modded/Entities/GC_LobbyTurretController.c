modded class SCR_TurretControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && GC_LobbyGameMode.GetGameMode() && GetCompartmentSlot())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetCompartmentSlot().GetOccupant());
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
				controller.GC_FilterRestrictedActions(am);
		}

		super.OnPrepareControls(owner, am, dt, player);
	}
}
