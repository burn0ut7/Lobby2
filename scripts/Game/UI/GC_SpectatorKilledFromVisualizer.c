//! Client-only killed-from preview and line renderer. No entity created here is
//! inserted on the server or exposed as replicated gameplay state.
class GC_SpectatorKilledFromVisualizer
{
	protected static const float LINE_WIDTH = 3.0;
	protected static const float KILLER_LABEL_WIDTH = 160.0;
	protected static const float KILLER_LABEL_HEIGHT = 36.0;
	protected static const float KILLER_LABEL_WORLD_OFFSET = 2.2;
	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected IEntity m_KillerPreviewEntity;
	protected IEntity m_VictimPreviewEntity;
	protected bool m_bEnabled;
	protected bool m_bVisibilityReported;
	protected bool m_bLastTraceVisible;
	protected bool m_bLastLabelVisible;
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world, GC_KilledFromSnapshot snapshot, bool enabled)
	{
		if (!canvas || !workspace || !world)
		{
			PrintFormat("GC | Killed From | event=visualizer-start result=rejected canvas=%1 workspace=%2 world=%3", canvas, workspace, world, level: LogLevel.ERROR);
			return false;
		}

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_Snapshot = snapshot;
		if (snapshot)
			PrintFormat("GC | Killed From | event=visualizer-start result=ready enabled=%1 snapshot={%2}", enabled, snapshot.GetDebugReport());
		else
			PrintFormat("GC | Killed From | event=visualizer-start result=ready enabled=%1 snapshot=none", enabled);
		SetEnabled(enabled);
		return true;
	}

	void SetSnapshot(GC_KilledFromSnapshot snapshot)
	{
		if (snapshot)
			PrintFormat("GC | Killed From | event=visualizer-snapshot enabled=%1 killerPreview=%2 victimPreview=%3 snapshot={%4}", m_bEnabled, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null, snapshot.GetDebugReport());
		else
			PrintFormat("GC | Killed From | event=visualizer-snapshot enabled=%1 killerPreview=%2 victimPreview=%3 snapshot=none", m_bEnabled, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null);
		m_Snapshot = snapshot;
		m_bVisibilityReported = false;
		DestroyPreview();
		if (m_bEnabled)
			CreatePreview();
	}

	void SetEnabled(bool enabled)
	{
		if (m_bEnabled == enabled)
		{
			PrintFormat("GC | Killed From | event=visualizer-toggle result=unchanged enabled=%1 snapshot=%2 killerPreview=%3 victimPreview=%4", enabled, m_Snapshot != null, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null);
			return;
		}

		m_bEnabled = enabled;
		m_bVisibilityReported = false;
		PrintFormat("GC | Killed From | event=visualizer-toggle result=changed enabled=%1 snapshot=%2 killerPreview=%3 victimPreview=%4", enabled, m_Snapshot != null, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null);
		GetGame().GetCallqueue().Remove(Update);
		if (m_bEnabled)
		{
			CreatePreview();
			GetGame().GetCallqueue().CallLater(Update, 0, true);
			return;
		}

		DestroyPreview();
		ClearLine();
	}

	void Destroy()
	{
		PrintFormat("GC | Killed From | event=visualizer-destroy enabled=%1 snapshot=%2 killerPreview=%3 victimPreview=%4", m_bEnabled, m_Snapshot != null, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null);
		GetGame().GetCallqueue().Remove(Update);
		DestroyPreview();
		ClearLine();
		m_Snapshot = null;
		m_wCanvas = null;
		m_Workspace = null;
		m_World = null;
	}

	protected void CreatePreview()
	{
		if (m_KillerPreviewEntity || m_VictimPreviewEntity)
		{
			PrintFormat("GC | Killed From | event=preview-create result=ignored reason=already-exists killer=%1 victim=%2", m_KillerPreviewEntity, m_VictimPreviewEntity);
			return;
		}
		if (!m_Snapshot)
		{
			Print("GC | Killed From | event=preview-create result=rejected reason=no-snapshot", LogLevel.WARNING);
			return;
		}
		vector killerTransform[4];
		m_Snapshot.GetKillerRootTransform(killerTransform);
		m_KillerPreviewEntity = CreateActorClone(m_Snapshot.GetKillerPrefab(), killerTransform, m_Snapshot.GetStance(), m_Snapshot.GetDynamicStance(), m_Snapshot.GetFlags(), m_Snapshot.GetKillerBonePose(), Color.FromRGBA(235, 45, 70, 255), "killer");

		vector victimTransform[4];
		m_Snapshot.GetVictimRootTransform(victimTransform);
		m_VictimPreviewEntity = CreateActorClone(m_Snapshot.GetVictimPrefab(), victimTransform, m_Snapshot.GetVictimStance(), m_Snapshot.GetVictimDynamicStance(), m_Snapshot.GetVictimFlags(), m_Snapshot.GetVictimBonePose(), Color.FromRGBA(65, 170, 255, 255), "victim");
		PrintFormat("GC | Killed From | event=preview-create result=complete killer=%1 victim=%2 snapshot={%3}", m_KillerPreviewEntity, m_VictimPreviewEntity, m_Snapshot.GetDebugReport());
	}

	protected IEntity CreateActorClone(ResourceName prefab, vector rootTransform[4], int stance, float dynamicStance, int flags, array<ref GC_KilledFromBonePose> bonePose, Color outlineColor, string role)
	{
		if (prefab == ResourceName.Empty)
		{
			PrintFormat("GC | Killed From | event=actor-clone result=skipped role=%1 reason=no-prefab", role, level: LogLevel.WARNING);
			return null;
		}

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			PrintFormat("GC | Killed From | event=actor-clone result=failed role=%1 stage=resource prefab=%2", role, prefab, level: LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixCopy(rootTransform, params.Transform);
		IEntity preview = GetGame().SpawnEntityPrefabLocal(resource, m_World, params);
		if (!preview)
		{
			PrintFormat("GC | Killed From | event=actor-clone result=failed role=%1 stage=spawn prefab=%2 transform=%3", role, prefab, rootTransform, level: LogLevel.ERROR);
			return null;
		}

		AIControlComponent aiControl = AIControlComponent.Cast(preview.FindComponent(AIControlComponent));
		if (aiControl)
			aiControl.DeactivateAI();
		ApplyCharacterState(preview, stance, dynamicStance, flags);
		int isolatedPhysics = IsolatePhysicsHierarchy(preview);
		ReportPhysicsHierarchy(preview, role);

		ChimeraWorld world = ChimeraWorld.CastFrom(m_World);
		if (world)
			world.OutlineEntity(preview, outlineColor, 1.0, 0.2, true, 3);

		int capturedBones;
		if (bonePose)
			capturedBones = bonePose.Count();
		vector transformReadback[4];
		preview.GetWorldTransform(transformReadback);
		PrintFormat("GC | Killed From | event=actor-clone result=spawned role=%1 preview=%2 prefab=%3 isolatedPhysics=%4 capturedBones=%5 applyBones=0 sourceMutation=none", role, preview, prefab, isolatedPhysics, capturedBones);
		PrintFormat("GC | Killed From | event=actor-clone-transform role=%1 requested=%2 readback=%3", role, rootTransform, transformReadback);
		return preview;
	}

	protected void ApplyCharacterState(IEntity entity, int stance, float dynamicStance, int flags)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;
		controller.ForceStance(stance + 1);
		if (controller.CanSetDynamicStance(dynamicStance))
			controller.SetDynamicStance(dynamicStance);
		controller.SetWeaponRaised((flags & GC_KilledFromSnapshot.FLAG_WEAPON_RAISED) != 0);
		controller.SetWeaponADS((flags & GC_KilledFromSnapshot.FLAG_WEAPON_ADS) != 0);
	}

	protected int IsolatePhysicsHierarchy(IEntity entity)
	{
		if (!entity)
			return 0;
		int count;
		Physics physics = entity.GetPhysics();
		if (physics)
		{
			physics.SetInteractionLayer(EPhysicsLayerDefs.Unused);
			int geometryCount = physics.GetNumGeoms();
			for (int geometryIndex; geometryIndex < geometryCount; geometryIndex++)
				physics.SetGeomInteractionLayer(geometryIndex, EPhysicsLayerDefs.Unused);
			count++;
		}
		IEntity child = entity.GetChildren();
		while (child)
		{
			count += IsolatePhysicsHierarchy(child);
			child = child.GetSibling();
		}
		return count;
	}

	protected void ReportPhysicsHierarchy(IEntity entity, string role)
	{
		if (!entity)
			return;
		Physics physics = entity.GetPhysics();
		if (physics)
		{
			int geometryCount = physics.GetNumGeoms();
			PrintFormat("GC | Killed From | event=clone-physics role=%1 entity=%2 bodyLayer=%3 simulation=%4 active=%5 dynamic=%6 kinematic=%7 geometries=%8", role, entity, physics.GetInteractionLayer(), physics.GetSimulationState(), physics.IsActive(), physics.IsDynamic(), physics.IsKinematic(), geometryCount);
			for (int geometryIndex; geometryIndex < geometryCount; geometryIndex++)
				PrintFormat("GC | Killed From | event=clone-physics-geometry role=%1 entity=%2 index=%3 name=%4 layer=%5", role, entity, geometryIndex, physics.GetGeomName(geometryIndex), physics.GetGeomInteractionLayer(geometryIndex));
		}
		IEntity child = entity.GetChildren();
		while (child)
		{
			ReportPhysicsHierarchy(child, role);
			child = child.GetSibling();
		}
	}

	protected void DestroyPreview()
	{
		if (m_KillerPreviewEntity)
		{
			IEntity killer = m_KillerPreviewEntity;
			m_KillerPreviewEntity = null;
			PrintFormat("GC | Killed From | event=preview-destroy role=killer preview=%1", killer);
			delete killer;
		}
		if (m_VictimPreviewEntity)
		{
			IEntity victim = m_VictimPreviewEntity;
			m_VictimPreviewEntity = null;
			PrintFormat("GC | Killed From | event=preview-destroy role=victim preview=%1", victim);
			delete victim;
		}
	}

	protected void Update()
	{
		m_aDrawCommands = {};
		if (!m_bEnabled || !m_Snapshot || !m_wCanvas || !m_Workspace || !m_World)
			return;

		// Character and inventory systems can create or rebuild physics after the
		// local prefab spawn. Reassert only the unused interaction layer; disabling
		// simulation or activity corrupts the character animation transform.
		IsolatePhysicsHierarchy(m_KillerPreviewEntity);
		IsolatePhysicsHierarchy(m_VictimPreviewEntity);
		ApplyCharacterState(m_KillerPreviewEntity, m_Snapshot.GetStance(), m_Snapshot.GetDynamicStance(), m_Snapshot.GetFlags());
		ApplyCharacterState(m_VictimPreviewEntity, m_Snapshot.GetVictimStance(), m_Snapshot.GetVictimDynamicStance(), m_Snapshot.GetVictimFlags());

		bool traceVisible;
		vector start = m_Workspace.ProjWorldToScreen(m_Snapshot.GetLineOrigin(), m_World);
		vector end = m_Workspace.ProjWorldToScreen(m_Snapshot.GetHitPosition(), m_World);
		if (start[2] > 0.0 && end[2] > 0.0)
		{
			traceVisible = true;
			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = Color.YELLOW;
			line.m_fWidth = LINE_WIDTH;
			line.m_bShouldEnclose = false;
			line.m_Vertices = {};
			line.m_Vertices.Insert(start[0]);
			line.m_Vertices.Insert(start[1]);
			line.m_Vertices.Insert(end[0]);
			line.m_Vertices.Insert(end[1]);
			m_aDrawCommands.Insert(line);
		}

		bool killerLabelVisible = DrawActorLabel(m_KillerPreviewEntity, "KILLED YOU", 0xe6a20f2d);
		bool victimLabelVisible = DrawActorLabel(m_VictimPreviewEntity, "YOU", 0xe63278b7);
		bool labelVisible = killerLabelVisible || victimLabelVisible;
		if (!m_bVisibilityReported || traceVisible != m_bLastTraceVisible || labelVisible != m_bLastLabelVisible)
		{
			PrintFormat("GC | Killed From | event=render-visibility traceVisible=%1 killerLabel=%2 victimLabel=%3 killerPreview=%4 victimPreview=%5 lineStartScreen=%6 lineEndScreen=%7", traceVisible, killerLabelVisible, victimLabelVisible, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null, start, end);
			m_bVisibilityReported = true;
			m_bLastTraceVisible = traceVisible;
			m_bLastLabelVisible = labelVisible;
		}
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected bool DrawActorLabel(IEntity actor, string text, int backgroundColor)
	{
		if (!actor)
			return false;

		vector labelWorldPosition = actor.GetOrigin();
		labelWorldPosition[1] = labelWorldPosition[1] + KILLER_LABEL_WORLD_OFFSET;
		vector labelPosition = m_Workspace.ProjWorldToScreen(labelWorldPosition, m_World);
		if (labelPosition[2] <= 0.0)
			return false;

		float halfWidth = KILLER_LABEL_WIDTH * 0.5;
		float halfHeight = KILLER_LABEL_HEIGHT * 0.5;
		PolygonDrawCommand background = new PolygonDrawCommand();
		background.m_iColor = backgroundColor;
		background.m_Vertices = {};
		background.m_Vertices.Insert(labelPosition[0] - halfWidth);
		background.m_Vertices.Insert(labelPosition[1] - halfHeight);
		background.m_Vertices.Insert(labelPosition[0] + halfWidth);
		background.m_Vertices.Insert(labelPosition[1] - halfHeight);
		background.m_Vertices.Insert(labelPosition[0] + halfWidth);
		background.m_Vertices.Insert(labelPosition[1] + halfHeight);
		background.m_Vertices.Insert(labelPosition[0] - halfWidth);
		background.m_Vertices.Insert(labelPosition[1] + halfHeight);
		m_aDrawCommands.Insert(background);

		TextDrawCommand label = new TextDrawCommand();
		label.m_iColor = Color.WHITE;
		label.m_Position = labelPosition;
		label.m_sText = text;
		label.m_fSize = 22.0;
		label.m_Pivot[0] = 0.5;
		label.m_Pivot[1] = 0.5;
		m_aDrawCommands.Insert(label);
		return true;
	}

	protected void ClearLine()
	{
		m_aDrawCommands = {};
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}
}
