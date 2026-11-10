[BaseContainerProps(), BaseContainerCustomStringTitleField("Timer Condition")]
class GC_TimerCondition : GC_ConditionBase
{
	[Attribute("60", UIWidgets.SpinBox, "Seconds after Match starts before this condition becomes true. It starts false, then stays true for the rest of the Match. Other conditions must still pass. This timer does not restart when an event runs or is reset, and Repeatable does not make it a repeating interval. No timer runs during briefing or after Match.", "0.1 86400 0.1")]
	float m_fSeconds;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		// Reuse the existing scheduled-leaf path: certainty, one deadline, no random draw.
		// Do not expose a random reset name: resetting an event does not restart this match timer.
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 6;
		check.m_fRandomInterval = m_fSeconds;
		check.m_fRandomChance = 1;
		check.m_iRandomBehavior = 1;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
