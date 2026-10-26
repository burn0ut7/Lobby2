[BaseContainerProps(), BaseContainerCustomStringTitleField("Action (choose a type)")]
class GC_MissionAction
{
	[Attribute("0", UIWidgets.EditBox, "Delay after event starts, in seconds. Independent of other actions. Once queued, conditions are not checked again. Ending the mission or disabling the event cancels pending actions.", params: "0 86400 0.1")]
	float m_fDelayAfterEventStarts;

	//------------------------------------------------------------------------------------------------
	bool Validate(GC_LobbyMissionContext context, out string error)
	{
		error = "Choose an action type.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	bool Execute(GC_LobbyMissionContext context)
	{
		return false;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("End Mission")]
class GC_EndMissionAction : GC_MissionAction
{
	[Attribute("", UIWidgets.ComboBox, "Winning faction key. Ends the mission with a faction victory through Lobby2's normal completion path.", enums: {ParamEnum("", "", "rst-choices:1:GC_MissionFactionChoices")})]
	protected string m_sWinningFaction;
	[Attribute(SCR_Enum.GetDefault(EGameOverTypes.EDITOR_FACTION_VICTORY), UIWidgets.ComboBox, "Mission outcome associated with this faction.", enums: ParamEnumArray.FromEnum(EGameOverTypes))]
	EGameOverTypes m_eEnding;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (context.IsKnownFaction(m_sWinningFaction))
			return true;
		error = "End Mission: unknown winning faction '" + m_sWinningFaction + "'.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.EndMission(m_sWinningFaction, m_eEnding);
	}
}
