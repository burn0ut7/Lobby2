//! Shared serialized type for the condition-object list. Incomplete entries fail closed.
[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Choose a condition")]
class GC_MissionCondition
{
	//------------------------------------------------------------------------------------------------
	GC_RuleCondition BuildCheck(int depth = 0)
	{
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static GC_RuleCondition BuildGroup(array<ref GC_MissionCondition> items, GC_ERuleCheck kind, int number, int depth)
	{
		if (depth > 16 || (items && items.Count() > 256))
			return null;
		GC_RuleCondition group = new GC_RuleCondition();
		group.m_eGroupNeeds = kind;
		group.m_iHowMany = number;
		group.m_aChecks = {};
		if (items)
		{
			foreach (GC_MissionCondition item : items)
			{
				if (!item)
					return null;
				GC_RuleCondition check = item.BuildCheck(depth + 1);
				if (!check)
					return null;
				group.m_aChecks.Insert(check);
			}
		}
		if (!group.IsValid(depth))
			return null;
		return group;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("All are true (AND)")]
class GC_AllConditions : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "Every condition must be true. Add at least one condition. Conditions can contain other groups.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.ALL, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("At least one is true (OR)")]
class GC_AnyCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "At least one condition must be true. Add at least one condition. Conditions can contain other groups.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.ANY, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("None are true (NOR)")]
class GC_NoneConditions : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "Every condition must be false. With one condition, this reverses its answer. Add at least one condition. Conditions can contain other groups.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.NONE, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("At least one is false (NAND)")]
class GC_NotAllConditions : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "This passes unless every condition is true. Add at least one condition. Conditions can contain other groups.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.NOT_ALL, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("True or false value")]
class GC_BooleanCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.CheckBox, "Checked means true; unchecked means false. This is a fixed logic value, not a mission flag or a live game check.")]
	bool m_bValue;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_bValue;
		return check;
	}
}
