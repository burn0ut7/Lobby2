enum GC_ERuleEvent
{
	MATCH_BEGINS,
	TIMER,
	TEST_EVENT,
	FLAG_ON,
	FLAG_OFF,
	GAME_MODE_STATE,
	EVENT_RAN,
	RANDOM_CHECK,
	CASUALTIES_CHANGED,
	ENTITY_DAMAGE_CHANGED,
	AREA_CHANGED,
	INTERACTION_USED
}

enum GC_ERuleAction
{
	WRITE_LOG,
	SET_FLAG,
	SEND_TEST_EVENT,
	RANDOM_FLAG,
	SELECT_FLAGS,
	CALL_FLAG,
	RESET_FLAG,
	FLAG_CONTROL,
	EVENT_CONTROL,
	MISSION_MESSAGE,
	RESET_RANDOM,
	END_MISSION,
	RESPAWN_TICKETS,
	SPAWN,
	DELETE_ENTITIES,
	ASSIGN_WAYPOINTS,
	WEATHER
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_RuleAction
{
	[Attribute("Test action", UIWidgets.EditBox, "A short name to help you find this action.")]
	string m_sName;
	[Attribute("0", UIWidgets.ComboBox, "Choose a small test action. These do not spawn units, end the game, or change player settings.", enums: {ParamEnum("Write a test message to the log", "0"), ParamEnum("Set a flag to true or false", "1"), ParamEnum("Send a named test event", "2")})]
	GC_ERuleAction m_eDo;
	[Attribute("Test passed", UIWidgets.EditBox, "For a log message: the text to write. For a flag or test event: its exact name.")]
	string m_sTextOrName;
	[Attribute("1", UIWidgets.CheckBox, "For Set a flag: checked means true; unchecked means false.")]
	bool m_bFlagAnswer;
	float m_fChancePercent;
	ref array<string> m_aFlagNames;
	int m_iSelectionCount;
	int m_iOperation;
	int m_iAmount;
	ref GC_PrefabSpawn m_Spawn;
	float m_fTransitionSeconds;

	//------------------------------------------------------------------------------------------------
	static bool ValidTargetNames(array<string> names)
	{
		if (!names)
			return true;
		if (names.Count() > 128)
			return false;
		foreach (string name : names)
		{
			if (name.IsEmpty())
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		if (m_eDo == GC_ERuleAction.WEATHER)
			return !m_sTextOrName.IsEmpty() && m_fTransitionSeconds >= 0 && m_fTransitionSeconds <= 86400;
		if (m_eDo == GC_ERuleAction.DELETE_ENTITIES)
			return m_aFlagNames && !m_aFlagNames.IsEmpty() && GC_RuleAction.ValidTargetNames(m_aFlagNames);
		if (m_eDo == GC_ERuleAction.ASSIGN_WAYPOINTS)
			return !m_sTextOrName.IsEmpty() && GC_RuleAction.ValidTargetNames(m_aFlagNames)
				&& (m_bFlagAnswer || (m_aFlagNames && !m_aFlagNames.IsEmpty()));
		if (m_eDo == GC_ERuleAction.SPAWN)
			return m_Spawn && m_Spawn.IsValid();
		if (m_eDo == GC_ERuleAction.FLAG_CONTROL)
			return !m_sTextOrName.IsEmpty() && m_iOperation >= 0 && m_iOperation <= 5 && m_iAmount >= 0 && m_iAmount <= 1000000;
		if (m_eDo == GC_ERuleAction.EVENT_CONTROL)
			return !m_sTextOrName.IsEmpty() && m_iOperation >= 0 && m_iOperation <= 3;
		if (m_eDo == GC_ERuleAction.RESET_RANDOM)
			return !m_sTextOrName.IsEmpty();
		if (m_eDo == GC_ERuleAction.END_MISSION)
			return m_iOperation >= 0 && m_iOperation <= 3 && (m_iOperation == 0 || !m_sTextOrName.IsEmpty());
		if (m_eDo == GC_ERuleAction.RESPAWN_TICKETS)
			return !m_sTextOrName.IsEmpty() && m_iOperation >= 0 && m_iOperation <= 2 && m_iAmount >= 0 && m_iAmount <= 1000000;
		if (m_eDo == GC_ERuleAction.MISSION_MESSAGE)
		{
			if (m_sTextOrName.IsEmpty() || m_sTextOrName.Length() > 512 || m_sTextOrName.Contains("<") || m_sTextOrName.Contains(">"))
				return false;
			if (m_aFlagNames)
			{
				if (m_aFlagNames.Count() > 256)
					return false;
				foreach (string faction : m_aFlagNames)
				{
					if (faction.IsEmpty())
						return false;
				}
			}
			return true;
		}
		if (m_eDo == GC_ERuleAction.CALL_FLAG || m_eDo == GC_ERuleAction.RESET_FLAG)
			return !m_sTextOrName.IsEmpty();
		if (m_eDo == GC_ERuleAction.RANDOM_FLAG)
			return !m_sTextOrName.IsEmpty() && m_fChancePercent >= 0 && m_fChancePercent <= 100;
		if (m_eDo == GC_ERuleAction.SELECT_FLAGS)
		{
			if (!m_aFlagNames || m_aFlagNames.IsEmpty() || m_aFlagNames.Count() > 256
				|| m_iSelectionCount < 1 || m_iSelectionCount > m_aFlagNames.Count())
				return false;
			foreach (int index, string name : m_aFlagNames)
			{
				if (name.IsEmpty() || m_aFlagNames.Find(name) != index)
					return false;
			}
			return true;
		}
		return m_eDo >= 0 && m_eDo <= 2 && !m_sTextOrName.IsEmpty();
	}
}

class GC_CompiledMissionRule
{
	ref GC_RuleRandom m_Random;
	float m_fChance = 1;
	bool m_bObserveFlags;
	bool m_bObserveEvents;
	int m_iPickCount;
	ref array<ref GC_CompiledMissionRule> m_aChoices;
	string m_sName;
	bool m_bEnabled;
	GC_ERuleEvent m_eWhen;
	string m_sTestEventName;
	float m_fEverySeconds;
	ref GC_RuleCondition m_If;
	ref array<ref GC_RuleAction> m_aThen;
	bool m_bRunOnlyOnce;
	float m_fWaitSeconds;

	//------------------------------------------------------------------------------------------------
	bool IsValid(int depth = 0)
	{
		if (depth > 1 || m_fChance < 0 || m_fChance > 1)
			return false;
		if (m_Random && (!m_Random.IsValid() || m_fWaitSeconds + m_Random.m_fMaximumSeconds > 86400))
			return false;
		if (m_eWhen < 0 || m_eWhen > GC_ERuleEvent.GAME_MODE_STATE || m_fWaitSeconds < 0 || m_fWaitSeconds > 86400)
			return false;
		if (m_eWhen == GC_ERuleEvent.TIMER && (m_fEverySeconds < 0.1 || m_fEverySeconds > 86400))
			return false;
		if (m_eWhen >= GC_ERuleEvent.TEST_EVENT && m_eWhen <= GC_ERuleEvent.FLAG_OFF && m_sTestEventName.IsEmpty())
			return false;
		if (m_If && !m_If.IsValid())
			return false;
		if (m_aChoices)
		{
			if (depth > 0 || m_aChoices.IsEmpty() || m_aChoices.Count() > 255 || m_iPickCount < 1 || m_iPickCount > m_aChoices.Count() || (m_aThen && !m_aThen.IsEmpty()))
				return false;
			foreach (GC_CompiledMissionRule child : m_aChoices)
			{
				if (!child || !child.IsValid(depth + 1))
					return false;
			}
			return true;
		}
		if (!m_aThen || m_aThen.IsEmpty() || m_aThen.Count() > 256)
			return false;
		foreach (GC_RuleAction action : m_aThen)
		{
			if (!action || !action.IsValid())
				return false;
		}
		return true;
	}
}
