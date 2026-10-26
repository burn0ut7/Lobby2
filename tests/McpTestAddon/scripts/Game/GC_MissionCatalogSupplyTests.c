#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionCatalogSupplies()
	{
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		if (!context.IsActive())
			return "UNAVAILABLE active authority";
		GC_SpawnPrefabAction spawn = new GC_SpawnPrefabAction();
		spawn.m_sPrefab = "{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et";
		spawn.m_sLocation = GetOwner().GetName();
		spawn.m_sNewName = "FrameworkSupplyTruck";
		bool spawned = spawn.Execute(context);
		GC_ServiceVehiclesAction service = new GC_ServiceVehiclesAction();
		service.m_aVehicles = {"FrameworkSupplyTruck"};
		service.m_bSetSupplies = true;
		service.m_iSupplies = 37;
		service.m_bRepair = false;
		bool serviced = service.Execute(context);
		IEntity vehicle = GetGame().GetWorld().FindEntityByName("FrameworkSupplyTruck");
		SCR_ResourceComponent resources = SCR_ResourceComponent.FindResourceComponent(vehicle, false);
		SCR_ResourceContainer container;
		bool supplies = resources && resources.GetContainer(EResourceType.SUPPLIES, container) && container.GetResourceValue() == 37;
		array<string> audience = {"US"};
		bool created = SetBriefingMarker("FrameworkMarker", "{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset",
			"{67B3A6DC2D712B52}UI/Textures/Icons/icons_mapMarkersUI-glow.imageset", "circle-2", GetOwner().GetOrigin(),
			0, true, 10, 1, "Framework marker test", audience, SCR_EGameModeState.PREGAME);
		GC_SetMarkerVisibilityAction visibility = new GC_SetMarkerVisibilityAction();
		visibility.m_aMarkers = {"FrameworkMarker"};
		visibility.m_bVisible = false;
		bool hidden = visibility.Execute(context);
		GC_BriefingMarker marker = GetBriefingMarker("FrameworkMarker");
		bool preserved = marker && !marker.IsVisible() && marker.GetFactionKeys().Contains("US");
		visibility.m_bVisible = true;
		visibility.m_bChangeAudience = true;
		visibility.m_aFactions = {"USSR"};
		bool changed = visibility.Execute(context);
		marker = GetBriefingMarker("FrameworkMarker");
		bool audienceChanged = marker && marker.IsVisible() && marker.GetFactionKeys().Contains("USSR") && !marker.GetFactionKeys().Contains("US");
		GC_DeleteEntitiesAction cleanup = new GC_DeleteEntitiesAction();
		cleanup.m_aEntities = {"FrameworkSupplyTruck"};
		cleanup.Execute(context);
		bool passed = spawned && serviced && supplies && created && hidden && preserved && changed && audienceChanged;
		return string.Format("pass=%1 spawned=%2 serviced=%3 supplies=%4 marker=%5 hidden=%6 audience=%7", passed,
			spawned, serviced, supplies, created, hidden && preserved, changed && audienceChanged);
	}
}
#endif
