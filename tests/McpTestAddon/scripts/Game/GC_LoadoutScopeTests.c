#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutScope(string field, string action, string value)
	{
		GC_LoadoutFilesMenu menu = GC_LoadoutFilesMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu));
		if (!menu)
			return "menu absent";
		Widget root = menu.GetRootWidget().FindAnyWidget(field);
		GC_LoadoutScopeSelector selector = GC_LoadoutScopeSelector.Cast(root.FindHandler(GC_LoadoutScopeSelector));
		if (action == "type")
		{
			EditBoxWidget input = EditBoxWidget.Cast(root.FindAnyWidget("FilterInput"));
			input.SetText(value);
			SCR_EventHandlerComponent.Cast(input.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(input);
		}
		if (action == "open" || action == "choose")
			selector.OpenList();
		if (action == "choose")
		{
			array<Widget> rows = {};
			selector.GetElementWidgets(rows);
			foreach (Widget row : rows)
			{
				SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
				if (TextWidget.Cast(row.FindAnyWidget("Text")).GetText() == value)
				{
					button.OnClick(row, 0, 0, 0);
					break;
				}
			}
		}
		if (action == "close")
			selector.CloseList();
		return menu.GC_TestScopeState();
	}
}

modded class GC_LoadoutFilesMenu
{
	//------------------------------------------------------------------------------------------------
	string GC_TestScopeState()
	{
		string report = "faction=" + m_Faction.GetText() + " team=" + m_Group.GetText();
		foreach (int index, SCR_ButtonBaseComponent row : m_aRows)
		{
			if (row.GetRootWidget().IsVisible())
				report += "\nvisible=" + m_aRowResources[index];
		}
		array<GC_LoadoutScopeSelector> selectors = {m_Faction, m_Group};
		foreach (GC_LoadoutScopeSelector selector : selectors)
		{
			report += "\n" + selector.GetRootWidget().GetName() + " open=" + selector.IsOpened();
			array<Widget> rows = {};
			selector.GetElementWidgets(rows);
			foreach (Widget row : rows)
			{
				if (row.IsVisible())
					report += " option=" + TextWidget.Cast(row.FindAnyWidget("Text")).GetText();
			}
		}
		return report;
	}
}
#endif
