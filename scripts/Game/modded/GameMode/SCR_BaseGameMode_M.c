//! Tracks last-mission recovery eligibility and exposes Lobby2-only pre-match transitions.
//! The shared initialization hook only writes Lobby2's eligibility file; unrelated
//! gameplay and native GAME/POSTGAME transitions retain their inherited behavior.
modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (SCR_Global.IsEditMode() || !Replication.IsServer())
			return;

		GC_MapPlans.NoteMission();
	}

	//------------------------------------------------------------------------------------------------
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
