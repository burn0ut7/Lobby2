//! Explicit validation checks only; no automatic execution or world mutations.
class GC_EntityActionTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunEntityActionValidation()
	{
		GC_DeleteEntitiesAction deletion = new GC_DeleteEntitiesAction();
		Check(!deletion.BuildAction().IsValid(), "Delete requires targets");
		deletion.m_aEntities = {"truck", "truck"};
		GC_RuleAction compiled = deletion.BuildAction();
		Check(compiled.IsValid(), "Duplicate delete names are accepted and deduplicated at execution");
		deletion.m_aEntities.Clear();
		Check(compiled.m_aFlagNames.Count() == 2, "Compiled delete list is independent of authoring list");
		deletion.m_aEntities = {""};
		Check(!deletion.BuildAction().IsValid(), "Blank target rejected");
		GC_AssignWaypointsAction orders = new GC_AssignWaypointsAction();
		orders.m_sGroupName = "patrol";
		Check(!orders.BuildAction().IsValid(), "Empty append rejected");
		orders.m_bReplaceExisting = true;
		Check(orders.BuildAction().IsValid(), "Empty replacement clears orders");
		orders.m_aWaypoints = {"route_a", "route_b"};
		compiled = orders.BuildAction();
		Check(compiled.IsValid() && compiled.m_bFlagAnswer, "Replacement keeps ordered waypoint names");
		orders.m_aWaypoints.Clear();
		Check(compiled.m_aFlagNames[1] == "route_b", "Compiled waypoint list is independent of authoring list");
		orders.m_sGroupName = "";
		Check(!orders.BuildAction().IsValid(), "Clearing still requires a group name");
		Check(!GC_WaypointOrders.Apply(null, null, true), "Missing group cannot change orders");
		return string.Format("EntityActionValidation: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}
