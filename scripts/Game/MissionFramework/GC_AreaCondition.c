[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Condition")]
class GC_AreaCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.Object, "Choose Named Entities for specific world objects, or Players / AI for living characters. Only the selected target type has settings. Spectators never count as Players / AI.")]
	ref GC_AreaTargetsBase m_Targets;

	[Attribute("", UIWidgets.EditBox, "Exact unique name of a Radius Area or closed polygon in the world. Radius Area uses a 3D distance in metres. A polygon checks its horizontal outline and ignores height, like the AO limit. Configure the shape on that area, not here.")]
	string m_sArea;

	[Attribute("0", UIWidgets.ComboBox, "Choose which side to count. By default, at least one chosen target must be on this side; Count can change that requirement. Checks run once per second during Match and on phase changes. Missing named targets or invalid areas cannot pass, even in a None group. Brief crossings between checks can be missed.", enums: {ParamEnum("Inside", "0"), ParamEnum("Outside", "1")})]
	int m_iCheck;

	[Attribute("", UIWidgets.Object, "Optional number of matching targets. Leave unset to require at least one target on the selected side. Count compares targets, not child conditions.")]
	ref GC_AreaCount m_Count;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition condition = new GC_RuleCondition();
		condition.m_iCheck = 10;
		condition.m_sAreaName = m_sArea;
		condition.m_bAreaOutside = m_iCheck == 1;
		if (!m_Targets || !m_Targets.BuildTargets(condition))
			return null;
		if (m_Count)
		{
			condition.m_iAreaComparison = m_Count.m_iComparison;
			condition.m_iAreaRequired = m_Count.m_iRequiredCount;
		}
		if (m_iCheck < 0 || m_iCheck > 1 || !condition.IsValid(depth))
			return null;
		return condition;
	}
}
