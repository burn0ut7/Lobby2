#ifdef WORKBENCH
class GC_DamageEntityChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return entity.FindComponent(SCR_DamageManagerComponent) != null;
	}
}
#endif
