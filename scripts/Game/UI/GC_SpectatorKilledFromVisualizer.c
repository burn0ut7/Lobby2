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
	protected IEntity m_PreviewEntity;
	protected bool m_bEnabled;
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world, GC_KilledFromSnapshot snapshot, bool enabled)
	{
		if (!canvas || !workspace || !world)
			return false;

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_Snapshot = snapshot;
		SetEnabled(enabled);
		return true;
	}

	void SetSnapshot(GC_KilledFromSnapshot snapshot)
	{
		m_Snapshot = snapshot;
		DestroyPreview();
		if (m_bEnabled)
			CreatePreview();
	}

	void SetEnabled(bool enabled)
	{
		if (m_bEnabled == enabled)
			return;

		m_bEnabled = enabled;
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
		if (m_PreviewEntity || !m_Snapshot || !m_Snapshot.HasKillerPreview())
			return;

		Resource resource = Resource.Load(m_Snapshot.GetKillerPrefab());
		if (!resource)
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(m_Snapshot.GetKillerAngles(), params.Transform);
		params.Transform[3] = m_Snapshot.GetKillerPosition();
		m_PreviewEntity = GetGame().SpawnEntityPrefabLocal(resource, m_World, params);
		if (!m_PreviewEntity)
			return;

		// The preview is presentation only. Disable every physics object in the
		// hierarchy so the cloned character, weapon, vehicle, or turret cannot
		// participate in the live world's simulation or collision layers.
		DisablePhysicsHierarchy(m_PreviewEntity);
		ChimeraWorld world = ChimeraWorld.CastFrom(m_World);
		if (world)
			world.OutlineEntity(m_PreviewEntity, Color.FromRGBA(235, 45, 70, 255), 1.0, 0.2, true, 3);

		AIControlComponent aiControl = AIControlComponent.Cast(m_PreviewEntity.FindComponent(AIControlComponent));
		if (aiControl)
			aiControl.DeactivateAI();

		ChimeraCharacter character = ChimeraCharacter.Cast(m_PreviewEntity);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		// ECharacterStance values are zero-based while ForceStance consumes the
		// corresponding stance-change value (erect/crouch/prone are 1/2/3).
		controller.ForceStance(m_Snapshot.GetStance() + 1);
		if (controller.CanSetDynamicStance(m_Snapshot.GetDynamicStance()))
			controller.SetDynamicStance(m_Snapshot.GetDynamicStance());
		controller.SetWeaponRaised((m_Snapshot.GetFlags() & GC_KilledFromSnapshot.FLAG_WEAPON_RAISED) != 0);
		controller.SetWeaponADS((m_Snapshot.GetFlags() & GC_KilledFromSnapshot.FLAG_WEAPON_ADS) != 0);
	}

	protected void DisablePhysicsHierarchy(IEntity entity)
	{
		if (!entity)
			return;

		Physics physics = entity.GetPhysics();
		if (physics)
		{
			physics.SetActive(ActiveState.INACTIVE);
			physics.ChangeSimulationState(SimulationState.NONE);
			physics.SetInteractionLayer(EPhysicsLayerDefs.Unused);
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity sibling = child.GetSibling();
			DisablePhysicsHierarchy(child);
			child = sibling;
		}
	}

	protected void DestroyPreview()
	{
		if (!m_PreviewEntity)
			return;

		IEntity preview = m_PreviewEntity;
		m_PreviewEntity = null;
		delete preview;
	}

	protected void Update()
	{
		m_aDrawCommands = {};
		if (!m_bEnabled || !m_Snapshot || !m_wCanvas || !m_Workspace || !m_World)
			return;

		vector start = m_Workspace.ProjWorldToScreen(m_Snapshot.GetLineOrigin(), m_World);
		vector end = m_Workspace.ProjWorldToScreen(m_Snapshot.GetHitPosition(), m_World);
		if (start[2] > 0.0 && end[2] > 0.0)
		{
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

		DrawKillerLabel();
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected void DrawKillerLabel()
	{
		if (!m_PreviewEntity)
			return;

		vector labelWorldPosition = m_PreviewEntity.GetOrigin();
		labelWorldPosition[1] = labelWorldPosition[1] + KILLER_LABEL_WORLD_OFFSET;
		vector labelPosition = m_Workspace.ProjWorldToScreen(labelWorldPosition, m_World);
		if (labelPosition[2] <= 0.0)
			return;

		float halfWidth = KILLER_LABEL_WIDTH * 0.5;
		float halfHeight = KILLER_LABEL_HEIGHT * 0.5;
		PolygonDrawCommand background = new PolygonDrawCommand();
		background.m_iColor = 0xe6a20f2d;
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
		label.m_sText = "KILLED YOU";
		label.m_fSize = 22.0;
		label.m_Pivot[0] = 0.5;
		label.m_Pivot[1] = 0.5;
		m_aDrawCommands.Insert(label);
	}

	protected void ClearLine()
	{
		m_aDrawCommands = {};
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}
}
