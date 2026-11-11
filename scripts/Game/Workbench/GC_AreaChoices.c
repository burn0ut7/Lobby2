#ifdef WORKBENCH
class GC_AreaChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		if (GC_RadiusAreaEntity.Cast(entity))
			return true;
		PolylineShapeEntity polygon = PolylineShapeEntity.Cast(entity);
		return polygon && polygon.IsClosed();
	}
}
#endif
