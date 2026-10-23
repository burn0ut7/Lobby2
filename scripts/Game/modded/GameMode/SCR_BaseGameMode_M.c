//! Adds a narrowly guarded transition entry point for Lobby2's two modded
//! pre-match states. It cannot mutate unrelated game modes or native GAME and
//! POSTGAME transitions.
modded class SCR_BaseGameMode
{
	bool GC_SetLobbyState(SCR_EGameModeState state)
	{
		if (GC_LobbyGameMode.GetGameMode() != this || !IsMaster())
			return false;

		if (state != SCR_EGameModeState.SLOTTING && state != SCR_EGameModeState.BRIEFING)
			return false;

		if (m_eGameState == state)
			return true;

		m_eGameState = state;
		Replication.BumpMe();
		OnGameStateChanged();
		return true;
	}
}
