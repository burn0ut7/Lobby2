#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref GC_LobbyMissionContext m_GC_PopulationTestContext;

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionPopulation(string step)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "UNAVAILABLE active GAME authority required";
		if (step == "start")
		{
			if (m_GC_PopulationTestContext)
				return "UNAVAILABLE already started";
			m_GC_PopulationTestContext = new GC_LobbyMissionContext(this);
			m_GC_PopulationTestContext.RequireAI();
			m_GC_PopulationTestContext.StartObservations();
			GC_SpawnPrefabAction spawn = new GC_SpawnPrefabAction();
			spawn.m_sPrefab = "{EA158B6EB6A24B4B}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman_Variant_1.et";
			spawn.m_sLocation = "MissionFrameworkMode";
			spawn.m_sNewName = "PopulationTestAI";
			bool spawned = spawn.Execute(m_GC_PopulationTestContext);
			return string.Format("spawned=%1; capture after native initialization", spawned);
		}
		if (!m_GC_PopulationTestContext)
			return "UNAVAILABLE start first";
		if (step == "kill")
		{
			m_GC_PopulationTestContext.CapturePlayers();
			float before;
			bool observed = m_GC_PopulationTestContext.TryGetAICasualties("US", before);
			GC_SetEntityHealthAction kill = new GC_SetEntityHealthAction();
			kill.m_sEntity = "PopulationTestAI";
			kill.m_fHealthPercentage = 0;
			bool killed = kill.Execute(m_GC_PopulationTestContext);
			return string.Format("observed=%1 before=%2 killed=%3", observed, before, killed);
		}
		m_GC_PopulationTestContext.CapturePlayers();
		float after;
		bool recorded = m_GC_PopulationTestContext.TryGetAICasualties("US", after);
		IEntity entity = GetGame().GetWorld().FindEntityByName("PopulationTestAI");
		bool dead = entity && !SCR_AIDamageHandling.IsAlive(entity);
		m_GC_PopulationTestContext.StopObservations();
		m_GC_PopulationTestContext = null;
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		return string.Format("pass=%1 recorded=%2 after=%3 dead=%4", recorded && after == 100 && dead, recorded, after, dead);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionRespawn(string step)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "UNAVAILABLE active GAME authority required";
		if (step == "prepare")
		{
			bool disabled = GC_GetMissionRuntime().ControlEvent("US defeated", GC_EMissionEventOperation.DISABLE, false);
			return string.Format("endingDisabled=%1", disabled);
		}
		GC_ChangeRespawnTicketsAction tickets = new GC_ChangeRespawnTicketsAction();
		tickets.m_sFaction = "US";
		tickets.m_eOperation = GC_EMissionNumberOperation.SET;
		tickets.m_iAmount = 1;
		tickets.m_bRespawnNow = true;
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		bool applied = tickets.Execute(context);
		return string.Format("applied=%1 remaining=%2", applied, m_mGC_MissionTickets.Get("US"));
	}
}
#endif
