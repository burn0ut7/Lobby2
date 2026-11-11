//! Presentation derived from native end data; faction and commander ownership stays native.
class GC_MissionResult
{
	static string Reason(int reason)
	{
		switch (reason)
		{
			case EGameOverTypes.EDITOR_FACTION_VICTORY:
			case EGameOverTypes.FACTION_VICTORY_SCORE:
			case EGameOverTypes.FACTION_VICTORY_TIME:
				return "Victory";
			case EGameOverTypes.EDITOR_FACTION_DEFEAT:
			case EGameOverTypes.FACTION_DEFEAT_SCORE:
			case EGameOverTypes.FACTION_DEFEAT_TIME:
				return "Defeat";
			case EGameOverTypes.LOOSE:
				return "Mission failed";
			case EGameOverTypes.ENDREASON_DRAW:
			case EGameOverTypes.FACTION_DRAW:
			case EGameOverTypes.EDITOR_FACTION_DRAW:
			case EGameOverTypes.COMBATPATROL_DRAW:
			case EGameOverTypes.DEATHMATCH_DRAW:
				return "Draw";
			case EGameOverTypes.EDITOR_NEUTRAL:
				return "Mission complete";
			case EGameOverTypes.SERVER_RESTART:
				return "Server restart";
		}
		return "Mission ended";
	}

	//------------------------------------------------------------------------------------------------
	static string Title(SCR_GameModeEndData data)
	{
		if (!data)
			return "Mission ended";
		string title = Reason(data.GetEndReason());
		Faction faction = GetGame().GetFactionManager().GetFactionByIndex(data.GetWinnerFactionId());
		if (faction)
			return GC_DebriefText.Short(WidgetManager.Translate(faction.GetFactionName())) + " — " + title;
		return title;
	}
}
