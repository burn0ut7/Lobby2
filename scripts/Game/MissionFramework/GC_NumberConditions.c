//! Compare how many child conditions are true against a required count. Each child may be another group.

[BaseContainerProps(), BaseContainerCustomStringTitleField("Exactly this many are true (=)")]
class GC_EqualCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "Exactly this many listed conditions must be true.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.EXACTLY, m_iRequiredCount, depth);
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("A different number are true (!=)")]
class GC_NotEqualCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "The number of true conditions must differ from this count.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.EXACTLY, m_iRequiredCount, depth);
		if (check)
			check.m_bReverseAnswer = true;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Fewer than this many are true (<)")]
class GC_LessThanCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "Fewer than this many listed conditions must be true. For 2, either 0 or 1 may be true.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.LESS_THAN, m_iRequiredCount, depth);
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("At most this many are true (<=)")]
class GC_LessOrEqualCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "No more than this many listed conditions may be true. For 2, either 0, 1 or 2 may be true.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.AT_MOST, m_iRequiredCount, depth);
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("More than this many are true (>)")]
class GC_GreaterThanCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "More than this many listed conditions must be true. For 2, at least 3 must be true.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.GREATER_THAN, m_iRequiredCount, depth);
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("At least this many are true (>=)")]
class GC_GreaterOrEqualCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.SpinBox, "At least this many listed conditions must be true. For 2 out of 3, either 2 or 3 must be true.", "0 256 1")]
	int m_iRequiredCount;
	[Attribute("", UIWidgets.Object, "Conditions to count. Each entry contributes one true or false answer, even when it contains another group. This counts satisfied conditions, not people or entities.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_iRequiredCount < 0 || m_iRequiredCount > 256)
			return null;
		GC_RuleCondition check = BuildGroup(m_aConditions, GC_ERuleCheck.AT_LEAST, m_iRequiredCount, depth);
		return check;
	}
}
