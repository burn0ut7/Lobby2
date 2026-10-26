//! The mission framework's only Lobby2 integration boundary. Native player and
//! faction ownership are queried afresh; the maps are one evaluation's observations.
class GC_LobbyMissionContext
{
	protected GC_LobbyManager m_Manager;
	protected GC_MissionRuntime m_Runtime;
	protected ref GC_MissionPopulation m_Population = new GC_MissionPopulation();
	protected ref map<GC_MissionCondition, float> m_mHeldSince = new map<GC_MissionCondition, float>();
	protected ref map<GC_MissionCondition, ref GC_MissionCaptureProgress> m_mCapture = new map<GC_MissionCondition, ref GC_MissionCaptureProgress>();
	protected ref map<GC_MissionCondition, ref GC_MissionAreaDebug> m_mAreaDebug = new map<GC_MissionCondition, ref GC_MissionAreaDebug>();
	protected bool m_bNeedsAI;
	protected float m_fNow;

	//------------------------------------------------------------------------------------------------
	void GC_LobbyMissionContext(GC_LobbyManager manager)
	{
		m_Manager = manager;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOwned()
	{
		return Replication.IsServer() && m_Manager && GC_LobbyManager.GetManager() == m_Manager;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return IsOwned() && m_Manager.GetLobbyPhase() == SCR_EGameModeState.GAME;
	}

	//------------------------------------------------------------------------------------------------
	bool HasEnded()
	{
		return !IsOwned() || m_Manager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME;
	}

	//------------------------------------------------------------------------------------------------
	bool IsKnownFaction(string factionKey)
	{
		return factionKey != string.Empty && GetGame().GetFactionManager()
			&& GetGame().GetFactionManager().GetFactionByKey(factionKey) != null;
	}

	//------------------------------------------------------------------------------------------------
	void Bind(GC_MissionRuntime runtime)
	{
		m_Runtime = runtime;
		m_Population = new GC_MissionPopulation();
		m_mHeldSince.Clear();
		m_mCapture.Clear();
		m_mAreaDebug.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void StartObservations()
	{
		m_Population.Start(m_bNeedsAI);
	}

	//------------------------------------------------------------------------------------------------
	void StopObservations()
	{
		m_Population.Stop();
		m_Runtime = null;
		m_mHeldSince.Clear();
		m_mCapture.Clear();
		m_mAreaDebug.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void ResetCondition(GC_MissionCondition condition)
	{
		m_mHeldSince.Remove(condition);
		m_mCapture.Remove(condition);
		m_mAreaDebug.Remove(condition);
	}

	//------------------------------------------------------------------------------------------------
	void DrawArea(GC_MissionCondition condition, IEntity area, float radius)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		if (!m_mAreaDebug.Contains(condition))
			m_mAreaDebug.Set(condition, new GC_MissionAreaDebug());
		m_mAreaDebug.Get(condition).Draw(area, radius);
	}

	//------------------------------------------------------------------------------------------------
	GC_MissionCaptureProgress CaptureProgress(GC_MissionCondition condition)
	{
		if (!m_mCapture.Contains(condition))
			m_mCapture.Set(condition, new GC_MissionCaptureProgress());
		return m_mCapture.Get(condition);
	}

	//------------------------------------------------------------------------------------------------
	float GetTime()
	{
		return m_fNow;
	}

	//------------------------------------------------------------------------------------------------
	void RequireAI()
	{
		m_bNeedsAI = true;
	}

	//------------------------------------------------------------------------------------------------
	void CapturePlayers()
	{
		m_Population.Capture(IsRunning());
	}

	//------------------------------------------------------------------------------------------------
	bool TryGetPlayerCasualties(string faction, out float percentage)
	{
		return IsRunning() && m_Population.PlayerCasualties(faction, percentage);
	}

	//------------------------------------------------------------------------------------------------
	bool TryGetAICasualties(string faction, out float percentage)
	{
		return IsRunning() && m_Population.AICasualties(faction, percentage);
	}

	//------------------------------------------------------------------------------------------------
	array<SCR_ChimeraCharacter> GetCharacters()
	{
		return m_Population.GetCharacters();
	}

	//------------------------------------------------------------------------------------------------
	bool IsActive()
	{
		return IsOwned() && !HasEnded();
	}

	//------------------------------------------------------------------------------------------------
	GC_LobbyManager GetManager()
	{
		return m_Manager;
	}

	//------------------------------------------------------------------------------------------------
	GC_MissionRuntime GetRuntime()
	{
		return m_Runtime;
	}

	//------------------------------------------------------------------------------------------------
	bool HasFlag(string name)
	{
		return m_Runtime && m_Runtime.HasFlag(name);
	}

	//------------------------------------------------------------------------------------------------
	bool IsFlagSet(string name)
	{
		return m_Runtime && m_Runtime.IsFlagSet(name);
	}

	//------------------------------------------------------------------------------------------------
	bool SetFlag(string name, bool value)
	{
		return IsActive() && m_Runtime && m_Runtime.SetFlag(name, value);
	}

	//------------------------------------------------------------------------------------------------
	bool CanDelete(IEntity entity)
	{
		if (!IsActive() || !entity)
			return false;
		IEntity ancestor = GetGame().GetGameMode();
		while (ancestor)
		{
			if (ancestor == entity)
				return false;
			ancestor = ancestor.GetParent();
		}
		return !m_Runtime || !m_Runtime.IsEntityProtected(entity);
	}

	//------------------------------------------------------------------------------------------------
	void SetTime(float now)
	{
		m_fNow = now;
	}

	//------------------------------------------------------------------------------------------------
	bool HeldFor(GC_MissionCondition condition, bool met, float seconds)
	{
		if (!met)
		{
			m_mHeldSince.Remove(condition);
			return false;
		}
		if (!m_mHeldSince.Contains(condition))
			m_mHeldSince.Set(condition, m_fNow);
		return m_fNow - m_mHeldSince.Get(condition) >= seconds;
	}

	//------------------------------------------------------------------------------------------------
	bool EndMission(string winningFactionKey, EGameOverTypes ending = EGameOverTypes.EDITOR_FACTION_VICTORY)
	{
		if (!IsRunning() || !IsKnownFaction(winningFactionKey))
			return false;
		if (m_Manager.GC_IsMissionEndPreview())
		{
			PrintFormat("GC | Mission Framework | mission end preview: winner=%1 ending=%2; mission remains running", winningFactionKey, ending);
			return true;
		}
		Faction winner = GetGame().GetFactionManager().GetFactionByKey(winningFactionKey);
		int winnerIndex = GetGame().GetFactionManager().GetFactionIndex(winner);
		ref SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(ending, -1, winnerIndex);
		return m_Manager.EnterPostGame(endData);
	}
}
