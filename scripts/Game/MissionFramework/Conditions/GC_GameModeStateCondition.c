//! Reads the active GC Lobby state supplied by its authoritative game-mode callback.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Game Mode State Condition")]
class GC_GameModeStateCondition : GC_ConditionBase
{
	[Attribute("1", UIWidgets.ComboBox, "Choose the mission phase. Match means active play after briefing.", enums: {ParamEnum("Mission preview", "0"), ParamEnum("Role selection", "1787356801"), ParamEnum("Briefing", "1787356802"), ParamEnum("Match", "1"), ParamEnum("After action", "2")})]
	SCR_EGameModeState m_eGameModeState;

	[Attribute("0", UIWidgets.CheckBox, "Checked: true when the game is in another state. Unchecked: true when in the selected state. Checked at startup and state changes, not every frame.")]
	bool m_bRequireOtherState;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_LobbyPhase.IsValid(m_eGameModeState))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 3;
		check.m_eGameModeState = m_eGameModeState;
		check.m_bReverseAnswer = m_bRequireOtherState;
		return check;
	}
}
