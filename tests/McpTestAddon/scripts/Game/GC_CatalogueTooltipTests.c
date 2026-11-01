#ifdef WORKBENCH
modded class SCR_InputButtonComponent
{
	//! Test native action dispatch, not physical keyboard input.
	void GC_TestPress()
	{
		OnButtonPressed(1, EActionTrigger.DOWN);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCatalogueHover(bool enter)
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		Widget root = menu.GetRootWidget();
		if (enter)
			menu.GC_TestMouse(root.FindAnyWidget("Results").GetChildren().FindAnyWidget("SelectItem"));
		else
			WidgetManager.ReportMouse(1, 1, root);
		return "mouse reported";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFilesKey()
	{
		Widget root = SCR_InventoryMenuUI.GetInventoryMenu().GetRootWidget();
		Widget widget = root.FindAnyWidget("CatalogueFiles");
		if (!widget)
			return "files button absent";
		SCR_InputButtonComponent.Cast(widget.FindHandler(SCR_InputButtonComponent)).GC_TestPress();
		return "action dispatched";
	}
}
#endif
