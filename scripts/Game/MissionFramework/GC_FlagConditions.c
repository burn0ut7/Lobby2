[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission Flag Condition")]
class GC_MissionFlagCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag to check. Use its exact name from Mission Flags.")]
	string m_sFlagName;
	[Attribute("0", UIWidgets.ComboBox, "Check the flag's remembered result. True means it is active. Its own conditions control activation. A repeatable flag is true during its check cycle, then resets to false.", enums: {ParamEnum("Is true", "0"), ParamEnum("Is false", "1")})]
	int m_iMatch;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty() || m_iMatch < 0 || m_iMatch > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 1;
		check.m_sFlagName = m_sFlagName;
		check.m_bReverseAnswer = m_iMatch == 1;
		return check;
	}
}
