//! One count comparison for child conditions or remembered mission-flag calls.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Count Condition")]
class GC_ConditionCount : GC_ConditionBase
{
	[Attribute("0", UIWidgets.CheckBox, "Checked: read the remembered count of Mission Flag Name, including Start Call Count and Flag Action edits. Unchecked: count true entries in Conditions. Only the selected source is used.")]
	bool m_bCountFlagCalls;

	[Attribute("0", UIWidgets.ComboBox, "Compare the chosen count with Required Count. Greater than 2 needs 3 or more; Less than 2 allows 0 or 1; Equal 2 needs exactly 2; Not equal 2 allows any count except 2.", enums: {ParamEnum("Greater than", "0"), ParamEnum("Less than", "1"), ParamEnum("Equal", "2"), ParamEnum("Not equal", "3")})]
	int m_iComparison;

	[Attribute("1", UIWidgets.SpinBox, "The number to compare against. For conditions, use 0 to 256. For flag calls, use 0 to 1000000. Less than 5 permits calls while the remembered count is below five.", "0 1000000 1")]
	int m_iRequiredCount;

	[Attribute("", UIWidgets.EditComboBox, "Only used when Count Flag Calls is checked. Enter the exact name from Mission Flags. To limit the flag containing this condition, enter that flag's name.", enums: {ParamEnum("", "", "rst-choices:1:GC_MissionFlagChoices")})]
	string m_sFlagName;

	[Attribute("", UIWidgets.Object, "Only used when Count Flag Calls is unchecked. Conditions to count. Each entry gives one true or false answer and may contain other groups. Add at least one condition.")]
	ref array<ref GC_ConditionBase> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_iComparison < 0 || m_iComparison > 3 || m_iRequiredCount < 0 || m_iRequiredCount > 1000000)
			return null;
		if (m_bCountFlagCalls)
		{
			if (m_sFlagName.IsEmpty())
				return null;
			GC_RuleCondition flagCheck = new GC_RuleCondition();
			flagCheck.m_iCheck = 4;
			flagCheck.m_sFlagName = m_sFlagName;
			flagCheck.m_iFlagComparison = m_iComparison;
			flagCheck.m_iFlagCount = m_iRequiredCount;
			if (m_iComparison == 3)
			{
				flagCheck.m_iFlagComparison = 2;
				flagCheck.m_bReverseAnswer = true;
			}
			return flagCheck;
		}
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount > 256)
			return null;

		GC_ERuleCheck operation;
		switch (m_iComparison)
		{
			case 0:
				operation = GC_ERuleCheck.GREATER_THAN;
				break;
			case 1:
				operation = GC_ERuleCheck.LESS_THAN;
				break;
			case 2:
			case 3:
				operation = GC_ERuleCheck.EXACTLY;
				break;
			default:
				return null;
		}
		GC_RuleCondition check = BuildGroup(m_aConditions, operation, m_iRequiredCount, depth);
		if (check && m_iComparison == 3)
			check.m_bReverseAnswer = true;
		return check;
	}
}
