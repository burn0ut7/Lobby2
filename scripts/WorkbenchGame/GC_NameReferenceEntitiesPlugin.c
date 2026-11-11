#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Name unnamed entities", description: "Give unnamed entities in the active editable subscene their standard Workbench names so string references can find them. Existing names and locked layers are preserved.", wbModules: {"WorldEditor"}, category: "GC")]
class GC_NameReferenceEntitiesPlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor || !GetGame() || GetGame().InPlayMode())
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.GetWorld() || !api.IsEditMode() || api.IsGameMode() || api.IsDoingEditAction())
			return;
		string worldPath;
		api.GetWorldPath(worldPath);
		if (!worldPath.EndsWith(".ent"))
			return;

		array<EntityID> candidates = {};
		for (int i; i < api.GetEditorEntityCount(); i++)
		{
			IEntitySource source = api.GetEditorEntity(i);
			if (CanName(api, source))
				candidates.Insert(source.GetID());
		}
		if (candidates.IsEmpty())
		{
			Print("GC | Reference names | No unnamed editable entities.");
			return;
		}
		if (!api.BeginEntityAction("Name unnamed entities"))
			return;
		int named;
		foreach (EntityID id : candidates)
		{
			IEntitySource source = api.FindEntityByID(id);
			if (!CanName(api, source))
				continue;
			string name = api.GetEntityNiceName(source);
			if (name.IsEmpty() || api.FindEntityByName(name) || api.GetWorld().FindEntityByName(name))
				name = api.GenerateDefaultEntityName(source);
			if (name.IsEmpty() || api.FindEntityByName(name) || api.GetWorld().FindEntityByName(name))
				continue;
			if (!api.RenameEntity(source, name))
				break;
			source = api.FindEntityByID(id);
			if (!source)
				break;
			IEntity entity = api.SourceToEntity(source);
			if (!source || !entity || source.GetName() != name || entity.GetName() != name
				|| api.FindEntityByName(name) != source || api.GetWorld().FindEntityByName(name) != entity)
			{
				Print("GC | Reference names | Name did not verify; inspect before continuing.", LogLevel.ERROR);
				break;
			}
			named++;
			Print("GC | Reference names | Assigned " + name);
		}
		bool ended = api.EndEntityAction();
		api.UpdateSelectionGui();
		PrintFormat("GC | Reference names | Named %1 entities; actionClosed=%2. Save the world to keep these names.", named, ended);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanName(WorldEditorAPI api, IEntitySource source)
	{
		return source && source.GetName().IsEmpty() && source.GetSubScene() == api.GetCurrentSubScene()
			&& !api.IsEntityLayerLockedHierarchy(source.GetSubScene(), source.GetLayerID()) && api.SourceToEntity(source);
	}
}
#endif
