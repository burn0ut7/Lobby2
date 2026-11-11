[BaseContainerProps(), BaseContainerCustomStringTitleField("Custom Script Condition")]
class GC_CustomScriptCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.Object, "Select your GC_MissionScript subclass. Run(manager) returns the condition's true/false result. Sampled once per second during Match on the server; results are reused between samples. Keep it quick and read-only. Missing/invalid scripts cannot pass, even in a None group.")]
	ref GC_MissionScript m_Script;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 13;
		check.m_Script = m_Script;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
