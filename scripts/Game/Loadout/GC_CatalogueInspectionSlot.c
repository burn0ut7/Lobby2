#ifdef WORKBENCH
//! Keep the native inspection point; route its compatible-items view to the opted-in catalogue.
modded class SCR_InventorySlotGearInspectionUI
{
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (button == 0 && controller && GC_WorkbenchItemBrowser.IsActive(controller.GetControlledEntity()))
		{
			SCR_InventoryMenuUI.GetInventoryMenu().GC_SelectInspectionSlot(this);
			Select(true);
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
}
#endif
