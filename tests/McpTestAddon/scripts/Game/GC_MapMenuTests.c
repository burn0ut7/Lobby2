//! Optional checks use production callbacks; they do not emulate physical keys.
#ifdef WORKBENCH
modded class SCR_MapMarkerManagerComponent
{
	//------------------------------------------------------------------------------------------------
	int GC_TestMapMarkerCount()
	{
		return GetStaticMarkers().Count() + GetDisabledMarkers().Count();
	}
}

modded class SCR_MapRadialDisplay
{
	//------------------------------------------------------------------------------------------------
	string GC_TestDisplayState()
	{
		float x, y, width, height;
		m_wBase.GetScreenPos(x, y);
		m_wBase.GetScreenSize(width, height);
		return string.Format("shown=%1 entries=%2 segments=%3 baseVisible=%4 baseOpacity=%5 baseRect=%6,%7,%8,%9", m_bShown, m_aEntries.Count(), m_aSegments.Count(), m_wBase.IsVisibleInHierarchy(), m_wBase.GetOpacity(), x, y, width, height);
	}
}

modded class SCR_MapRadialUI
{
	//------------------------------------------------------------------------------------------------
	string GC_TestContextMenu(string action)
	{
		if (action == "open")
			OnInputMenuOpen(1, EActionTrigger.UP);
		else if (action == "close" && m_RadialMenu)
			m_RadialMenu.Close();
		else if (action == "category" && m_RadialMenu && m_RadialMenu.IsOpened() && m_RadialMenu.GetEntryCount() > 0)
			m_RadialMenu.PerformEntry(m_RadialMenu.GetEntries()[0]);
		else if (action == "custom" && m_RadialMenu && m_RadialMenu.IsOpened())
		{
			foreach (SCR_SelectionMenuEntry entry : m_RadialMenu.GetEntries())
			{
				SCR_MapMarkerMenuEntry markerEntry = SCR_MapMarkerMenuEntry.Cast(entry);
				if (markerEntry && markerEntry.GetMarkerType() == SCR_EMapMarkerType.PLACED_CUSTOM)
				{
					m_RadialMenu.PerformEntry(entry);
					break;
				}
			}
		}
		else if (action != "snapshot")
			return "REJECTED action";

		string result = string.Format("cursor=%1 opened=%2 display=%3", m_CursorModule.GetCursorState(), m_RadialMenu && m_RadialMenu.IsOpened(), m_Display != null);
		if (m_Display)
		{
			result += " " + m_Display.GC_TestDisplayState();
			Widget widget = m_Display.GetRootWidget();
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format(" rect=%1,%2,%3,%4", x, y, width, height);
			while (widget)
			{
				result += string.Format(" | %1 visible=%2 opacity=%3 z=%4", widget.GetName(), widget.IsVisible(), widget.GetOpacity(), widget.GetZOrder());
				widget = widget.GetParent();
			}
		}
		return result;
	}
}

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
	string GC_TestMapHit(string name)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget target = menu.GetRootWidget().FindAnyWidget(name);
		if (!target)
			return "UNAVAILABLE widget";
		float x, y, width, height;
		target.GetScreenPos(x, y);
		target.GetScreenSize(width, height);
		array<Widget> hits = {};
		WidgetManager.TraceWidgets(x + width * 0.5, y + height * 0.5, GetGame().GetWorkspace(), hits);
		string result = string.Format("target=%1 rect=%2,%3,%4,%5 hits=%6", name, x, y, width, height, hits.Count());
		InputManager input = GetGame().GetInputManager();
		result += string.Format(" mapSelect=%1 contextMenu=%2 menuSelect=%3 toggle=%4 toggleContext=%5 mapContext=%6", input.IsActionActive("MapSelect"), input.IsActionActive("MapContextualMenu"), input.IsActionActive("MenuSelect"), input.IsActionActive("GC_ToggleBriefing"), input.IsContextActive("GC_BriefingToggle"), input.IsContextActive("GC_Lobby_Preview"));
		Widget ancestor = menu.GetRootWidget();
		while (ancestor)
		{
			result += string.Format(" ancestor=%1:%2", ancestor.GetName(), ancestor.GetZOrder());
			ancestor = ancestor.GetParent();
		}
		foreach (Widget hit : hits)
		{
			if (!(hit.GetFlags() & WidgetFlags.IGNORE_CURSOR))
				result += string.Format(" | pointerTarget=%1 enabled=%2", hit.GetName(), hit.IsEnabled());
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapContext(string action)
	{
		SCR_MapRadialUI radial = SCR_MapRadialUI.GetInstance();
		if (!radial)
			return "UNAVAILABLE radial menu";
		return radial.GC_TestContextMenu(action);
	}

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
		else if (action == "tool-leave")
		{
			SCR_MapToolMenuUI toolMenu = SCR_MapToolMenuUI.Cast(SCR_MapEntity.GetMapInstance().GetMapUIComponent(SCR_MapToolMenuUI));
			if (!toolMenu)
				return "UNAVAILABLE tool menu";
			toolMenu.OnMouseLeave(root.FindAnyWidget("ToolMenu"), root.FindAnyWidget("MapFrame"), 0, 0);
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
