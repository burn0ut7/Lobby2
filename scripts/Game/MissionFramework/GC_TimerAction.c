[BaseContainerProps(), BaseContainerCustomStringTitleField("Timer Action")]
class GC_TimerAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditBox, "Exact Timer Name on a Timer Condition, including inside a condition group or flag. Missing or duplicate names reject the action without changing any timer.")]
	string m_sTimerName;
	[Attribute("0", UIWidgets.ComboBox, "Restart: start now using the timer's duration, choosing again if random. Pause/Resume: freeze/continue a started timer. Set time: replace remaining time and start a waiting timer. Add/Remove: adjust a started timer. Paused timers stay paused when adjusted. Zero remaining time finishes immediately. This never resets the owning event or bypasses its other conditions.", enums: {ParamEnum("Restart", "0"), ParamEnum("Pause", "1"), ParamEnum("Resume", "2"), ParamEnum("Set time", "3"), ParamEnum("Add time", "4"), ParamEnum("Remove time", "5")})]
	int m_iOperation;
	[Attribute("0", UIWidgets.EditBox, "Seconds for Set time, Add time or Remove time only. Decimals are allowed. Enter 0 to 86400. Restart, Pause and Resume ignore this field. Remove stops at zero; adding beyond 86400 remaining seconds is rejected.", precision: 3)]
	float m_fSeconds;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.TIMER_CONTROL;
		action.m_sTextOrName = m_sTimerName;
		action.m_sTextOrName.TrimInPlace();
		action.m_iOperation = m_iOperation;
		action.m_fTimerSeconds = m_fSeconds;
		return action;
	}
}
