//! Client-only killed-from preview and line renderer. No entity created here is
//! inserted on the server or exposed as replicated gameplay state.
class GC_SpectatorKilledFromVisualizer
{
	protected static const float LINE_WIDTH = 3.0;
	protected static const float KILLER_LABEL_WIDTH = 160.0;
	protected static const float KILLER_LABEL_HEIGHT = 36.0;
	protected static const float KILLER_LABEL_WORLD_OFFSET = 2.2;
	protected static const ResourceName PREVIEW_MATERIAL = "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat";
	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected SCR_BasePreviewEntity m_KillerPreviewEntity;
	protected SCR_BasePreviewEntity m_VictimPreviewEntity;
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
		m_KillerPreviewEntity = CreateActorPreview(m_Snapshot.GetKillerVisualObject(), m_Snapshot.GetKillerPreviewEntries(), killerTransform, m_Snapshot.GetKillerBonePose(), Color.FromRGBA(235, 45, 70, 255), "killer");

		vector victimTransform[4];
		m_Snapshot.GetVictimRootTransform(victimTransform);
		m_VictimPreviewEntity = CreateActorPreview(m_Snapshot.GetVictimVisualObject(), m_Snapshot.GetVictimPreviewEntries(), victimTransform, m_Snapshot.GetVictimBonePose(), Color.FromRGBA(65, 170, 255, 255), "victim");
		PrintFormat("GC | Killed From | event=preview-create result=complete killer=%1 victim=%2 snapshot={%3}", m_KillerPreviewEntity, m_VictimPreviewEntity, m_Snapshot.GetDebugReport());
	}

	protected SCR_BasePreviewEntity CreateActorPreview(ResourceName visualObject, array<ref SCR_BasePreviewEntry> entries, vector rootTransform[4], array<ref GC_KilledFromBonePose> bonePose, Color outlineColor, string role)
	{
		if (visualObject == ResourceName.Empty || !entries || entries.IsEmpty())
		{
			PrintFormat("GC | Killed From | event=actor-preview result=skipped role=%1 stage=hierarchy-baseline reason=no-client-visual", role, level: LogLevel.WARNING);
			return null;
		}
		foreach (int visualPartIndex, SCR_BasePreviewEntry visualPart : entries)
		{
			PrintFormat("GC | Killed From | event=visual-part role=%1 index=%2 parent=%3 pivot=%4 mesh=%5 shape=%6", role, visualPartIndex, visualPart.m_iParentID, visualPart.m_iPivotID, visualPart.m_Mesh, visualPart.m_Shape);
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixCopy(rootTransform, params.Transform);
		SCR_BasePreviewEntity preview = SCR_BasePreviewEntity.SpawnPreview(entries, "SCR_BasePreviewEntity", m_World, params, PREVIEW_MATERIAL);
		if (!preview)
		{
			PrintFormat("GC | Killed From | event=actor-preview result=failed role=%1 stage=hierarchy-baseline reason=spawn sourceObject=%2 visualParts=%3", role, visualObject, entries.Count(), level: LogLevel.ERROR);
			return null;
		}

		ChimeraWorld world = ChimeraWorld.CastFrom(m_World);
		if (world)
		{
			world.OutlineEntity(preview, outlineColor, 1.0, 0.2, true, 3);
			PrintFormat("GC | Killed From | event=preview-outline result=applied role=%1 preview=%2 includeChildren=1 thickness=3", role, preview);
		}
		else
			PrintFormat("GC | Killed From | event=preview-outline result=failed role=%1 reason=world-cast", role, level: LogLevel.ERROR);

		int capturedBones;
		int capturedLocalBones;
		int matchingBones;
		int missingBones;
		int targetBones;
		if (bonePose)
			capturedBones = bonePose.Count();
		IEntity skeletalEntity;
		array<SCR_BasePreviewEntity> previewChildren = preview.GetPreviewChildren();
		if (previewChildren && !previewChildren.IsEmpty())
			skeletalEntity = previewChildren[0];
		Animation targetAnimation;
		if (skeletalEntity)
			targetAnimation = skeletalEntity.GetAnimation();
		if (targetAnimation)
		{
			array<string> targetBoneNames = {};
			targetAnimation.GetBoneNames(targetBoneNames);
			targetBones = targetBoneNames.Count();
		}
		if (bonePose)
		{
			foreach (GC_KilledFromBonePose pose : bonePose)
			{
				if (pose.m_bHasLocalTransform)
					capturedLocalBones++;
				if (targetAnimation)
				{
					TNodeId targetBone = targetAnimation.GetBoneIndex(pose.m_sBoneName);
					if (targetBone == -1)
						missingBones++;
					else
						matchingBones++;
				}
			}
		}

		int entryCount;
		if (entries)
			entryCount = entries.Count();
		ResourceName targetObject;
		VObject previewObject;
		if (skeletalEntity)
			previewObject = skeletalEntity.GetVObject();
		if (previewObject)
			targetObject = previewObject.GetResourceName();
		Physics previewPhysics;
		if (skeletalEntity)
			previewPhysics = skeletalEntity.GetPhysics();
		PrintFormat("GC | Killed From | event=actor-preview result=spawned role=%1 stage=hierarchy-material-baseline preview=%2 sourceObject=%3 targetObject=%4 visualParts=%5 physics=%6 sourceMutation=none exactWorldTransform=%7", role, preview, visualObject, targetObject, entryCount, previewPhysics, rootTransform);
		PrintFormat("GC | Killed From | event=actor-preview-compat role=%1 stage=hierarchy-baseline capturedModelBones=%2 capturedLocalBones=%3 targetBones=%4 matchingBones=%5 missingBones=%6 applyBones=0", role, capturedBones, capturedLocalBones, targetBones, matchingBones, missingBones);
		return preview;
	}

	protected void DestroyPreview()
	{
		if (m_KillerPreviewEntity)
		{
			SCR_BasePreviewEntity killer = m_KillerPreviewEntity;
			m_KillerPreviewEntity = null;
			PrintFormat("GC | Killed From | event=preview-destroy role=killer preview=%1", killer);
			delete killer;
		}
		if (m_VictimPreviewEntity)
		{
			SCR_BasePreviewEntity victim = m_VictimPreviewEntity;
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
