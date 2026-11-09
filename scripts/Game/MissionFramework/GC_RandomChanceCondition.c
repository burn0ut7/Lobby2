//! Scheduled chance with a stable result between rolls; evaluation never rerolls it.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Random Chance Condition")]
class GC_RandomChanceCondition : GC_ConditionBase
{
	[Attribute("60", UIWidgets.SpinBox, "Seconds between random checks during Match. For example, 90 checks every 90 seconds. The first check waits one full interval after Match starts. No checks run before or after Match.", "0.1 86400 0.1")]
	float m_fCheckInterval;

	[Attribute("1", UIWidgets.Slider, "Chance from 0 to 1 for each scheduled check: 1 = 100%, 0.05 = 5%, 0.0005 = 0.05%, and 0 = never true. Other conditions do not reroll it. Before the first check its answer is false; All, Any and None combine that answer normally. Events run once unless Advanced > Run is Every time.", "0 1 0.0001")]
	float m_fChance;

	[Attribute("0", UIWidgets.ComboBox, "Keep rolling: make a new roll every Check Interval and keep each answer until the next roll. Roll once: after the first interval, keep that first answer for this Match, even if it is false. This controls the condition only; the event's Once or Every time setting controls its actions.", enums: {ParamEnum("Keep rolling", "0"), ParamEnum("Roll once", "1")})]
	int m_iRollBehavior;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 6;
		check.m_fRandomInterval = m_fCheckInterval;
		check.m_fRandomChance = m_fChance;
		check.m_iRandomBehavior = m_iRollBehavior;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}

//! Per-engine schedule. Authored and compiled condition containers never hold live results.
class GC_RandomConditionState
{
	ref GC_RuleCondition m_Condition;
	float m_fNextCheck;
	bool m_bHasRolled;
}
