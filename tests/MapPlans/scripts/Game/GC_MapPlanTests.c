//! Optional MCP fixture: calls production permissions, storage and map handlers.
modded class SCR_MapMarkerManagerComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapPlans(string action)
	{
		GC_MapPlans plans = GC_MapPlans.GetInstance();
		if (!plans)
			return "no-server-plans";
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		string output;
		if (action == "snapshot")
		{
			string result = plans.Execute(playerId, "save", "", output);
			return result + " | " + output;
		}
		if (action == "add")
		{
			SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
			marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			marker.SetWorldPos(1000, 1000);
			marker.SetCustomText("Map plan test");
			InsertStaticMarker(marker, false);
			return "requested-player-marker";
		}
		if (action == "save")
		{
			string result = plans.Execute(playerId, "save", "", output);
			bool written = GC_MapPlanData.WriteFile("$profile:Lobby2MapPlanTest.json", output);
			return result + " file=" + written.ToString();
		}
		if (action == "load")
		{
			string input;
			if (!GC_MapPlanData.ReadFile("$profile:Lobby2MapPlanTest.json", input))
				return "missing-test-file";
			return plans.Execute(playerId, "load", input, output);
		}
		if (action == "clear")
			return plans.Execute(playerId, "clear", "", output);
		if (action == "unauthorized")
			return plans.Execute(-1, "clear", "", output);
		if (action == "invalid")
			return plans.Execute(playerId, "load", "{}", output);
		if (action == "advance")
		{
			GC_LobbyManager manager = GC_LobbyManager.GetManager();
			SCR_EGameModeState phase = manager.GetLobbyPhase();
			SCR_EGameModeState next;
			if (phase == SCR_EGameModeState.PREGAME)
				next = SCR_EGameModeState.SLOTTING;
			else if (phase == SCR_EGameModeState.SLOTTING)
				next = SCR_EGameModeState.BRIEFING;
			else
				next = SCR_EGameModeState.GAME;
			manager.TryAdvanceLobbyPhase(playerId, next);
			return "phase=" + manager.GetLobbyPhase().ToString();
		}
		if (action == "ui")
		{
			GC_AdminToolsComponent.GC_TestOpenPlans();
			return "opened-admin-map-plans";
		}
		if (action == "close-ui")
		{
			GC_AdminToolsComponent.Close();
			return "closed-admin-map-plans";
		}
		if (action.StartsWith("ui-"))
			return GC_AdminToolsComponent.GC_TestPlanAction(action.Substring(3, action.Length() - 3));
		return "unknown-action";
	}
}

modded class GC_AdminToolsComponent
{
	//------------------------------------------------------------------------------------------------
	static string GC_TestPlanAction(string name)
	{
		if (!s_wPanel)
			return "no-admin-panel";
		GC_AdminToolsComponent panel = GC_AdminToolsComponent.Cast(s_wPanel.FindHandler(GC_AdminToolsComponent));
		return panel.m_MapPlans.GC_TestAction(name);
	}

	//------------------------------------------------------------------------------------------------
	static void GC_TestOpenPlans()
	{
		Open();
		if (!s_wPanel)
			return;
		GC_AdminToolsComponent panel = GC_AdminToolsComponent.Cast(s_wPanel.FindHandler(GC_AdminToolsComponent));
		panel.m_iTool = 6;
		panel.Refresh();
	}
}

modded class GC_AdminMapPlansComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAction(string name)
	{
		if (name != "status")
		{
			m_Name.SetText("MCP map test");
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
			if (!button)
				return "missing-button";
			button.m_OnClicked.Invoke(button);
		}
		return m_Status.GetText();
	}
}
