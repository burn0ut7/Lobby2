[BaseContainerProps(), BaseContainerCustomStringTitleField("Custom Script Action")]
class GC_CustomScriptAction : GC_ActionBase
{
	[Attribute("", UIWidgets.Object, "Select your GC_MissionScript subclass. Runs once when this action executes on the server. Run(manager) returns true for success or false for rejection. Event settings control repetition and delay.")]
	ref GC_MissionScript m_Script;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.CUSTOM_SCRIPT;
		action.m_Script = m_Script;
		return action;
	}
}
