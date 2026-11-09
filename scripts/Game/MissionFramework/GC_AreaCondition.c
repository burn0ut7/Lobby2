[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Condition")]
class GC_AreaCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditBox, "Exact unique name of the world entity to check. Uses its position, not its size or collision shape. This does not search for every entity in the area. Multiple targets can be combined using Condition Group or Condition Count.")]
	string m_sTarget;

	[Attribute("", UIWidgets.EditBox, "Exact unique name of a Radius Area or closed polygon in the world. Radius Area uses a 3D distance in metres. A polygon checks its horizontal outline and ignores height, like the AO limit. Configure the shape on that area, not here.")]
	string m_sArea;

	[Attribute("0", UIWidgets.ComboBox, "Inside passes when the target is in the area; Outside passes when it is out. Checks run once per second during Match and on phase changes. Missing targets or invalid areas cannot pass, even in a None group. Brief crossings between checks can be missed.", enums: {ParamEnum("Inside", "0"), ParamEnum("Outside", "1")})]
	int m_iCheck;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition condition = new GC_RuleCondition();
		condition.m_iCheck = 10;
		condition.m_sAreaTarget = m_sTarget;
		condition.m_sAreaName = m_sArea;
		condition.m_bAreaOutside = m_iCheck == 1;
		if (m_iCheck < 0 || m_iCheck > 1 || !condition.IsValid(depth))
			return null;
		return condition;
	}
}
