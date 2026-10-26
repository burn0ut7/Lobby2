#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref GC_MissionRuntime m_GC_WorldTestRuntime;
	protected ref GC_LobbyMissionContext m_GC_WorldTestContext;

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionWorldStart()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME || m_GC_WorldTestRuntime)
			return "UNAVAILABLE fresh active session required";
		m_GC_WorldTestContext = new GC_LobbyMissionContext(this);
		GC_SpawnPrefabAction waypoint = new GC_SpawnPrefabAction();
		waypoint.m_sPrefab = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
		waypoint.m_sLocation = "MissionFrameworkMode";
		waypoint.m_sNewName = "FrameworkTestWaypoint";
		bool spawnedWaypoint = waypoint.Execute(m_GC_WorldTestContext);
		GC_SpawnAIGroupAction group = new GC_SpawnAIGroupAction();
		group.m_sPrefab = "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et";
		group.m_sLocation = "MissionFrameworkMode";
		group.m_sNewName = "FrameworkTestGroup";
		group.m_aWaypoints = {"FrameworkTestWaypoint"};
		bool spawnedGroup = group.Execute(m_GC_WorldTestContext);
		AIGroup nativeGroup = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(group.m_sNewName));
		array<AIWaypoint> waypoints = {};
		if (nativeGroup)
			nativeGroup.GetWaypoints(waypoints);
		bool assigned = waypoints.Count() == 1;
		GC_AssignWaypointsAction clear = new GC_AssignWaypointsAction();
		clear.m_sGroup = group.m_sNewName;
		clear.m_bClearExisting = true;
		bool cleared = clear.Execute(m_GC_WorldTestContext);
		waypoints.Clear();
		if (nativeGroup)
			nativeGroup.GetWaypoints(waypoints);
		cleared = cleared && waypoints.IsEmpty();

		GC_SpawnCrewedVehicleAction vehicle = new GC_SpawnCrewedVehicleAction();
		vehicle.m_sPrefab = "{3EA6F47D95867114}Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et";
		vehicle.m_sLocation = "MissionFrameworkMode";
		vehicle.m_sNewName = "FrameworkTestVehicle";
		GC_MissionCrewSeats seats = new GC_MissionCrewSeats();
		seats.m_bDrivers = true;
		seats.m_bGunners = true;
		seats.m_aCharacters = {
			"{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et",
			"{5B1996C05B1E51A4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_AR.et"
		};
		GC_MissionCrewGroup crew = new GC_MissionCrewGroup();
		crew.m_bEnabled = true;
		crew.m_sName = "FrameworkTestCrew";
		crew.m_aSeats = {seats};
		crew.m_bKeepGunnersSeated = true;
		vehicle.m_aCrew = {crew};
		GC_AlwaysCondition always = new GC_AlwaysCondition();
		GC_MissionEvent rule = new GC_MissionEvent();
		rule.m_sName = "World action test";
		rule.m_bAllowBeforeGame = true;
		rule.m_aConditions = {always};
		rule.m_aActions = {vehicle};
		array<ref GC_MissionEvent> rules = {rule};
		m_GC_WorldTestRuntime = new GC_MissionRuntime();
		bool started = m_GC_WorldTestRuntime.Start(m_GC_WorldTestContext, rules);
		m_GC_WorldTestRuntime.Update(1);
		return string.Format("waypoint=%1 group=%2 assigned=%3 cleared=%4 started=%5 vehicle=%6",
			spawnedWaypoint, spawnedGroup, assigned, cleared, started, GetGame().GetWorld().FindEntityByName(vehicle.m_sNewName) != null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionWorldFinish()
	{
		if (!m_GC_WorldTestRuntime || !m_GC_WorldTestContext || !m_GC_WorldTestContext.IsActive())
			return "UNAVAILABLE start world test first";
		m_GC_WorldTestRuntime.Update(3);
		IEntity vehicle = GetGame().GetWorld().FindEntityByName("FrameworkTestVehicle");
		if (!vehicle)
			return "FAIL vehicle missing";
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> seats = {};
		compartments.GetCompartments(seats);
		int occupied;
		int customCrew;
		foreach (BaseCompartmentSlot seat : seats)
		{
			if (seat.IsOccupied())
			{
				occupied++;
				ResourceName prefab = seat.GetOccupant().GetPrefabData().GetPrefabName();
				if (prefab.Contains("Character_US_GL.et") || prefab.Contains("Character_US_AR.et"))
					customCrew++;
			}
		}
		GC_ServiceVehiclesAction service = new GC_ServiceVehiclesAction();
		service.m_aVehicles = {"FrameworkTestVehicle"};
		service.m_bSetFuel = true;
		service.m_fFuelPercentage = 25;
		service.m_bRepair = true;
		bool serviced = service.Execute(m_GC_WorldTestContext);
		SCR_FuelManagerComponent fuel = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
		bool fuelMatches = fuel && Math.AbsFloat(fuel.GetTotalFuel() / fuel.GetTotalMaxFuel() - 0.25) < 0.01;
		GC_SetEntityHealthAction health = new GC_SetEntityHealthAction();
		health.m_sEntity = "FrameworkTestVehicle";
		health.m_fHealthPercentage = 50;
		bool healthApplied = health.Execute(m_GC_WorldTestContext);
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.GetDamageManager(vehicle);
		bool healthMatches = damage && Math.AbsFloat(damage.GetDefaultHitZone().GetHealthScaled() - 0.5) < 0.01;
		GC_EntitiesInAreaCondition area = new GC_EntitiesInAreaCondition();
		area.m_sArea = "MissionFrameworkMode";
		area.m_fRadius = 100;
		area.m_eComparison = GC_EMissionCountComparison.AT_LEAST;
		area.m_fPercentage = 100;
		area.m_aEntities = {"FrameworkTestVehicle"};
		bool inside = area.Evaluate(m_GC_WorldTestContext);
		array<string> audience = {"US"};
		SetBriefing(32, "Framework test", "Visibility check", audience, SCR_EGameModeState.PREGAME);
		GC_SetBriefingVisibilityAction visibility = new GC_SetBriefingVisibilityAction();
		visibility.m_iBriefing = 32;
		visibility.m_bVisible = false;
		bool hidden = visibility.Execute(m_GC_WorldTestContext) && !GetBriefing(32).IsVisible()
			&& GetBriefing(32).GetFactionKeys().Contains("US");
		GC_ChangeRespawnTicketsAction tickets = new GC_ChangeRespawnTicketsAction();
		tickets.m_sFaction = "US";
		tickets.m_eOperation = GC_EMissionNumberOperation.SET;
		tickets.m_iAmount = 3;
		bool budget = tickets.Execute(m_GC_WorldTestContext) && m_mGC_MissionTickets.Get("US") == 3;
		tickets.m_eOperation = GC_EMissionNumberOperation.MULTIPLY;
		tickets.m_iAmount = 2;
		budget = tickets.Execute(m_GC_WorldTestContext) && m_mGC_MissionTickets.Get("US") == 6 && budget;
		GC_DeleteEntitiesAction remove = new GC_DeleteEntitiesAction();
		remove.m_aEntities = {"FrameworkTestVehicle", "FrameworkTestWaypoint", "FrameworkTestGroup"};
		bool deleted = remove.Execute(m_GC_WorldTestContext) && !GetGame().GetWorld().FindEntityByName("FrameworkTestVehicle");
		m_GC_WorldTestRuntime.Stop();
		m_GC_WorldTestRuntime = null;
		m_GC_WorldTestContext = null;
		bool passed = occupied == 2 && customCrew == 2 && serviced && fuelMatches && healthApplied && healthMatches && inside && hidden && budget && deleted;
		return string.Format("pass=%1 crew=%2 service=%3 fuel=%4 health=%5 inside=%6 hidden=%7", passed, occupied, serviced, fuelMatches, healthApplied && healthMatches, inside, hidden)
			+ string.Format(" budget=%1 deleted=%2 customCrew=%3", budget, deleted, customCrew);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionWorldBoard()
	{
		if (!m_GC_WorldTestRuntime || !m_GC_WorldTestContext || !m_GC_WorldTestContext.IsActive())
			return "UNAVAILABLE start world test first";
		m_GC_WorldTestRuntime.Update(2);
		IEntity vehicle = GetGame().GetWorld().FindEntityByName("FrameworkTestVehicle");
		if (!vehicle)
			return "FAIL vehicle missing";
		string result = string.Format("prefab=%1", vehicle.GetPrefabData().GetPrefabName());
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> seats = {};
		compartments.GetCompartments(seats);
		foreach (BaseCompartmentSlot seat : seats)
			result += string.Format(" [type=%1 accessible=%2 occupied=%3 prefab=%4]", seat.GetType(), seat.IsCompartmentAccessible(), seat.IsOccupied(), seat.GetDefaultOccupantPrefab());
		return result;
	}
}
#endif
