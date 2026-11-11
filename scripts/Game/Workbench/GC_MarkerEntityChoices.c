#ifdef WORKBENCH
class GC_MarkerEntityChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return entity.FindComponent(GC_BriefingMarkerComponent) != null;
	}
}
#endif
