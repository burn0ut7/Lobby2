#ifdef WORKBENCH
class GC_WaypointChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return AIWaypoint.Cast(entity) != null;
	}
}
#endif
