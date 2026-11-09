[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission Event Condition")]
class GC_MissionEventCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The exact unique name of a regular mission event or random event group. Checks its result without running its actions or checking its conditions again.")]
	string m_sEventName;
	[Attribute("0", UIWidgets.ComboBox, "True means the named event ran. Once events remember true. Every time events are true for their check cycle, then reset to false. A random group is true when it makes a selection. Missing or duplicate event names never pass, including Is false.", enums: {ParamEnum("Is true", "0"), ParamEnum("Is false", "1")})]
	int m_iMatch;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sEventName.IsEmpty() || m_iMatch < 0 || m_iMatch > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 5;
		check.m_sEventName = m_sEventName;
		check.m_bReverseAnswer = m_iMatch == 1;
		return check;
	}
}
