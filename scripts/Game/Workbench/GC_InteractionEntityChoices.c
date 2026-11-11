#ifdef WORKBENCH
class GC_InteractionEntityChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return entity.FindComponent(GC_MissionInteractionComponent) != null;
	}
}
#endif
