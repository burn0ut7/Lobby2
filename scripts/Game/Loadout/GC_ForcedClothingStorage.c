//! Native equipped-storage checks run before inventory mutations on authority and clients.
//! Pocket contents and unworn spare clothing keep their inherited behavior.
modded class SCR_CharacterInventoryStorageComponent
{
	//------------------------------------------------------------------------------------------------
	void GC_RefreshClothingPermission()
	{
		if (GC_LobbyManager.GetManager())
			UpdateUI();
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsClothingLocked(IEntity item)
	{
		if (!item || !GC_LobbyManager.GetManager())
			return false;
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetOwner());
		if (playerId <= 0)
			return false;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller && controller.GC_AreClothingChangesAllowed())
			return false;
		return GetClothFromArea(LoadoutJacketArea) == item || GetClothFromArea(LoadoutPantsArea) == item || GetClothFromArea(LoadoutBootsArea) == item;
	}

	//------------------------------------------------------------------------------------------------
	//! Keep native validation enabled. The engine runs the script permission after
	//! native checks when its cached override switch is false.
	override bool CanRemoveItem(IEntity item)
	{
		return !GC_IsClothingLocked(item) && super.CanRemoveItem(item);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanReplaceItem(IEntity nextItem, int slotID)
	{
		InventoryStorageSlot slot;
		if (slotID >= 0 && slotID < GetSlotsCount())
			slot = GetSlot(slotID);
		if (slot && GC_IsClothingLocked(slot.GetAttachedEntity()))
			return false;
		return super.CanReplaceItem(nextItem, slotID);
	}
}
