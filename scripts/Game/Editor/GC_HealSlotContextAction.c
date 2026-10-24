[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class GC_HealSlotContextAction : SCR_SelectedEntitiesContextAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity)
			return false;
		GC_AdminSlotPlayerAction action = new GC_AdminSlotPlayerAction(GC_EAdminSlotAction.HEAL_OR_RESPAWN);
		return action.IsVisible(GC_ContextMenuContext.ForEntity(selectedEntity.GetOwner()));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity)
			return false;
		GC_AdminSlotPlayerAction action = new GC_AdminSlotPlayerAction(GC_EAdminSlotAction.HEAL_OR_RESPAWN);
		return action.CanExecute(GC_ContextMenuContext.ForEntity(selectedEntity.GetOwner()));
	}

	//------------------------------------------------------------------------------------------------
	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		if (!selectedEntity)
			return;
		// Run locally: the existing slot action sends the authenticated controller request.
		GC_AdminSlotPlayerAction action = new GC_AdminSlotPlayerAction(GC_EAdminSlotAction.HEAL_OR_RESPAWN);
		action.Execute(GC_ContextMenuContext.ForEntity(selectedEntity.GetOwner()));
	}
}
