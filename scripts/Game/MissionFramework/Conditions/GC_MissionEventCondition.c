[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission Event Condition")]
class GC_MissionEventCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "The exact unique name of a regular mission event or random event group. Checks its result without running its actions or checking its conditions again.", enums: {ParamEnum("", "", "rst-choices:1:GC_MissionEventChoices")})]
	string m_sEventName;
	[Attribute("0", UIWidgets.CheckBox, "Checked: require the event result to be false. Unchecked: require true. A non-repeatable event remembers true after running; a repeatable event resets after its check cycle. Missing or duplicate names never pass.")]
	bool m_bRequireFalse;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sEventName.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 5;
		check.m_sEventName = m_sEventName;
		check.m_bReverseAnswer = m_bRequireFalse;
		return check;
	}
}
