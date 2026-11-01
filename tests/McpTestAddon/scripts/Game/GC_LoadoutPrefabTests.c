#ifdef WORKBENCH
//! Explicit MCP tests of the production file buttons and native equipped contents.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutIcon(int index)
	{
		MenuBase menu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu);
		if (!menu)
			return "dialog absent";
		GC_LoadoutIconSelector selector = GC_LoadoutIconSelector.Cast(menu.GetRootWidget().FindAnyWidget("RoleSelector").FindHandler(GC_LoadoutIconSelector));
		selector.OpenList();
		if (index >= 0)
			selector.GC_TestChoose(index);
		GC_LoadoutMetadata icon = selector.GetSelectedIcon();
		return string.Format("count=%1 selected=%2 icon=%3", selector.GetNumItems(), selector.GetCurrentIndex(), icon.m_sIconName);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutMetadata(string prefab)
	{
		GC_LoadoutMetadata metadata = GC_LoadoutMetadata.Read(prefab);
		if (!metadata)
			return "metadata absent";
		return string.Format("name=%1 faction=%2 icon=%3 image=%4", metadata.m_sName, metadata.m_sFaction, metadata.m_sIconName, metadata.m_sIcon);
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutConfirm(string action)
	{
		SCR_ConfigurableDialogUi dialog = SCR_ConfigurableDialogUi.GetCurrentDialog();
		if (!dialog || dialog.GetDialogPreset().m_sTag != "overwrite")
			return "overwrite dialog absent";
		if (!action.IsEmpty())
		{
			SCR_InputButtonComponent button = dialog.FindButton(action);
			if (!button)
				return "button absent";
			button.GC_TestPress();
		}
		return "overwrite " + action;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutFlowState()
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		string actual;
		if (owner)
			actual = SCR_ResourceNameUtils.GetPrefabName(owner);
		MenuBase files = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu);
		string status;
		if (files)
			status = TextWidget.Cast(files.GetRootWidget().FindAnyWidget("Status")).GetText();
		return string.Format("editing=%1 actual=%2 active=%3 files=%4 gear=%5 status=%6", GC_WorkbenchItemBrowser.GetEditingPrefab(), actual, GC_WorkbenchItemBrowser.IsActive(owner), files != null, SCR_InventoryMenuUI.GetInventoryMenu() != null, status);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutMenu(string action, string faction, string group, string name)
	{
		GC_LoadoutFilesMenu menu = GC_LoadoutFilesMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu));
		if (!menu)
		{
			SCR_InventoryMenuUI inventory = SCR_InventoryMenuUI.GetInventoryMenu();
			if (!inventory)
				return "inventory absent";
			inventory.GC_ActionLoadoutFiles();
			menu = GC_LoadoutFilesMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu));
		}
		if (!menu)
			return "dialog absent";
		Widget root = menu.GetRootWidget();
		if (!faction.IsEmpty())
			EditBoxWidget.Cast(root.FindAnyWidget("Faction")).SetText(faction);
		if (!group.IsEmpty())
			EditBoxWidget.Cast(root.FindAnyWidget("Group")).SetText(group);
		if (!name.IsEmpty())
			EditBoxWidget.Cast(root.FindAnyWidget("Name")).SetText(name);
		if (!action.IsEmpty())
			SCR_ButtonTextComponent.GetButtonText(action, root).OnClick(root.FindAnyWidget(action), 0, 0, 0);
		return "editing=" + GC_WorkbenchItemBrowser.GetEditingPrefab();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutSnapshot()
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		if (!owner)
			return "no controlled entity";
		JsonSaveContext context = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(owner, context))
			return "capture failed";
		return context.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadoutChoose(string name)
	{
		MenuBase menu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu);
		if (!menu)
			return "dialog absent";
		Widget row = menu.GetRootWidget().FindAnyWidget("SavedRows").GetChildren();
		while (row)
		{
			TextWidget label = TextWidget.Cast(row.FindAnyWidget("Text"));
			if (label && label.GetText() == name)
			{
				SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent)).OnClick(row, 0, 0, 0);
				return "selected " + name;
			}
			row = row.GetSibling();
		}
		return "not found";
	}
}

modded class GC_LoadoutIconSelector
{
	void GC_TestChoose(int index)
	{
		if (index < 0 || index >= m_aElementWidgets.Count())
			return;
		Widget row = m_aElementWidgets[index];
		SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent)).OnClick(row, 0, 0, 0);
	}
}
#endif
