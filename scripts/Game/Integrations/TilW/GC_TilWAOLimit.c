modded class TILW_AOLimitComponent
{
	protected GC_LobbyManager m_TILW_Lobby2Manager;

	//------------------------------------------------------------------------------------------------
	void TILW_ListenToLobby2Faction()
	{
		m_TILW_Lobby2Manager = GC_LobbyManager.GetManager();
		if (m_TILW_Lobby2Manager)
			m_TILW_Lobby2Manager.m_OnPlayerAffiliationChanged.Insert(TILW_OnLobby2FactionChanged);
	}

	//------------------------------------------------------------------------------------------------
	protected void TILW_OnLobby2FactionChanged(int playerId)
	{
		if (GC_LobbyManager.GetManager() == m_TILW_Lobby2Manager)
			LobbySlottingChanged();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_TILW_Lobby2Manager)
			m_TILW_Lobby2Manager.m_OnPlayerAffiliationChanged.Remove(TILW_OnLobby2FactionChanged);
		m_TILW_Lobby2Manager = null;
		super.OnDelete(owner);
	}
}
