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
	AT_MOST,
	AT_LEAST,
	EXACTLY
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Player Casualties (%)")]
class GC_PlayerCasualtiesCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "Faction key, for example US.")]
	string m_sFaction;
	[Attribute("1", UIWidgets.ComboBox, "Compare the percentage of players lost.", enums: ParamEnumArray.FromEnum(GC_EMissionCountComparison))]
	GC_EMissionCountComparison m_eComparison;
	[Attribute("75", UIWidgets.EditBox, "Percentage lost compared with the highest simultaneous living-player count during GAME. 75 means three quarters lost. Disconnects count as losses; respawns can lower the percentage. Unused factions never match.", params: "0 100 1")]
	float m_fPercentage;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!context.IsKnownFaction(m_sFaction))
			error = "Player Casualties: choose a known faction.";
		else if (m_fPercentage < 0 || m_fPercentage > 100 || m_eComparison < 0 || m_eComparison > 2)
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
		return Math.AbsFloat(value - threshold) < 0.0001;
	}
}
