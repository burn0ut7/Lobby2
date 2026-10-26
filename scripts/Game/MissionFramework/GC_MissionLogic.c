enum GC_EMissionGroupMatch
{
	ALL,
	ANY,
	AT_LEAST,
	AT_MOST,
	EXACTLY,
	ALL_SAME,
	LESS_THAN,
	GREATER_THAN,
	NOT_EQUAL
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition Group")]
class GC_ConditionGroup : GC_MissionCondition
{
	[Attribute("0", UIWidgets.ComboBox, "How many conditions must be true? All Same means all true or all false.", enums: {ParamEnum("All", "0"), ParamEnum("Any", "1"), ParamEnum("Greater than (>)", "7"), ParamEnum("Greater than or equal (>=)", "2"), ParamEnum("Less than (<)", "6"), ParamEnum("Less than or equal (<=)", "3"), ParamEnum("Exactly equal (==)", "4"), ParamEnum("Not equal (!=)", "8"), ParamEnum("All same", "5")})]
	GC_EMissionGroupMatch m_eMatch;
	[Attribute("1", UIWidgets.EditBox, "Number of true conditions used by the numeric comparisons.", params: "0 inf 1")]
	int m_iNumber;
	[Attribute("", UIWidgets.Object, "Conditions in this group. Groups can contain other groups.")]
	ref array<ref GC_MissionCondition> m_aConditions;

	//------------------------------------------------------------------------------------------------
	override void Reset(GC_LobbyMissionContext context)
	{
		super.Reset(context);
		foreach (GC_MissionCondition condition : m_aConditions)
			condition.Reset(context);
	}

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aConditions || m_aConditions.IsEmpty() || m_eMatch < 0 || m_eMatch > GC_EMissionGroupMatch.NOT_EQUAL || m_iNumber < 0)
		{
			error = "Condition Group: choose a rule and add conditions.";
			return false;
		}
		foreach (GC_MissionCondition condition : m_aConditions)
		{
			if (!condition || !condition.Validate(context, error))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		int count;
		foreach (GC_MissionCondition condition : m_aConditions)
		{
			if (condition.Evaluate(context))
				count++;
		}
		switch (m_eMatch)
		{
			case GC_EMissionGroupMatch.ALL: return count == m_aConditions.Count();
			case GC_EMissionGroupMatch.ANY: return count > 0;
			case GC_EMissionGroupMatch.AT_LEAST: return count >= m_iNumber;
			case GC_EMissionGroupMatch.AT_MOST: return count <= m_iNumber;
			case GC_EMissionGroupMatch.EXACTLY: return count == m_iNumber;
			case GC_EMissionGroupMatch.LESS_THAN: return count < m_iNumber;
			case GC_EMissionGroupMatch.GREATER_THAN: return count > m_iNumber;
			case GC_EMissionGroupMatch.NOT_EQUAL: return count != m_iNumber;
			case GC_EMissionGroupMatch.ALL_SAME: return count == 0 || count == m_aConditions.Count();
		}
		return false;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Always")]
class GC_AlwaysCondition : GC_MissionCondition
{
	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionFlag
{
	[Attribute("", UIWidgets.EditBox, "Unique flag name, for example Radio destroyed.")]
	string m_sName;
	[Attribute("0", UIWidgets.CheckBox, "Starting value for a manually controlled flag.")]
	bool m_bInitiallySet;
	[Attribute("0", UIWidgets.CheckBox, "Randomize the starting value instead of using Initially Set.", category: "Advanced")]
	bool m_bRandomizeInitially;
	[Attribute("50", UIWidgets.EditBox, "Chance to start set when Randomize Initially is enabled.", params: "0 100 1", category: "Advanced")]
	float m_fInitialChance;
	[Attribute("", UIWidgets.Object, "Optional automatic condition. Empty: actions and interactions control this flag. Assigned: the condition updates it every evaluation.")]
	ref GC_MissionCondition m_Condition;
	[Attribute("0", UIWidgets.CheckBox, "For an automatic flag, keep it set after the condition first becomes true.")]
	bool m_bKeepSet;
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag Is Set")]
class GC_FlagCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditComboBox, "Choose a flag or type its name, even before creating its source. Unset or not yet created is false. Use Invert to check that it is not set.", enums: {ParamEnum("", "", "rst-choices:1:GC_MissionFlagChoices")})]
	string m_sFlag;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sFlag.IsEmpty())
			return true;
		error = "Enter a flag name: " + m_sFlag;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		return context.IsFlagSet(m_sFlag);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition Held For")]
class GC_HeldCondition : GC_MissionCondition
{
	[Attribute("10", UIWidgets.EditBox, "Seconds the condition must remain true. A false result resets the timer.", params: "0 86400 0.1")]
	float m_fSeconds;
	[Attribute("", UIWidgets.Object, "The condition to hold. Use Condition Group for several checks.")]
	ref GC_MissionCondition m_Condition;

	//------------------------------------------------------------------------------------------------
	override void Reset(GC_LobbyMissionContext context)
	{
		super.Reset(context);
		m_Condition.Reset(context);
	}

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_Condition || m_fSeconds < 0 || m_fSeconds > 86400)
		{
			error = "Condition Held For: add a condition and a duration from 0 to 86400 seconds.";
			return false;
		}
		return m_Condition.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		return context.HeldFor(this, m_Condition.Evaluate(context), m_fSeconds);
	}
}
