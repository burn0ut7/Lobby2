//! Explicit optional fixtures exercise production map filtering and drawing.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapFrameSpawn(string name)
	{
		if (!Replication.IsServer() || GetGame().GetWorld().FindEntityByName(name))
			return "UNAVAILABLE";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = Vector(1300, 0, 2800);
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load("{8F470B543A20D944}Prefabs/Logic/AO/AOLimit.et"), GetGame().GetWorld(), params);
		if (!entity)
			return "FAILED";
		entity.SetName(name);
		return name;
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestMapFrameConfigure(string name, float width, float height, vector offset, bool frame, string factionKey)
	{
		GC_AOLimitEntity limit = GC_AOLimitEntity.Cast(GetGame().GetWorld().FindEntityByName(name));
		if (!Replication.IsServer() || !limit)
			return "UNAVAILABLE AO";
		vector center = Vector(1300, 0, 2800) + offset;
		array<vector> polygon = {center + Vector(-width / 2, 0, -height / 2),
			center + Vector(width / 2, 0, -height / 2), center + Vector(width / 2, 0, -height / 4),
			center + Vector(-width / 4, 0, -height / 4), center + Vector(-width / 4, 0, height / 4),
			center + Vector(width / 2, 0, height / 4), center + Vector(width / 2, 0, height / 2),
			center + Vector(-width / 2, 0, height / 2)};
		bool configured = limit.SetPolygon(polygon, false, "");
		limit.SetEnabled(false);
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(limit.FindComponent(GC_MapShapeComponent));
		array<string> factions = {};
		if (!factionKey.IsEmpty())
			factions.Insert(factionKey);
		shape.SetFactionKeys(factions);
		shape.SetStyle(0xFF0088FF, 3, false, false, 0);
		shape.SetEnabled(true);
		shape.SetAutoFrameMap(frame);
		return configured.ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapFrameRead(bool resetFirstFrame = false)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (resetFirstFrame)
			controller.GC_TestMapFrameReset();
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {}, bounds = {};
		array<string> names = {};
		GC_LobbyGameMode.GetGameMode().GetMapShapes().GetSnapshot(controller.GetPlayerId(), counts, colors, fills, inverted, widths, points, names, bounds);
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		string result = "bounds=" + bounds.Count().ToString();
		if (mapEntity && mapEntity.IsOpen())
		{
			float width, height, centerX, centerZ;
			mapEntity.GetMapWidget().GetScreenSize(width, height);
			mapEntity.GetMapCenterWorldPosition(centerX, centerZ);
			result += string.Format(" viewport=%1,%2 zoom=%3 center=%4,%5", width, height, mapEntity.GetCurrentZoom(), centerX, centerZ);
			foreach (vector point : bounds)
			{
				int x, y;
				mapEntity.WorldToScreen(point[0], point[2], x, y, true);
				result += string.Format(" world=%1 pixel=%2,%3", point, x, y);
			}
		}
		return result + " " + controller.GC_TestAOMapLocal();
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapSurface(string surface, bool open)
	{
		if (surface == "spectator")
		{
			GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
			if (!menu)
				return "UNAVAILABLE spectator menu";
			return menu.GC_TestAOMapOpen(open);
		}
		if (surface != "gameplay")
			return "REJECTED surface";
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		if (!character)
			return "UNAVAILABLE character";
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		if (inventory)
			inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			SCR_MapGadgetComponent gadget = SCR_MapGadgetComponent.Cast(item.FindComponent(SCR_MapGadgetComponent));
			if (!gadget)
				continue;
			gadget.SetMapMode(open);
			return "DISPATCHED native map gadget";
		}
		return "UNAVAILABLE inventory map";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapStyle(int index, int visibility, int color, float width, bool factionColor, string factionKey)
	{
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !m_aGC_AOFixtures[index])
			return "UNAVAILABLE fixture";
		GC_AOLimitEntity limit = m_aGC_AOFixtures[index];
		array<string> factions = {};
		if (!factionKey.IsEmpty())
			factions.Insert(factionKey);
		bool eligibility = limit.SetEligibility(factions, GC_EAOVehicleState.ANY,
			GC_EAOListMode.IGNORE, {}, GC_EAOItemCondition.IGNORE, {});
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(limit.FindComponent(GC_MapShapeComponent));
		if (!shape)
			return "UNAVAILABLE map shape component";
		return string.Format("eligibility=%1 visibility=%2 style=%3 enabled=%4", eligibility,
			shape.SetFactionKeys(factions), shape.SetStyle(color, width, factionColor, false, 0), shape.SetEnabled(visibility != 3));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapSnapshot()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		GC_LobbyGameMode.GetGameMode().GetMapShapes().GetSnapshot(controller.GetPlayerId(), counts, colors, fills, inverted, widths, points);
		string result = string.Format("serverOutlines=%1 points=%2", counts.Count(), points.Count());
		foreach (int i, int count : counts)
			result += string.Format(" [%1 vertices=%2 color=%3 width=%4]", i, count, colors[i], widths[i]);
		return result + " " + controller.GC_TestAOMapLocal();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapView(float zoom, vector offset)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return "UNAVAILABLE open map";
		vector center = m_vGC_AOOrigin + offset;
		mapEntity.SetZoom(zoom, true);
		// Let the map's native frame update finish zooming before centering.
		GetGame().GetCallqueue().CallLater(GC_TestAOMapPan, 100, false, center);
		return string.Format("zoom=%1 center=%2", mapEntity.GetCurrentZoom(), center);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestAOMapPan(vector center)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;
		int x, y;
		mapEntity.WorldToScreen(center[0], center[2], x, y);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		mapEntity.SetPan(workspace.DPIUnscale(x), workspace.DPIUnscale(y));
	}
}

