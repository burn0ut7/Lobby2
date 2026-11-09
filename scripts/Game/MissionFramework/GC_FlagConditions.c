[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag State")]
class GC_FlagStateCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag to check. Use its exact name from Mission Flags.")]
	string m_sFlagName;
	[Attribute("0", UIWidgets.ComboBox, "On means the flag is active. A repeatable flag stays On only while its call is being handled, then resets to Off.", enums: {ParamEnum("Is On", "0"), ParamEnum("Is Off", "1")})]
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
