//! Lobby2 extends the base game-mode phase domain so every base-game consumer
//! observes the same automatically replicated state. Explicit large values
//! avoid collisions with base-game or other add-on enum members.
modded enum SCR_EGameModeState
{
	SLOTTING = 1787356801,
	BRIEFING = 1787356802
}

//! Encodes Lobby2's legal ordering without storing a second phase value.
class GC_LobbyPhase
{
	//------------------------------------------------------------------------------------------------
	//! Re-query the active mode so shared marker hooks also work across mode changes.
	static bool ArePlayerMapMarkersLocked()
	{
		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		return gameMode && IsAtOrAfter(gameMode.GetState(), SCR_EGameModeState.GAME);
	}

	static SCR_EGameModeState GetNext(SCR_EGameModeState phase)
	{
		switch (phase)
		{
			case SCR_EGameModeState.PREGAME: return SCR_EGameModeState.SLOTTING;
			case SCR_EGameModeState.SLOTTING: return SCR_EGameModeState.BRIEFING;
			case SCR_EGameModeState.BRIEFING: return SCR_EGameModeState.GAME;
			case SCR_EGameModeState.GAME: return SCR_EGameModeState.POSTGAME;
		}

		return -1;
	}

	static bool IsValid(SCR_EGameModeState phase)
	{
		return phase == SCR_EGameModeState.PREGAME
			|| phase == SCR_EGameModeState.SLOTTING
			|| phase == SCR_EGameModeState.BRIEFING
			|| phase == SCR_EGameModeState.GAME
			|| phase == SCR_EGameModeState.POSTGAME;
	}

	static bool IsAtOrAfter(SCR_EGameModeState phase, SCR_EGameModeState threshold)
	{
		if (!IsValid(phase) || !IsValid(threshold))
			return false;

		return GetOrdinal(phase) >= GetOrdinal(threshold);
	}

	static string GetName(SCR_EGameModeState phase)
	{
		switch (phase)
		{
			case SCR_EGameModeState.PREGAME: return "PREGAME";
			case SCR_EGameModeState.SLOTTING: return "SLOTTING";
			case SCR_EGameModeState.BRIEFING: return "BRIEFING";
			case SCR_EGameModeState.GAME: return "GAME";
			case SCR_EGameModeState.POSTGAME: return "POSTGAME";
		}

		return "UNKNOWN";
	}

	//! Explicit ordering is required because modded enum storage values are
	//! intentionally non-contiguous and must never define lifecycle order.
	protected static int GetOrdinal(SCR_EGameModeState phase)
	{
		switch (phase)
		{
			case SCR_EGameModeState.PREGAME: return 0;
			case SCR_EGameModeState.SLOTTING: return 1;
			case SCR_EGameModeState.BRIEFING: return 2;
			case SCR_EGameModeState.GAME: return 3;
			case SCR_EGameModeState.POSTGAME: return 4;
		}

		return -1;
	}
}
