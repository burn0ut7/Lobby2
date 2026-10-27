[EntityEditorProps(category: "GameScripted/Mission Framework", description: "Mission Event. Put condition and action entities beneath it in the hierarchy.")]
class GC_MissionEventEntityClass : GC_MissionAuthoringEntityClass
{
}

class GC_MissionEventEntity : GC_MissionAuthoringEntity
{
	[Attribute("", UIWidgets.EditBox, "Unique event name.")]
	string m_sName;
	[Attribute("0", UIWidgets.CheckBox, "Allow during preview, slotting and briefing. Casualty conditions still wait for GAME.")]
	bool m_bAllowBeforeGame;
	[Attribute("0", UIWidgets.ComboBox, "Match the child conditions.", enums: {ParamEnum("All", "0"), ParamEnum("Any", "1"), ParamEnum("None", "2")})]
	GC_EMissionConditionMatch m_eMatch;

	//------------------------------------------------------------------------------------------------
	GC_MissionEvent BuildEvent(out string error)
	{
		GC_MissionEvent definition = new GC_MissionEvent();
		definition.m_sName = m_sName;
		definition.m_bAllowBeforeGame = m_bAllowBeforeGame;
		definition.m_eMatch = m_eMatch;
		definition.m_aConditions = {};
		definition.m_aActions = {};
		if (!ReadChildren(definition.m_aConditions, definition.m_aActions, error))
			return null;
		return definition;
	}
}
