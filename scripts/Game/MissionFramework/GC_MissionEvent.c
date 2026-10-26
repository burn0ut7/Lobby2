enum GC_EMissionConditionMatch
{
	ALL_CONDITIONS,
	ANY_CONDITION
}

//! Authored definition only. Execution history and pending actions belong to the runtime.
[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionEvent
{
	[Attribute("", UIWidgets.EditBox, "Unique event name, for example BLUFOR defeated. Events run once, during the mission only.")]
	string m_sName;
	[Attribute("0", UIWidgets.CheckBox, "Allow this event during preview, slotting and briefing as well as GAME. Casualty conditions still wait for GAME.", category: "Advanced")]
	bool m_bAllowBeforeGame;
	[Attribute("0", UIWidgets.ComboBox, "When should this event start?", enums: ParamEnumArray.FromEnum(GC_EMissionConditionMatch))]
	GC_EMissionConditionMatch m_eMatch;
	[Attribute("", UIWidgets.Object, "Conditions checked together. At least one condition is required.")]
	ref array<ref GC_MissionCondition> m_aConditions;
	[Attribute("", UIWidgets.Object, "Actions to perform. Each delay is measured from this event's start.")]
	ref array<ref GC_MissionAction> m_aActions;

	//------------------------------------------------------------------------------------------------
	bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_sName.IsEmpty() || !m_aConditions || m_aConditions.IsEmpty() || !m_aActions || m_aActions.IsEmpty())
		{
			error = "Each event needs a name, at least one condition and at least one action.";
			return false;
		}
		if (m_eMatch < GC_EMissionConditionMatch.ALL_CONDITIONS || m_eMatch > GC_EMissionConditionMatch.ANY_CONDITION)
		{
			error = "Choose All conditions or Any condition.";
			return false;
		}
		foreach (GC_MissionCondition condition : m_aConditions)
		{
			if (!condition)
			{
				error = "Missing condition.";
				return false;
			}
			if (!condition.Validate(context, error))
				return false;
		}
		foreach (GC_MissionAction action : m_aActions)
		{
			if (!action || action.m_fDelayAfterEventStarts < 0 || action.m_fDelayAfterEventStarts > 86400)
			{
				error = "Missing action or delay outside 0 to 86400 seconds.";
				return false;
			}
			if (!action.Validate(context, error))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsMet(GC_LobbyMissionContext context)
	{
		int matching;
		foreach (GC_MissionCondition condition : m_aConditions)
		{
			// Evaluate every condition so timed conditions observe false results too.
			if (condition.Evaluate(context))
				matching++;
		}
		if (m_eMatch == GC_EMissionConditionMatch.ALL_CONDITIONS)
			return matching == m_aConditions.Count();
		return matching > 0;
	}
}
