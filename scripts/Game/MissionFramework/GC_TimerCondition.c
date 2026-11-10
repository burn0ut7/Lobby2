[BaseContainerProps(), BaseContainerCustomStringTitleField("Timer Condition")]
class GC_TimerCondition : GC_ConditionBase
{
	[Attribute("1", UIWidgets.ComboBox, "Start once when the mission enters this state. If the framework starts in this state, start then. The timer continues through later states and stays true once finished, until the framework restarts.", enums: {ParamEnum("Mission preview", "0"), ParamEnum("Role selection", "1787356801"), ParamEnum("Briefing", "1787356802"), ParamEnum("Match", "1"), ParamEnum("After action", "2")})]
	SCR_EGameModeState m_eStartState;
	[Attribute("60", UIWidgets.SpinBox, "Fixed duration in seconds. When Random Duration is checked, this is the shortest possible duration. Event Repeatable and Reset do not restart this timer.", "0.1 86400 0.1")]
	float m_fSeconds;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: use Seconds exactly. Checked: choose one duration between Seconds and Maximum Seconds when the timer starts. Checking conditions never chooses again.")]
	bool m_bRandomDuration;
	[Attribute("120", UIWidgets.SpinBox, "Longest possible duration in seconds, used only when Random Duration is checked. Must be at least Seconds. Equal values give a fixed duration.", "0.1 86400 0.1")]
	float m_fMaximumSeconds;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		// One shared scheduler; timer duration is sampled on state entry, never during evaluation.
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 12;
		check.m_eGameModeState = m_eStartState;
		check.m_fRandomInterval = m_fSeconds;
		check.m_fTimerMaximum = m_fSeconds;
		if (m_bRandomDuration)
			check.m_fTimerMaximum = m_fMaximumSeconds;
		check.m_fRandomChance = 1;
		check.m_iRandomBehavior = 1;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
