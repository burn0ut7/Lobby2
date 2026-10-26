[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition (choose a type)")]
class GC_MissionCondition
{
	[Attribute("0", UIWidgets.CheckBox, "Reverse this condition: true becomes false and false becomes true.", category: "Advanced")]
	bool m_bInvert;

	//------------------------------------------------------------------------------------------------
	void Reset(GC_LobbyMissionContext context)
	{
		context.ResetCondition(this);
	}

	//------------------------------------------------------------------------------------------------
	bool Evaluate(GC_LobbyMissionContext context)
	{
		bool met = IsMet(context);
		if (m_bInvert)
			return !met;
		return met;
	}

	//------------------------------------------------------------------------------------------------
	bool Validate(GC_LobbyMissionContext context, out string error)
	{
		error = "Choose a condition type.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	bool IsMet(GC_LobbyMissionContext context)
	{
		return false;
	}
}

enum GC_EMissionCountComparison
{
	AT_MOST = 0,
	AT_LEAST = 1,
	EXACTLY = 2,
	LESS_THAN = 3,
	GREATER_THAN = 4,
	NOT_EQUAL = 5
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Player Casualties (%)")]
class GC_PlayerCasualtiesCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.ComboBox, "Faction with playable group slots in this mission.", enums: {ParamEnum("", "", "rst-choices:1:GC_PlayableFactionChoices")})]
	string m_sFaction;
	[Attribute("1", UIWidgets.ComboBox, "Compare the percentage of players lost.", enums: {ParamEnum("Greater than (>)", "4"), ParamEnum("Greater than or equal (>=)", "1"), ParamEnum("Less than (<)", "3"), ParamEnum("Less than or equal (<=)", "0"), ParamEnum("Exactly equal (==)", "2"), ParamEnum("Not equal (!=)", "5")})]
	GC_EMissionCountComparison m_eComparison;
	[Attribute("75", UIWidgets.EditBox, "Percentage lost compared with the highest simultaneous living-player count during GAME. 75 means three quarters lost. Disconnects count as losses; respawns can lower the percentage. Unused factions never match.", params: "0 100 1")]
	float m_fPercentage;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!context.IsKnownFaction(m_sFaction))
			error = "Player Casualties: choose a known faction.";
		else if (m_fPercentage < 0 || m_fPercentage > 100 || m_eComparison < 0 || m_eComparison > GC_EMissionCountComparison.NOT_EQUAL)
			error = "Player Casualties: choose a comparison and a percentage from 0 to 100.";
		else
			return true;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		float percentage;
		return context.TryGetPlayerCasualties(m_sFaction, percentage)
			&& GC_MissionCompare.Number(percentage, m_eComparison, m_fPercentage);
	}
}

//! Comparisons shared by percentage, area and nested-condition checks.
class GC_MissionCompare
{
	//------------------------------------------------------------------------------------------------
	static bool Number(float value, GC_EMissionCountComparison comparison, float threshold)
	{
		if (comparison == GC_EMissionCountComparison.AT_MOST)
			return value <= threshold;
		if (comparison == GC_EMissionCountComparison.AT_LEAST)
			return value >= threshold;
		if (comparison == GC_EMissionCountComparison.EXACTLY)
			return value == threshold;
		if (comparison == GC_EMissionCountComparison.LESS_THAN)
			return value < threshold;
		if (comparison == GC_EMissionCountComparison.GREATER_THAN)
			return value > threshold;
		if (comparison == GC_EMissionCountComparison.NOT_EQUAL)
			return value != threshold;
		return false;
	}
}
