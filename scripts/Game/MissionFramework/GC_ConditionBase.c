//! Required shared base for all authored conditions. Do not create this type directly.
[BaseContainerProps(description: "BASE TYPE - Do not use directly. This shared base lets the Conditions list contain different condition types. It does not perform a check. Choose Mission Flag Condition, Mission Event Condition, Game Mode State, Condition Group or Condition Count instead. Replace any base entry before using the event or flag.", visible: false, insertable: false), BaseContainerCustomStringTitleField("Do not use")]
class GC_ConditionBase
{
	//------------------------------------------------------------------------------------------------
	GC_RuleCondition BuildCheck(int depth = 0)
	{
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static GC_RuleCondition BuildGroup(array<ref GC_ConditionBase> items, GC_ERuleCheck kind, int number, int depth)
	{
		if (depth > 16 || (items && items.Count() > 256))
			return null;
		GC_RuleCondition group = new GC_RuleCondition();
		group.m_eGroupNeeds = kind;
		group.m_iHowMany = number;
		group.m_aChecks = {};
		if (items)
		{
			foreach (GC_ConditionBase item : items)
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

//! Combine child answers without a numeric threshold.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition Group")]
class GC_ConditionGroup : GC_ConditionBase
{
	[Attribute("0", UIWidgets.ComboBox, "All: every condition is true. Any: at least one is true. None: every condition is false. Not all: at least one is false.", enums: {ParamEnum("All are true (AND)", "0"), ParamEnum("Any are true (OR)", "1"), ParamEnum("None are true (NOR)", "2"), ParamEnum("Not all are true (NAND)", "3")})]
	int m_iMatch;

	[Attribute("", UIWidgets.Object, "Conditions to combine. Each entry can contain another group or a Condition Count. Add at least one condition. None with one child reverses that child's answer.")]
	ref array<ref GC_ConditionBase> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;

		GC_ERuleCheck operation;
		switch (m_iMatch)
		{
			case 0:
				operation = GC_ERuleCheck.ALL;
				break;
			case 1:
				operation = GC_ERuleCheck.ANY;
				break;
			case 2:
				operation = GC_ERuleCheck.NONE;
				break;
			case 3:
				operation = GC_ERuleCheck.NOT_ALL;
				break;
			default:
				return null;
		}
		return BuildGroup(m_aConditions, operation, 0, depth);
	}
}
