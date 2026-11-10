[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Entities Action")]
class GC_DeleteEntitiesAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditBox, "Exact names of entities to remove, including their attached children. Missing or already deleted names are skipped. This does not find entities by prefab or automatically find an AI group's members. The active game mode and its parents cannot be deleted.")]
	ref array<string> m_aEntities;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.DELETE_ENTITIES;
		action.m_aFlagNames = {};
		if (m_aEntities)
			action.m_aFlagNames.Copy(m_aEntities);
		if (!action.m_aFlagNames.IsEmpty())
			action.m_sTextOrName = action.m_aFlagNames[0];
		return action;
	}
}
