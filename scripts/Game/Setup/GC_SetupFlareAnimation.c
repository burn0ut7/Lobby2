//! Flares bypass OnPrepareControls through an input listener and animation events.
modded class SCR_FlareAnimationComponent
{
	//------------------------------------------------------------------------------------------------
	override protected bool CanContinueTheAnimation()
	{
		if (GC_LobbyGameMode.GetGameMode() && m_UserController)
		{
			PlayerManager players = GetGame().GetPlayerManager();
			int playerId = players.GetPlayerIdFromControlledEntity(m_UserController.GetCharacter());
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(players.GetPlayerController(playerId));
			if (controller && controller.GC_IsCombatRestricted())
				return false;
		}
		return super.CanContinueTheAnimation();
	}
}
