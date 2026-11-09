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

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag Call Count")]
class GC_FlagCallCountCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag whose remembered call count to check. Use its exact name from Mission Flags.")]
	string m_sFlagName;
	[Attribute("1", UIWidgets.ComboBox, "Compare the remembered count with the number below.", enums: {ParamEnum("Greater than", "0"), ParamEnum("Less than", "1"), ParamEnum("Equal to", "2")})]
	int m_iComparison;
	[Attribute("5", UIWidgets.SpinBox, "The number of accepted calls to compare against. On the flag itself, Less than 5 allows five calls. Blocked calls do not count.", "0 1000000 1")]
	int m_iCount;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty() || m_iComparison < 0 || m_iComparison > 2 || m_iCount < 0 || m_iCount > 1000000)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 4;
		check.m_sFlagName = m_sFlagName;
		check.m_iFlagComparison = m_iComparison;
		check.m_iFlagCount = m_iCount;
		return check;
	}
}
