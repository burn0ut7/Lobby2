#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Spawn Mission Template", description: "Place the GC co-op template and the loaded terrain's AI world in the active mission layer. One undo action; does not save.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF14E)]
class GC_MissionTemplatePlugin : WorkbenchPlugin
{
	protected static const ResourceName TEMPLATE = "{A79C4E0A43DEB821}Prefabs/Templates/GC_MissionTemplate_Coop.et";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction())
		{
			Report("Open a mission world in edit mode before spawning its template.");
			return;
		}

		int subscene = api.GetCurrentSubScene();
		int layer = api.GetCurrentEntityLayerId();
		if (layer < 0 || api.IsEntityLayerLockedHierarchy(subscene, layer))
		{
			Report("Select an unlocked mission layer first.");
			return;
		}

		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (!terrain || !api.GetWorld())
		{
			Report("No loaded terrain was found. Nothing was placed.");
			return;
		}
		string terrainScene = api.GetWorld().GetSubSceneName(terrain.GetSubScene());
		ResourceName aiPrefab = GetAIWorldPrefab(terrainScene);
		if (aiPrefab.IsEmpty())
		{
			Report("No AI-world mapping exists for terrain scene: " + terrainScene + ". Add its world/prefab pair to GC_MissionTemplatePlugin. Nothing was placed.");
			return;
		}
		if (subscene == terrain.GetSubScene())
		{
			Report("Select your mission subscene, not the base terrain scene. Nothing was placed.");
			return;
		}

		// Prefab-inherited children are not all present in GetEditorEntity's flat list.
		// Walk each root's source hierarchy, including roots in inherited layers.
		for (int i, count = api.GetEditorEntityCount(); i < count; i++)
		{
			IEntitySource source = api.GetEditorEntity(i);
			if (!source || source.GetParent())
				continue;
			IEntitySource conflict = FindConflict(source);
			if (conflict)
			{
				Report(string.Format("An existing %1 (%2, subscene %3) conflicts with the template. Nothing was placed; keep or remove that setup explicitly before trying again.", conflict.GetClassName(), conflict.GetName(), conflict.GetSubScene()));
				return;
			}
		}
		if (api.FindEntityByName("GC_MissionTemplate") || api.FindEntityByName("GC_MissionAIWorld"))
		{
			Report("The template entity names are already in use. Nothing was placed.");
			return;
		}
		if (!GC_ModWorkflow.BeforeTemplate(api.GetWorld().GetSubSceneName(subscene)))
			return;

		Resource templateResource = Resource.Load(TEMPLATE);
		Resource aiResource = Resource.Load(aiPrefab);
		if (!templateResource || !templateResource.IsValid() || !aiResource || !aiResource.IsValid())
		{
			Report("The mission template or mapped AI-world prefab could not be loaded. Nothing was placed.");
			return;
		}
		if (!api.BeginEntityAction("Spawn GC mission template"))
			return;

		IEntitySource root = api.CreateEntity(TEMPLATE, "GC_MissionTemplate", layer, null, vector.Zero, vector.Zero);
		IEntitySource aiWorld;
		if (root)
			aiWorld = api.CreateEntity(aiPrefab, "GC_MissionAIWorld", layer, root, vector.Zero, vector.Zero);
		bool cleaned = true;
		if (root && !aiWorld)
			cleaned = api.DeleteEntity(root);
		api.EndEntityAction();
		if (!root || !aiWorld)
		{
			if (cleaned)
				Report("Template placement failed. No template was retained.");
			else
				Report("AI-world placement failed and the partial template could not be removed. Inspect the world or Undo before trying again.");
			return;
		}
		api.ClearEntitySelection();
		api.AddToEntitySelection(root);
		PrintFormat("GC | Mission Template | Placed %1 under GC_MissionTemplate for %2. Save the mission to keep it.", aiPrefab, terrainScene);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntitySource FindConflict(IEntitySource source)
	{
		typename type = source.GetClassName().ToType();
		if (type && (type.IsInherited(BaseGameMode) || type.IsInherited(AIWorld) || type.IsInherited(FactionManager)
			|| type.IsInherited(RadioManagerEntity) || type.IsInherited(ScriptedChatEntity)
			|| type.IsInherited(PerceptionManager)))
			return source;
		for (int i, count = source.GetNumChildren(); i < count; i++)
		{
			IEntitySource conflict = FindConflict(source.GetChild(i));
			if (conflict)
				return conflict;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Exact terrain-owner world mapping; mission filenames and directory substrings are not map identifiers.
	protected ResourceName GetAIWorldPrefab(string terrainScene)
	{
		// GetSubSceneName returns the mounted resource path, including for packed worlds.
		terrainScene.Replace("\\", "/");
		terrainScene.ToLower();
		switch (terrainScene)
		{
			case "worlds/eden/eden.ent":
				return "{70CCCF16487C927F}Prefabs/AI/SCR_AIWorld_Eden.et";
			case "worlds/arland/arland.ent":
				return "{01DC74137CFDDB6A}Prefabs/AI/SCR_AIWorld_Arland.et";
			case "worlds/cain/cain.ent":
				return "{672531FAC0834C06}Prefabs/AI/SCR_AIWorld_Cain.et";
		}
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void Report(string message)
	{
		Print("GC | Mission Template | " + message, LogLevel.WARNING);
		Workbench.Dialog("GC Mission Template", message);
	}
}
#endif