modded class GC_Lobby_SpectatorMenu
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapOpen(bool open)
	{
		if (m_bSpectatorMapOpen != open)
			OnToggleSpectatorMap();
		return string.Format("spectatorMapOpen=%1", m_bSpectatorMapOpen);
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_TestMapFrameReset()
	{
		if (m_GC_MapShapes)
			m_GC_MapShapes.GC_TestResetFrame();
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapLocal()
	{
		if (!m_GC_MapShapes)
			return "overlay=absent";
		return m_GC_MapShapes.GC_TestMapState();
	}
}

modded class GC_MapShapeOverlay
{
	//------------------------------------------------------------------------------------------------
	void GC_TestResetFrame()
	{
		m_bFramed = false;
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestMapState()
	{
		string result = string.Format("canvas=%1 generation=%2 commands=%3 framed=%4", m_Canvas != null, m_iGeneration, m_aCommands.Count(), m_bFramed);
		foreach (CanvasWidgetCommand command : m_aCommands)
		{
			PolygonDrawCommand fill = PolygonDrawCommand.Cast(command);
			if (fill)
			{
				result += string.Format(" [fill vertices=%1 color=%2]", fill.m_Vertices.Count() / 2, fill.m_iColor);
				continue;
			}
			TriMeshDrawCommand outside = TriMeshDrawCommand.Cast(command);
			if (outside)
			{
				result += string.Format(" [outside vertices=%1 triangles=%2 color=%3]", outside.m_Vertices.Count() / 2, outside.m_Indices.Count() / 3, outside.m_iColor);
				continue;
			}
			LineDrawCommand line = LineDrawCommand.Cast(command);
			result += string.Format(" [line vertices=%1 color=%2 width=%3 closed=%4 first=%5,%6]",
				line.m_Vertices.Count() / 2, line.m_iColor, line.m_fWidth, line.m_bShouldEnclose, line.m_Vertices[0], line.m_Vertices[1]);
		}
		return result;
	}
}
