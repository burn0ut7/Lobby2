#ifdef WORKBENCH
//! AO return anchors belong to the mission being edited, not its inherited map.
class GC_AOReturnEntityChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override array<IEntitySource> GetChoiceEntities(WorldEditorAPI api)
	{
		array<IEntitySource> sources = {};
		if (!api || !api.GetWorld())
			return sources;

		int subScene = api.GetCurrentSubScene();
		if (subScene < 0 || subScene >= api.GetNumSubScenes())
			return sources;

		// Use the editor-wide inventory so nested entities remain eligible too.
		// Reject other subscenes before resolving any live entity.
		int entityCount = api.GetEditorEntityCount();
		for (int index = 0; index < entityCount; index++)
		{
			IEntitySource source = api.GetEditorEntity(index);
			if (source && source.GetSubScene() == subScene && api.SourceToEntity(source))
				sources.Insert(source);
		}
		return sources;
	}
}
#endif
