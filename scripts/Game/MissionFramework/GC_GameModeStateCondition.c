//! Reads the active Lobby2 state supplied by its authoritative game-mode callback.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Game Mode State Condition")]
class GC_GameModeStateCondition : GC_ConditionBase
{
	[Attribute("1", UIWidgets.ComboBox, "Choose the mission phase. Match means active play after briefing.", enums: {ParamEnum("Mission preview", "0"), ParamEnum("Role selection", "1787356801"), ParamEnum("Briefing", "1787356802"), ParamEnum("Match", "1"), ParamEnum("After action", "2")})]
	SCR_EGameModeState m_eGameModeState;

	[Attribute("0", UIWidgets.ComboBox, "Is in passes while the selected phase is active. Is not in passes in other phases. Checked when the framework starts and whenever the state changes, not repeatedly each frame.", enums: {ParamEnum("Is in this state", "0"), ParamEnum("Is not in this state", "1")})]
	int m_iMatch;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_LobbyPhase.IsValid(m_eGameModeState) || m_iMatch < 0 || m_iMatch > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 3;
		check.m_eGameModeState = m_eGameModeState;
		check.m_bReverseAnswer = m_iMatch == 1;
		return check;
	}
}
