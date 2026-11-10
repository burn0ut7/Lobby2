[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission Flag Condition")]
class GC_MissionFlagCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditBox, "The flag to check. Use its exact name from Mission Flags.")]
	string m_sFlagName;
	[Attribute("0", UIWidgets.CheckBox, "Checked: require the flag to be false. Unchecked: require true. Zero successful calls means false; one or more means true, until Reset Flag clears the count. Reading this condition never calls the flag or rechecks its activation conditions.")]
	bool m_bRequireFalse;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 1;
		check.m_sFlagName = m_sFlagName;
		check.m_bReverseAnswer = m_bRequireFalse;
		return check;
	}
}
