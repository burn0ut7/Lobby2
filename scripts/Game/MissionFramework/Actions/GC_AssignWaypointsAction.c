[BaseContainerProps(), BaseContainerCustomStringTitleField("Assign Waypoints Action")]
class GC_AssignWaypointsAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Exact name of the existing AI group receiving these orders. Use the group name, not an individual soldier or vehicle name.", enums: {ParamEnum("", "", "gc-choices:2:GC_AIGroupChoices")})]
	string m_sGroupName;
	[Attribute("", UIWidgets.EditComboBox, "Exact names of existing AI waypoints, in order. If any waypoint is missing or is not an AI waypoint, no orders change.", enums: {ParamEnum("", "", "gc-choices:2:GC_WaypointChoices")})]
	ref array<string> m_aWaypoints;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: add after current orders. Checked: replace current orders; an empty list clears them. The engine may delete removed editable waypoints. Replacing with an already assigned, group-owned editable waypoint is rejected before any changes; use new waypoints instead. AI may still react to combat.")]
	bool m_bReplaceExisting;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.ASSIGN_WAYPOINTS;
		action.m_sTextOrName = m_sGroupName;
		action.m_bFlagAnswer = m_bReplaceExisting;
		action.m_aFlagNames = {};
		if (m_aWaypoints)
			action.m_aFlagNames.Copy(m_aWaypoints);
		return action;
	}
}
