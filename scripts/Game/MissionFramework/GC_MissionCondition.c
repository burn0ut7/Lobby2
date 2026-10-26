[BaseContainerProps(), BaseContainerCustomStringTitleField("Condition (choose a type)")]
class GC_MissionCondition
{
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

[BaseContainerProps(), BaseContainerCustomStringTitleField("Living Players")]
class GC_LivingPlayersCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "Faction key. Counts connected living players, including unconscious players; excludes AI and spectators.")]
	protected string m_sFaction;
	[Attribute("0", UIWidgets.ComboBox, "Comparison with the number below.", enums: ParamEnumArray.FromEnum(GC_EMissionCountComparison))]
	protected GC_EMissionCountComparison m_eComparison;
	[Attribute("0", UIWidgets.EditBox, "Number of living players. Checking begins only after this faction has had a living player observed during the mission.", params: "0 inf 1")]
	protected int m_iNumber;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!context.IsKnownFaction(m_sFaction))
			error = "Living Players: unknown faction '" + m_sFaction + "'.";
		else if (m_iNumber < 0 || m_eComparison < GC_EMissionCountComparison.AT_MOST || m_eComparison > GC_EMissionCountComparison.EXACTLY)
			error = "Living Players: invalid comparison or negative number.";
		else
			return true;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		int count;
		if (!context.TryGetLivingPlayers(m_sFaction, count))
			return false;
		if (m_eComparison == GC_EMissionCountComparison.AT_MOST)
			return count <= m_iNumber;
		if (m_eComparison == GC_EMissionCountComparison.AT_LEAST)
			return count >= m_iNumber;
		return count == m_iNumber;
	}
}
