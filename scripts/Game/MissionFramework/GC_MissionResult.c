//! Presentation derived from native end data; faction and commander ownership stays native.
class GC_MissionResult
{
	//! Shared native result construction for authored actions and authenticated admin controls.
	static SCR_GameModeEndData Build(int result, string factionKey, array<string> winners)
	{
		if (result < 0 || result > 4 || (winners && winners.Count() > 16))
			return null;
		EGameOverTypes reason = EGameOverTypes.EDITOR_NEUTRAL;
		array<int> factions = {};
		FactionManager manager = GetGame().GetFactionManager();
		if (result > 0 && result < 4)
		{
			Faction faction = manager.GetFactionByKey(factionKey);
			if (!faction)
				return null;
			factions.Insert(manager.GetFactionIndex(faction));
			if (result == 1)
				reason = EGameOverTypes.EDITOR_FACTION_VICTORY;
			else if (result == 2)
				reason = EGameOverTypes.EDITOR_FACTION_DEFEAT;
			else
				reason = EGameOverTypes.EDITOR_FACTION_DRAW;
		}
		else if (result == 4)
			reason = EGameOverTypes.LOOSE;
		if (winners)
		{
			foreach (string key : winners)
			{
				Faction winner = manager.GetFactionByKey(key);
				if (!winner)
					return null;
				int index = manager.GetFactionIndex(winner);
				if (!factions.Contains(index))
					factions.Insert(index);
			}
		}
		return SCR_GameModeEndData.Create(reason, null, factions);
	}

	//------------------------------------------------------------------------------------------------
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
