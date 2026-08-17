enum GC_ELobbyPhase
{
	PREGAME = 0,
	BRIEFING = 1,
	GAME = 2,
	POSTGAME = 3
}

//! Owns Lobby2's ordered mission phase independently from the base game-mode enum.
class GC_LobbyPhaseMachine
{
	protected GC_ELobbyPhase m_eCurrent = GC_ELobbyPhase.PREGAME;

	GC_ELobbyPhase GetCurrent()
	{
		return m_eCurrent;
	}

	bool CanAdvanceTo(GC_ELobbyPhase target)
	{
		return target == GetNext(m_eCurrent);
	}

	bool Apply(GC_ELobbyPhase target, out GC_ELobbyPhase previous)
	{
		if (!IsValid(target) || GetOrdinal(target) <= GetOrdinal(m_eCurrent))
			return false;

		previous = m_eCurrent;
		m_eCurrent = target;
		return true;
	}

	static GC_ELobbyPhase GetNext(GC_ELobbyPhase phase)
	{
		switch (phase)
		{
			case GC_ELobbyPhase.PREGAME: return GC_ELobbyPhase.BRIEFING;
			case GC_ELobbyPhase.BRIEFING: return GC_ELobbyPhase.GAME;
			case GC_ELobbyPhase.GAME: return GC_ELobbyPhase.POSTGAME;
		}

		return -1;
	}

	static bool IsValid(GC_ELobbyPhase phase)
	{
		return phase == GC_ELobbyPhase.PREGAME
			|| phase == GC_ELobbyPhase.BRIEFING
			|| phase == GC_ELobbyPhase.GAME
			|| phase == GC_ELobbyPhase.POSTGAME;
	}

	static bool IsAtOrAfter(GC_ELobbyPhase phase, GC_ELobbyPhase threshold)
	{
		if (!IsValid(phase) || !IsValid(threshold))
			return false;

		return GetOrdinal(phase) >= GetOrdinal(threshold);
	}

	static string GetName(GC_ELobbyPhase phase)
	{
		switch (phase)
		{
			case GC_ELobbyPhase.PREGAME: return "PREGAME";
			case GC_ELobbyPhase.BRIEFING: return "BRIEFING";
			case GC_ELobbyPhase.GAME: return "GAME";
			case GC_ELobbyPhase.POSTGAME: return "POSTGAME";
		}

		return "UNKNOWN";
	}

	//! Explicit ordering keeps transition logic independent from enum storage values.
	protected static int GetOrdinal(GC_ELobbyPhase phase)
	{
		switch (phase)
		{
			case GC_ELobbyPhase.PREGAME: return 0;
			case GC_ELobbyPhase.BRIEFING: return 1;
			case GC_ELobbyPhase.GAME: return 2;
			case GC_ELobbyPhase.POSTGAME: return 3;
		}

		return -1;
	}
}
