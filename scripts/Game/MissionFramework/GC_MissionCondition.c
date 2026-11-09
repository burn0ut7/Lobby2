[BaseContainerProps(visible: false, insertable: false)]
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

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag condition")]
class GC_FlagCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "The flag to check. Use the same name as the action that changes it.")]
	string m_sFlagName;
	[Attribute("1", UIWidgets.ComboBox, "Only allow this rule while the flag has this setting. A flag that has never been changed is Off.", enums: {ParamEnum("On", "1"), ParamEnum("Off", "0")})]
	int m_iMustBe;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sFlagName.IsEmpty() || m_iMustBe < 0 || m_iMustBe > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 1;
		check.m_sFlagName = m_sFlagName;
		check.m_bReverseAnswer = m_iMustBe == 0;
		return check;
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Test condition")]
class GC_TestCondition : GC_MissionCondition
{
	[Attribute("1", UIWidgets.ComboBox, "For testing only. Yes allows the rule; No stops it.", enums: {ParamEnum("Yes", "1"), ParamEnum("No", "0")})]
	int m_iAnswer;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_iAnswer < 0 || m_iAnswer > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_iAnswer == 1;
		return check;
	}
}

[BaseContainerProps(visible: false, insertable: false)]
class GC_CheckGroup : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "Add the requirements for this combination.")]
	ref array<ref GC_MissionCondition> m_aRequirements;
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("All conditions")]
class GC_CheckAll : GC_CheckGroup
{
	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.ALL, 0, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Any condition")]
class GC_CheckAny : GC_CheckGroup
{
	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.ANY, 0, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("No conditions")]
class GC_CheckNone : GC_CheckGroup
{
	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.NONE, 0, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("All conditions agree")]
class GC_CheckSame : GC_CheckGroup
{
	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.ALL_THE_SAME, 0, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Exactly this many")]
class GC_CheckExactly : GC_CheckGroup
{
	[Attribute("1", UIWidgets.SpinBox, "How many requirements must be met. Zero is allowed.", "0 256 1")]
	int m_iNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.EXACTLY, m_iNumber, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("At least this many")]
class GC_CheckAtLeast : GC_CheckGroup
{
	[Attribute("1", UIWidgets.SpinBox, "How many requirements must be met. Zero is allowed.", "0 256 1")]
	int m_iNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.AT_LEAST, m_iNumber, depth);
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("At most this many")]
class GC_CheckAtMost : GC_CheckGroup
{
	[Attribute("1", UIWidgets.SpinBox, "How many requirements must be met. Zero is allowed.", "0 256 1")]
	int m_iNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		return BuildGroup(m_aRequirements, GC_ERuleCheck.AT_MOST, m_iNumber, depth);
	}
}
