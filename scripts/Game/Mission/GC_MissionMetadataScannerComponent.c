[ComponentEditorProps(category: "Lobby2/Mission", description: "Workbench-only automatic placed-content metadata. Writes generated JSON beside the world; does not edit mission headers.")]
class GC_MissionMetadataScannerComponentClass : ScriptComponentClass
{
}

//! Explicit opt-in through the GC mission template. No game/server update events.
class GC_MissionMetadataScannerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Refresh generated metadata on mission load, owner edits and config registration in Workbench.")]
	protected bool m_bEnabled;

#ifdef WORKBENCH
	protected bool m_bRefreshPending;
	protected string m_sRefreshReason;
	protected string m_sLastWarning;

	//------------------------------------------------------------------------------------------------
	void RequestRefresh(string reason)
	{
		if (!m_bRefreshPending)
			m_sRefreshReason = reason;
		m_bRefreshPending = true;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		RequestRefresh("world/entity initialization");
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_OnCreate(IEntity owner, IEntitySource src)
	{
		super._WB_OnCreate(owner, src);
		RequestRefresh("owner created or restored");
	}

	//------------------------------------------------------------------------------------------------
	override bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		bool result = super._WB_OnKeyChanged(owner, src, key, ownerContainers, parent);
		RequestRefresh("owner property changed");
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		// Off-screen event requests need a dispatch point. Idle frames only test a flag;
		// no timers, source enumeration, file checks or metadata collection run while idle.
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bRefreshPending)
			return;
		if (!m_bEnabled || !GetGame() || GetGame().InPlayMode())
		{
			m_bRefreshPending = false;
			return;
		}
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.GetWorld() || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode()
			|| api.IsDoingEditAction() || api.IsModifyingData() || api.UndoOrRedoIsRestoring())
			return;
		// Consume once the editor is ready. Failures require a new event, not retries on a timer.
		m_bRefreshPending = false;
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
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		exporter.Collect(api, subscene);
		if (!exporter.Save(output, worldPath))
		{
			Warn("Could not write generated metadata: " + output);
			return;
		}
		m_sLastWarning = string.Empty;
		PrintFormat("GC | Automatic Metadata | Refreshed %1 entities (%2): %3", exporter.GetEntityCount(), m_sRefreshReason, output);
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
