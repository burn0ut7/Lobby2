[ComponentEditorProps(category: "Lobby2/Mission", description: "Workbench-only automatic placed-content metadata. Writes generated JSON beside the world; does not edit mission headers.")]
class GC_MissionMetadataScannerComponentClass : ScriptComponentClass
{
}

//! Explicit opt-in through the GC mission template. No game/server update events.
class GC_MissionMetadataScannerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Automatically update generated metadata while this mission is open in Workbench.")]
	protected bool m_bEnabled;

#ifdef WORKBENCH
	protected float m_fElapsed;
	protected string m_sPrevious;
	protected string m_sOutput;
	protected string m_sLastWarning;

	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		// Must work when the game-mode entity is off-screen. Only a timer runs each frame.
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bEnabled || !GetGame() || GetGame().InPlayMode())
			return;
		m_fElapsed += timeSlice;
		if (m_fElapsed < 10)
			return;
		m_fElapsed = 0;
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.GetWorld() || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode()
			|| api.IsDoingEditAction() || api.IsModifyingData() || api.UndoOrRedoIsRestoring())
			return;
		IEntitySource source = api.EntityToSource(owner);
		if (!source || source.GetParent())
			return;
		int subscene = source.GetSubScene();
		int scanners;
		for (int i, count = api.GetEntityCount(subscene); i < count; i++)
		{
			IEntitySource candidate = api.GetEntity(subscene, i);
			if (!candidate || candidate.GetSubScene() != subscene)
				continue;
			typename type = candidate.GetClassName().ToType();
			if (type && type.IsInherited(GenericTerrainEntity))
			{
				Warn("Refusing automatic metadata for the terrain-owning subscene.");
				return;
			}
			if (candidate.GetParent())
				continue;
			IEntity entity = api.SourceToEntity(candidate);
			if (entity && entity.FindComponent(GC_MissionMetadataScannerComponent))
				scanners++;
		}
		if (scanners != 1)
		{
			Warn("Keep exactly one metadata scanner on a root entity in this mission subscene.");
			return;
		}
		string worldPath = api.GetWorld().GetSubSceneName(subscene);
		string absolutePath;
		if (worldPath.IsEmpty() || !Workbench.GetAbsolutePath(worldPath, absolutePath))
		{
			Warn("Save the mission to a local world file before automatic metadata can be written.");
			return;
		}
		string output = absolutePath + ".metadata.json";
		if (output != m_sOutput)
		{
			m_sOutput = output;
			m_sPrevious = string.Empty;
		}
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		exporter.Collect(api, subscene);
		bool changed;
		if (!exporter.SaveIfChanged(output, worldPath, m_sPrevious, changed))
		{
			Warn("Could not write generated metadata: " + output);
			return;
		}
		m_sLastWarning = string.Empty;
		if (changed)
			PrintFormat("GC | Automatic Metadata | Updated %1 entities: %2", exporter.GetEntityCount(), output);
	}

	//------------------------------------------------------------------------------------------------
	protected void Warn(string message)
	{
		if (message == m_sLastWarning)
			return;
		m_sLastWarning = message;
		Print("GC | Automatic Metadata | " + message, LogLevel.WARNING);
	}
#endif
}
