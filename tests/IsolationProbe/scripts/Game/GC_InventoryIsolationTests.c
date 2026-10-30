#ifdef WORKBENCH
//! Same native inventory journey with and without Lobby2 loaded.
modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string OpenInventory()
	{
		string possession = PossessCharacter();
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		if (!character)
			return possession;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return "missing inventory";
		manager.OpenInventory();
		return "native inventory requested";
	}

	//------------------------------------------------------------------------------------------------
	string InventorySnapshot()
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.GetInventoryMenu();
		if (!menu)
			return "closed";
		Widget root = menu.GetRootWidget();
		string result = "menu=" + menu.Type();
		result += string.Format(" customPanel=%1", root.FindAnyWidget("GCCatalogue") != null);
		array<string> names = {"StorageLootSlot", "OpenedStorages", "StorageGrid", "playerRender"};
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("\n%1 visible=%2 x=%3 y=%4 w=%5 h=%6", name, widget.IsVisible(), x, y, width, height);
		}
		return result;
	}
}
#endif
