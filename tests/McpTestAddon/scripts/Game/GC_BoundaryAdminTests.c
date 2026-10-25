//! Optional fixtures exercise production requests and rendered buttons without granting permissions.
modded class GC_LobbyManager
{
	protected string m_sGC_BoundaryResult;

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryRequest(string action, string entityName, string title, int seconds, string factionKeys)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		IEntity target = GetGame().GetWorld().FindEntityByName(entityName);
		string key;
		if (target)
			key = target.GetID().ToString();
		array<string> factions = {};
		if (!factionKeys.IsEmpty())
			factionKeys.Split(",", factions, true);
		controller.m_GC_OnBoundaryResult.Remove(GC_TestBoundaryResult);
		controller.m_GC_OnBoundaryResult.Insert(GC_TestBoundaryResult);
		m_sGC_BoundaryResult = "Pending";
		controller.GC_RequestBoundary(action, key, title, seconds, factions, controller.ReserveAdminPlayerToolRequest());
		return m_sGC_BoundaryResult;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBoundaryResult(int requestId, string outcome, string snapshot)
	{
		m_sGC_BoundaryResult = outcome + "\n" + snapshot;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryCatalog()
	{
		PlayerController player = GetGame().GetPlayerController();
		return GC_LobbyGameMode.GetGameMode().GetBoundaryAdmin().Snapshot(player.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryDenied(string entityName = "", int seconds = 0)
	{
		if (!entityName.IsEmpty())
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
			if (!entity)
				return "UNAVAILABLE entity";
			return GC_LobbyGameMode.GetGameMode().GetBoundaryAdmin().Execute(-1, "extend", entity.GetID().ToString(), "", seconds, {});
		}
		return GC_LobbyGameMode.GetGameMode().GetBoundaryAdmin().Execute(-1, "panic", "", "", 0, {});
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundarySetup(string name, string factionKeys, int behavior, float seconds, vector offset)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity())
			return "UNAVAILABLE player";
		vector center = player.GetControlledEntity().GetOrigin() + offset;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = center;
		GC_SetupZoneEntity zone = GC_SetupZoneEntity.Cast(GetGame().SpawnEntityPrefab(
			Resource.Load("{6A4D831C2E5079B3}Prefabs/Logic/Setup/SetupZone.et"), GetGame().GetWorld(), params));
		if (!zone)
			return "FAILED spawn";
		zone.SetName(name);
		array<vector> polygon = {center + Vector(-30, 0, -30), center + Vector(30, 0, -30),
			center + Vector(30, 0, 30), center + Vector(-30, 0, 30)};
		array<string> factions = {};
		if (!factionKeys.IsEmpty())
			factionKeys.Split(",", factions, true);
		return zone.Configure(polygon, factions, GC_LobbyGameMode.GetGameMode().GetElapsedTime() + seconds, behavior, 120, true).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryUi(string entityName, string action)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget panel = menu.GetRootWidget().FindAnyWidget("BoundaryPanel");
		if (!panel)
			return "UNAVAILABLE panel";
		GC_AdminBoundariesComponent component = GC_AdminBoundariesComponent.Cast(panel.FindHandler(GC_AdminBoundariesComponent));
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!component)
			return "UNAVAILABLE component";
		string key;
		if (entity)
			key = entity.GetID().ToString();
		return component.GC_TestBoundaryRow(key, action);
	}



	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryMapFocus(float zoom)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		if (!mapEntity || !mapEntity.IsOpen() || !character)
			return "UNAVAILABLE open map and character";
		vector position = character.GetOrigin();
		mapEntity.SetZoom(zoom, true);
		int x, y;
		mapEntity.WorldToScreenCustom(position[0], position[2], x, y, mapEntity.GetCurrentZoom(), false);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		mapEntity.SetPan(workspace.DPIUnscale(x), workspace.DPIUnscale(y));
		return "DISPATCHED native map centering";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryMap()
	{
		PlayerController player = GetGame().GetPlayerController();
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		array<string> names = {};
		GC_LobbyGameMode.GetGameMode().GetMapShapes().GetSnapshot(player.GetPlayerId(), counts, colors, fills, inverted, widths, points, names);
		string result = string.Format("admin=%1 outlines=%2 ", SCR_Global.IsAdmin(player.GetPlayerId()), counts.Count());
		foreach (string name : names)
			result += name + "; ";
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryFaction(string key)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget panel = menu.GetRootWidget().FindAnyWidget("BoundaryPanel");
		GC_AdminBoundariesComponent component = GC_AdminBoundariesComponent.Cast(panel.FindHandler(GC_AdminBoundariesComponent));
		return component.GC_TestBoundaryFactionClick(key);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryHud()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		Widget root = GetGame().GetWorkspace().FindAnyWidget("LobbyAnnouncementCountdown");
		string title, time;
		if (root)
		{
			title = TextWidget.Cast(root.FindAnyWidget("StagingCountdownTitle")).GetText();
			time = TextWidget.Cast(root.FindAnyWidget("StagingCountdownTime")).GetText();
		}
		return string.Format("notice=%1 setup=%2 frozen=%3 title=%4 time=%5 %6",
			controller.GC_AnnouncementActive(), controller.GC_SetupActive(), controller.GC_IsSetupFrozen(),
			title, time, GC_LobbyManager.Cast(this).GC_TestSetupSnapshot());
	}
}

modded class GC_AdminBoundariesComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryFactionClick(string key)
	{
		if (m_iTab != 2)
			return "UNAVAILABLE announcement tab";
		foreach (SCR_ButtonBaseComponent button, string factionKey : m_mFactions)
		{
			if (key != factionKey)
				continue;
			button.OnClick(button.GetRootWidget(), 0, 0, 0);
			return string.Format("all=%1 selected=%2", m_bAllFactions, m_aSelectedFactions.Count());
		}
		return "UNAVAILABLE faction";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryRow(string key, string action)
	{
		Widget row;
		if (!m_mRows.Find(key, row))
			return "UNAVAILABLE row " + key;
		if (action.StartsWith("time="))
		{
			EditBoxWidget.Cast(row.FindAnyWidget("ZoneTime")).SetText(action.Substring(5, action.Length() - 5));
			return "time=" + EditBoxWidget.Cast(row.FindAnyWidget("ZoneTime")).GetText();
		}
		if (!action.IsEmpty())
		{
			Widget target = row.FindAnyWidget(action);
			Widget ancestor = target;
			while (ancestor)
			{
				if (!ancestor.IsVisible() || !ancestor.IsEnabled())
					return "UNAVAILABLE hidden or disabled";
				ancestor = ancestor.GetParent();
			}
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
			if (!button)
				return "UNAVAILABLE button";
			button.OnClick(target, 0, 0, 0);
		}
		return TextWidget.Cast(row.FindAnyWidget("ZoneName")).GetText() + "\n"
			+ TextWidget.Cast(row.FindAnyWidget("ZoneBody")).GetText() + "\n"
			+ TextWidget.Cast(m_wRoot.FindAnyWidget("BoundaryStatus")).GetText();
	}
}
