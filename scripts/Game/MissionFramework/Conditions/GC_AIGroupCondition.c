[BaseContainerProps(), BaseContainerCustomStringTitleField("AI Group Condition")]
class GC_AIGroupCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Existing AI group in the world. Select the group itself, not a soldier or vehicle. Missing groups are retried once per second during Match and cannot pass through inversion.", enums: {ParamEnum("", "", "gc-choices:2:GC_AIGroupChoices")})]
	string m_sGroup;
	[Attribute("2", UIWidgets.ComboBox, "Group control mode to observe. Autonomous normally means combat movement. Individual soldiers can fight while the group remains Following waypoint, particularly on Defend waypoints. None is pre-initialization; groups normally enter Idle before following orders.", enums: {ParamEnum("None (initializing)", "0"), ParamEnum("Idle", "1"), ParamEnum("Autonomous (combat movement)", "2"), ParamEnum("Following waypoint", "3")})]
	EGroupControlMode m_eMode;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: stay true after the first observed match, like TilW's one-shot control-mode flag. Checked: become false again when the group leaves the selected mode. Event repeatability and remembered mission flags retain their normal behavior. Observation begins at Match entry.")]
	bool m_bContinuous;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 14;
		check.m_sAIGroup = m_sGroup;
		check.m_eAIGroupMode = m_eMode;
		check.m_bAIGroupContinuous = m_bContinuous;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
