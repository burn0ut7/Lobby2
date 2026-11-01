#ifdef WORKBENCH
//! The editor's posed character cannot play gameplay item gestures. Keep native inventory transfers usable.
modded class SCR_InventoryStorageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override bool IsAnimationReady()
	{
		if (GC_WorkbenchItemBrowser.IsActive(GetOwner()))
			return true;
		return super.IsAnimationReady();
	}
}
#endif
