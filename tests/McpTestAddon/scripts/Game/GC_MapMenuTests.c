//! Optional checks use production callbacks; they do not emulate physical keys.
#ifdef WORKBENCH
modded class SCR_InputButtonComponent
{
	void GC_TestMapKey()
	{
		OnButtonPressed(1, EActionTrigger.DOWN);
	}
}

modded class GC_Lobby_SpectatorMenu
{
	void GC_TestMapToggle()
	{
		OnToggleSpectatorMap();
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapUI(string action)
	{
		if (GetManager() != this)
			return "UNAVAILABLE Lobby2";
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "UNAVAILABLE menu";
		Widget root = menu.GetRootWidget();
		Widget briefing = root.FindAnyWidget("BriefingView");
		Widget toggle = root.FindAnyWidget("BriefingToggle");
		if (action == "briefing-key")
		{
			if (!GC_Lobby_BriefingMenu.Cast(menu) || !toggle)
				return "UNAVAILABLE briefing";
			SCR_InputButtonComponent.Cast(toggle.FindHandler(SCR_InputButtonComponent)).GC_TestMapKey();
		}
		else if (action == "spectator-toggle")
		{
			GC_Lobby_SpectatorMenu spectator = GC_Lobby_SpectatorMenu.Cast(menu);
			if (!spectator)
				return "UNAVAILABLE spectator";
			spectator.GC_TestMapToggle();
		}
		else if (action == "tool-focus")
		{
			SCR_MapToolMenuUI toolMenu = SCR_MapToolMenuUI.Cast(SCR_MapEntity.GetMapInstance().GetMapUIComponent(SCR_MapToolMenuUI));
			if (!toolMenu)
				return "UNAVAILABLE tool menu";
			toolMenu.OnMouseEnter(root.FindAnyWidget("ToolMenu"), 0, 0);
		}
		else if (action != "snapshot")
			return "REJECTED action";

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.GetMapConfig())
			return "UNAVAILABLE map config";
		Widget toolbar = root.FindAnyWidget("ToolMenuHoriz");
		int buttons;
		if (toolbar)
		{
			Widget child = toolbar.GetChildren();
			while (child)
			{
				buttons++;
				child = child.GetSibling();
			}
		}
		array<string> keys = {};
		array<BaseContainer> filters = {};
		GetGame().GetInputManager().GetActionKeybinding("GC_ToggleBriefing", keys, filters);
		string keyNames;
		foreach (string key : keys)
			keyNames += key + " ";
		return string.Format("menu=%1 mapOpen=%2 mode=%3 tools=%4 drawing=%5 toolbarVisible=%6 buttons=%7 briefingVisible=%8 toggleVisible=%9",
			menu, mapEntity.IsOpen(), mapEntity.GetMapConfig().MapEntityMode,
			mapEntity.GetMapUIComponent(SCR_MapToolMenuUI) != null,
			mapEntity.GetMapUIComponent(SCR_MapDrawingUI) != null,
			toolbar && toolbar.IsVisibleInHierarchy(), buttons,
			briefing && briefing.IsVisibleInHierarchy(), toggle && toggle.IsVisibleInHierarchy())
			+ string.Format(" keys=%1 standardMap=%2", keyNames,
			GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.MapMenu) != null);
	}
}
#endif
