modded class GC_LobbyManager
{
	protected ref GC_MissionVote m_GC_MissionVote;

	//------------------------------------------------------------------------------------------------
	GC_MissionVote GC_GetMissionVote()
	{
		return m_GC_MissionVote;
	}

	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (Replication.IsServer() && GetManager() == this)
		{
			m_GC_MissionVote = new GC_MissionVote(this);
			m_GC_MissionVote.Prepare();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		if (state == SCR_EGameModeState.POSTGAME && m_GC_MissionVote)
			m_GC_MissionVote.Start();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		if (m_GC_MissionVote)
			m_GC_MissionVote.Stop();
		super.OnGameEnd();
	}
}
