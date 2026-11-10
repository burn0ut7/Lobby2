//! Shared by spawn setup and Assign Waypoints. Resolve all new orders before replacing any.
class GC_WaypointOrders
{
	//------------------------------------------------------------------------------------------------
	protected static bool Resolve(array<string> names, array<AIWaypoint> waypoints)
	{
		if (!names)
			return true;
		foreach (string name : names)
		{
			AIWaypoint waypoint = AIWaypoint.Cast(GetGame().GetWorld().FindEntityByName(name));
			if (!waypoint)
				return false;
			waypoints.Insert(waypoint);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool Check(array<string> names)
	{
		array<AIWaypoint> waypoints = {};
		return Resolve(names, waypoints);
	}

	//------------------------------------------------------------------------------------------------
	static bool Apply(AIGroup group, array<string> names, bool replaceExisting = false)
	{
		if (!group)
			return false;
		array<AIWaypoint> waypoints = {};
		if (!Resolve(names, waypoints))
			return false;
		if (replaceExisting)
		{
			array<AIWaypoint> previous = {};
			group.GetWaypoints(previous);
			SCR_EditableEntityComponent editableGroup = SCR_EditableEntityComponent.GetEditableEntity(group);
			// Native removal can delete group-owned editable waypoints. Reject reuse before changing orders.
			foreach (AIWaypoint desired : waypoints)
			{
				SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(SCR_EditableEntityComponent.GetEditableEntity(desired));
				if (previous.Contains(desired) && editableGroup && editable && editable.GetParentEntity() == editableGroup)
				{
					Print("GC | Mission Framework | Cannot replace orders with an already assigned group-owned editable waypoint. Use new waypoint entities or append orders.", LogLevel.WARNING);
					return false;
				}
			}
			foreach (AIWaypoint oldWaypoint : previous)
				group.RemoveWaypoint(oldWaypoint);
		}
		foreach (AIWaypoint waypoint : waypoints)
		{
			if (!group || !waypoint)
				return false;
			group.AddWaypoint(waypoint);
		}
		return true;
	}
}
