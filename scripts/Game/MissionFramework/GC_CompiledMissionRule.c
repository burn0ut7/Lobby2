enum GC_ERuleEvent
{
	MATCH_BEGINS,
	TIMER,
	TEST_EVENT,
	FLAG_ON,
	FLAG_OFF,
	GAME_MODE_STATE,
	EVENT_RAN
}

enum GC_ERuleAction
{
	WRITE_LOG,
	SET_FLAG,
	SEND_TEST_EVENT,
	RANDOM_FLAG,
	SELECT_FLAGS,
	CALL_FLAG
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_RuleAction
{
	[Attribute("Test action", UIWidgets.EditBox, "A short name to help you find this action.")]
	string m_sName;
	[Attribute("0", UIWidgets.ComboBox, "Choose a small test action. These do not spawn units, end the game, or change player settings.", enums: {ParamEnum("Write a test message to the log", "0"), ParamEnum("Set a flag to yes or no", "1"), ParamEnum("Send a named test event", "2")})]
	GC_ERuleAction m_eDo;
	[Attribute("Test passed", UIWidgets.EditBox, "For a log message: the text to write. For a flag or test event: its exact name.")]
	string m_sTextOrName;
	[Attribute("1", UIWidgets.CheckBox, "For Set a flag: checked means yes; unchecked means no.")]
	bool m_bFlagAnswer;
	float m_fChancePercent;
	ref array<string> m_aFlagNames;
	int m_iSelectionCount;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		if (m_eDo == GC_ERuleAction.CALL_FLAG)
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
