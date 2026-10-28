//! Lobby2 already uses the native state domain; only the notification adapter is needed.
modded class GC_LobbyGameMode
{
	protected ref ScriptInvokerInt m_TILW_OnGameStateChange = new ScriptInvokerInt();

	//------------------------------------------------------------------------------------------------
	ScriptInvokerInt TILW_GetOnGameStateChange()
	{
		return m_TILW_OnGameStateChange;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnGameStateChanged()
	{
		super.OnGameStateChanged();
		if (GC_LobbyGameMode.GetGameMode() != this)
			return;
		m_TILW_OnGameStateChange.Invoke(GetState());
		if (Replication.IsServer() && GetState() == SCR_EGameModeState.GAME)
		{
			TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
			if (framework)
				framework.PlayerUpdate(0, null);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		if (!Replication.IsServer() || GC_LobbyGameMode.GetGameMode() != this)
			return;
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		if (framework)
			framework.PlayerUpdate(playerId, playerEntity);
	}
}
