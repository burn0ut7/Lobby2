[BaseContainerProps(), BaseContainerCustomStringTitleField("Timer Condition")]
class GC_TimerCondition : GC_ConditionBase
{
	[Attribute("1", UIWidgets.ComboBox, "Start once when the mission enters this state. If the framework starts in this state, start then. The timer continues through later states and stays true once finished, until the framework restarts.", enums: {ParamEnum("Mission preview", "0"), ParamEnum("Role selection", "1787356801"), ParamEnum("Briefing", "1787356802"), ParamEnum("Match", "1"), ParamEnum("After action", "2")})]
	SCR_EGameModeState m_eStartState;
	[Attribute("60", UIWidgets.EditBox, "Duration in seconds; decimals are allowed, for example 2.5. Enter 0.1 to 86400. If Random Max Seconds is filled in, choose one duration between the two values. Reversed values are automatically ordered. Event Repeatable and Reset do not restart this timer.", precision: 3)]
	float m_fSeconds;
	[Attribute("", UIWidgets.EditBox, "Optional other end of the random range, in seconds. Leave blank or use 0 for a fixed timer. Otherwise enter 0.1 to 86400; decimals are allowed. If smaller than Seconds, the two bounds are swapped. Equal values are fixed. One duration is chosen when the timer starts, never on condition checks.", precision: 3)]
	float m_fRandomMaxSeconds;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		// Positive comparisons also reject NaN; upper bounds reject infinity.
		if (!(m_fSeconds >= 0.1 && m_fSeconds <= 86400)
			|| !(m_fRandomMaxSeconds == 0 || (m_fRandomMaxSeconds >= 0.1 && m_fRandomMaxSeconds <= 86400)))
			return null;
		// One shared scheduler; timer duration is sampled on state entry, never during evaluation.
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 12;
		check.m_eGameModeState = m_eStartState;
		check.m_fRandomInterval = m_fSeconds;
		check.m_fTimerMaximum = m_fSeconds;
		if (m_fRandomMaxSeconds > 0)
		{
			check.m_fRandomInterval = Math.Min(m_fSeconds, m_fRandomMaxSeconds);
			check.m_fTimerMaximum = Math.Max(m_fSeconds, m_fRandomMaxSeconds);
		}
		check.m_fRandomChance = 1;
		check.m_iRandomBehavior = 1;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
