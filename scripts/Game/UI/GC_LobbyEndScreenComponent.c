class GC_LobbyEndScreenComponentClass : SCR_GameOverScreenManagerComponentClass
{
}

//! Lobby2's native POSTGAME presentation owns the debrief; do not schedule a second end menu.
class GC_LobbyEndScreenComponent : SCR_GameOverScreenManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		if (GC_LobbyGameMode.GetGameMode() == GetOwner())
			return;
		super.OnGameModeEnd(data);
	}
}
