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

[BaseContainerProps(), BaseContainerCustomStringTitleField("All conditions (AND)")]
class GC_AllConditions : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "Every condition in this list must be true. Add at least one condition.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.ALL, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Any condition (OR)")]
class GC_AnyCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "At least one condition in this list must be true. Add at least one condition.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return BuildGroup(m_aConditions, GC_ERuleCheck.ANY, 0, depth);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Not condition (NOT)")]
class GC_NotCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "This condition must be false. For example, Not > Any means none of those conditions may be true.")]
	ref GC_MissionCondition m_Condition;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !m_Condition)
			return null;
		GC_RuleCondition check = m_Condition.BuildCheck(depth + 1);
		if (!check || !check.IsValid(depth + 1))
			return null;
		check.m_bReverseAnswer = !check.m_bReverseAnswer;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag is On condition")]
class GC_FlagOnCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag that must be On. Use the same name as the action that sets it. Flags are optional; use this condition only when needed.")]
	string m_sFlagName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 1;
		check.m_sFlagName = m_sFlagName;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag is Off condition")]
class GC_FlagOffCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag that must be Off. A flag with no starting value and no earlier change is Off.")]
	string m_sFlagName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 1;
		check.m_sFlagName = m_sFlagName;
		check.m_bReverseAnswer = true;
		return check;
	}
}
