modded class TILW_EndGameInstruction
{
	//------------------------------------------------------------------------------------------------
	override void Execute()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
		{
			super.Execute();
			return;
		}
		if (!Replication.IsServer())
			return;
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		if (!framework)
			return;
		if (framework.m_suppressGameEnd)
		{
			super.Execute();
			return;
		}
		FactionManager factions = GetGame().GetFactionManager();
		Faction faction = factions.GetFactionByKey(m_factionKey);
		int factionIndex = factions.GetFactionIndex(faction);
		manager.EnterPostGame(SCR_GameModeEndData.CreateSimple(m_gameOverType, -1, factionIndex));
	}
}
