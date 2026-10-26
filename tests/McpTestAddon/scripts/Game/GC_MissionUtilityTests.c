#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionAreaCapacity()
	{
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		if (!context.IsActive())
			return "UNAVAILABLE active authority required";
		GC_SpawnPrefabsInAreaAction spawn = new GC_SpawnPrefabsInAreaAction();
		spawn.m_sArea = "MissionFrameworkMode";
		spawn.m_fRadius = 10;
		spawn.m_aPrefabs = {"{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et"};
		spawn.m_iCount = 1000;
		spawn.m_sNamePrefix = "CapacitySpawn";
		spawn.m_bAvoidWater = false;
		bool spawned = spawn.Execute(context);
		int found;
		for (int i = 1; i <= 1000; i++)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName("CapacitySpawn_" + i);
			if (entity)
			{
				found++;
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}
		}
		return string.Format("pass=%1 spawned=%2 found=%3", spawned && found == 1000, spawned, found);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionUtilities()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority required";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "Utility test";
		array<ref GC_MissionFlag> flags = {flag};
		array<ref GC_MissionEvent> events = {};
		bool started = runtime.Start(context, events, flags);
		GC_SpawnPrefabsInAreaAction spawn = new GC_SpawnPrefabsInAreaAction();
		spawn.m_sArea = "MissionFrameworkMode";
		spawn.m_fRadius = 10;
		spawn.m_aPrefabs = {"{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et"};
		spawn.m_iCount = 2;
		spawn.m_sNamePrefix = "UtilitySpawn";
		spawn.m_bAvoidWater = false;
		string error;
		bool spawned = spawn.Validate(context, error) && spawn.Execute(context);
		IEntity first = GetGame().GetWorld().FindEntityByName("UtilitySpawn_1");
		IEntity second = GetGame().GetWorld().FindEntityByName("UtilitySpawn_2");
		bool positions = first && second && vector.DistanceSqXZ(first.GetOrigin(), GetOwner().GetOrigin()) <= 100
			&& vector.DistanceSqXZ(second.GetOrigin(), GetOwner().GetOrigin()) <= 100;
		GC_SetDeletionProtectionAction protection = new GC_SetDeletionProtectionAction();
		protection.m_aEntities = {"UtilitySpawn_1"};
		protection.m_bProtected = true;
		bool protectionApplied = protection.Execute(context) && !context.CanDelete(first);
		GC_DeleteEntitiesInAreaAction remove = new GC_DeleteEntitiesInAreaAction();
		remove.m_sArea = "MissionFrameworkMode";
		remove.m_fRadius = 20;
		remove.m_aPrefabs = spawn.m_aPrefabs;
		remove.m_bVisualObjectsOnly = false;
		remove.m_bRootsOnly = true;
		bool removed = remove.Execute(context);
		bool selective = GetGame().GetWorld().FindEntityByName("UtilitySpawn_1") != null
			&& GetGame().GetWorld().FindEntityByName("UtilitySpawn_2") == null;
		protection.m_bProtected = false;
		bool released = protection.Execute(context) && context.CanDelete(first);
		GC_DeleteEntitiesAction cleanup = new GC_DeleteEntitiesAction();
		cleanup.m_aEntities = {"UtilitySpawn_1"};
		bool cleaned = cleanup.Execute(context);
		array<string> names = {};
		array<bool> values = {};
		runtime.CopyFlagSnapshot(names, values);
		bool validSnapshot = names.Count() == 1 && values.Count() == 1;
		flags.Insert(null);
		bool invalidRejected = !runtime.Start(context, events, flags);
		runtime.CopyFlagSnapshot(names, values);
		bool invalidSnapshot = names.IsEmpty() && values.IsEmpty();
		runtime.Stop();
		bool passed = started && spawned && positions && protectionApplied && removed && selective && released && cleaned
			&& validSnapshot && invalidRejected && invalidSnapshot;
		return string.Format("pass=%1 spawned=%2 positions=%3 protected=%4 selective=%5 released=%6 cleaned=%7",
			passed, spawned, positions, protectionApplied, selective, released, cleaned)
			+ string.Format(" validSnapshot=%1 invalidRejected=%2 invalidSnapshot=%3", validSnapshot, invalidRejected, invalidSnapshot);
	}
}
#endif
