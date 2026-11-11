#ifdef WORKBENCH
class GC_AIGroupChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return AIGroup.Cast(entity) != null;
	}
}
#endif
