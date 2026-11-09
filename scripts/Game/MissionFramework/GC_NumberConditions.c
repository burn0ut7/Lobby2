//! Count satisfied child conditions. Each child may itself contain another group.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition Count")]
class GC_ConditionCount : GC_MissionCondition
{
	[Attribute("0", UIWidgets.ComboBox, "Compare the number of true conditions with Required Count. Greater than 2 needs 3 or more; Less than 2 allows 0 or 1; Equal to 2 needs exactly 2.", enums: {ParamEnum("Greater than (>)", "0"), ParamEnum("Less than (<)", "1"), ParamEnum("Equal to (=)", "2")})]
	int m_iComparison;

	[Attribute("1", UIWidgets.SpinBox, "The number to compare against. This counts true conditions, not people or entities.", "0 256 1")]
	int m_iRequiredCount;

	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry gives one true or false answer and may contain other groups. Add at least one condition.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
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
				operation = GC_ERuleCheck.EXACTLY;
				break;
			default:
				return null;
		}
		return BuildGroup(m_aConditions, operation, m_iRequiredCount, depth);
	}
}
