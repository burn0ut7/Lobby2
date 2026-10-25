//! Explicit optional fixtures exercise production map filtering and drawing.
modded class GC_LobbyManager
{
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
	string GC_TestMapState()
	{
		string result = string.Format("canvas=%1 generation=%2 commands=%3", m_Canvas != null, m_iGeneration, m_aCommands.Count());
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
