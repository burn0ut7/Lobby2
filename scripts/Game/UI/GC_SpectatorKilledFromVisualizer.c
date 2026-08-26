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
	protected IEntity m_KillerVehiclePreviewEntity;
	protected IEntity m_VictimVehiclePreviewEntity;
	protected bool m_bEnabled;
	protected bool m_bVisibilityReported;
	protected bool m_bPhysicsDeactivationReported;
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
		m_bPhysicsDeactivationReported = false;
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
		if (m_KillerPreviewEntity || m_VictimPreviewEntity || m_KillerVehiclePreviewEntity || m_VictimVehiclePreviewEntity)
		{
			PrintFormat("GC | Killed From | event=preview-create result=ignored reason=already-exists killer=%1 victim=%2 killerVehicle=%3 victimVehicle=%4", m_KillerPreviewEntity, m_VictimPreviewEntity, m_KillerVehiclePreviewEntity, m_VictimVehiclePreviewEntity);
			return;
		}
		if (!m_Snapshot)
		{
			Print("GC | Killed From | event=preview-create result=rejected reason=no-snapshot", LogLevel.WARNING);
			return;
		}
		GC_KilledFromVehicleState killerVehicle = m_Snapshot.GetKillerVehicle();
		if (killerVehicle && killerVehicle.IsValid())
			m_KillerVehiclePreviewEntity = CreateVehicleClone(killerVehicle, Color.FromRGBA(235, 45, 70, 255), "killer-vehicle");
		GC_KilledFromVehicleState victimVehicle = m_Snapshot.GetVictimVehicle();
		if (victimVehicle && victimVehicle.IsValid())
			m_VictimVehiclePreviewEntity = CreateVehicleClone(victimVehicle, Color.FromRGBA(65, 170, 255, 255), "victim-vehicle");

		vector killerTransform[4];
		m_Snapshot.GetKillerRootTransform(killerTransform);
		m_KillerPreviewEntity = CreateActorClone(m_Snapshot.GetKillerPrefab(), killerTransform, m_Snapshot.GetStance(), m_Snapshot.GetDynamicStance(), m_Snapshot.GetFlags(), m_Snapshot.GetKillerLean(), m_Snapshot.GetKillerAimingAngles(), m_Snapshot.GetKillerBonePose(), Color.FromRGBA(235, 45, 70, 255), "killer");

		vector victimTransform[4];
		m_Snapshot.GetVictimRootTransform(victimTransform);
		m_VictimPreviewEntity = CreateActorClone(m_Snapshot.GetVictimPrefab(), victimTransform, m_Snapshot.GetVictimStance(), m_Snapshot.GetVictimDynamicStance(), m_Snapshot.GetVictimFlags(), m_Snapshot.GetVictimLean(), m_Snapshot.GetVictimAimingAngles(), m_Snapshot.GetVictimBonePose(), Color.FromRGBA(65, 170, 255, 255), "victim");
		EnsureActorSeat(m_KillerPreviewEntity, m_KillerVehiclePreviewEntity, killerVehicle, "killer");
		EnsureActorSeat(m_VictimPreviewEntity, m_VictimVehiclePreviewEntity, victimVehicle, "victim");
		m_bPhysicsDeactivationReported = false;
		PrintFormat("GC | Killed From | event=preview-create result=complete killer=%1 victim=%2 killerVehicle=%3 victimVehicle=%4 snapshot={%5}", m_KillerPreviewEntity, m_VictimPreviewEntity, m_KillerVehiclePreviewEntity, m_VictimVehiclePreviewEntity, m_Snapshot.GetDebugReport());
	}

	protected IEntity CreateVehicleClone(GC_KilledFromVehicleState state, Color outlineColor, string role)
	{
		if (!state || !state.IsValid())
			return null;
		Resource resource = Resource.Load(state.GetPrefab());
		if (!resource || !resource.IsValid())
		{
			PrintFormat("GC | Killed From | event=vehicle-clone result=failed role=%1 stage=resource prefab=%2", role, state.GetPrefab(), level: LogLevel.ERROR);
			return null;
		}
		vector rootTransform[4];
		state.GetRootTransform(rootTransform);
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixCopy(rootTransform, params.Transform);
		IEntity preview = GetGame().SpawnEntityPrefabLocal(resource, m_World, params);
		if (!preview)
		{
			PrintFormat("GC | Killed From | event=vehicle-clone result=failed role=%1 stage=spawn prefab=%2 transform=%3", role, state.GetPrefab(), rootTransform, level: LogLevel.ERROR);
			return null;
		}
		AIControlComponent aiControl = AIControlComponent.Cast(preview.FindComponent(AIControlComponent));
		if (aiControl)
			aiControl.DeactivateAI();
		int isolatedPhysics = IsolatePhysicsHierarchy(preview);
		ChimeraWorld world = ChimeraWorld.CastFrom(m_World);
		if (world)
			world.OutlineEntity(preview, outlineColor, 1.0, 0.2, true, 3);
		vector readback[4];
		preview.GetWorldTransform(readback);
		PrintFormat("GC | Killed From | event=vehicle-clone result=spawned role=%1 preview=%2 prefab=%3 isolatedPhysics=%4 turrets=%5 requested=%6 readback=%7 sourceMutation=none", role, preview, state.GetPrefab(), isolatedPhysics, state.GetTurrets().Count(), rootTransform, readback);
		return preview;
	}

	protected bool EnsureActorSeat(IEntity actor, IEntity vehicle, GC_KilledFromVehicleState state, string role)
	{
		if (!actor || !vehicle || !state || !state.HasOccupiedCompartment())
			return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(actor);
		if (!character)
			return false;
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!access || !manager)
			return false;
		array<BaseCompartmentSlot> compartments = {};
		manager.GetCompartmentsOfType(compartments, state.GetCompartmentType());
		int compartmentIndex = state.GetCompartmentIndex();
		if (compartmentIndex < 0 || compartmentIndex >= compartments.Count())
		{
			PrintFormat("GC | Killed From | event=vehicle-seat result=failed role=%1 reason=index type=%2 index=%3 available=%4", role, state.GetCompartmentType(), compartmentIndex, compartments.Count(), level: LogLevel.ERROR);
			return false;
		}
		BaseCompartmentSlot targetCompartment = compartments[compartmentIndex];
		if (access.GetVehicleCompartmentManagerOwner() == vehicle && access.GetCompartment() == targetCompartment)
			return true;
		bool entered = access.GetInVehicle(vehicle, targetCompartment, true, 0, ECloseDoorAfterActions.CLOSE_DOOR, false);
		PrintFormat("GC | Killed From | event=vehicle-seat role=%1 result=%2 actor=%3 vehicle=%4 type=%5 index=%6 target=%7 actualVehicle=%8 actualCompartment=%9", role, entered, actor, vehicle, state.GetCompartmentType(), compartmentIndex, targetCompartment, access.GetVehicleCompartmentManagerOwner(), access.GetCompartment());
		return entered;
	}

	protected void ApplyVehicleState(IEntity vehicle, GC_KilledFromVehicleState state, string role)
	{
		if (!vehicle || !state)
			return;
		array<TurretControllerComponent> controllers = {};
		array<IEntity> hierarchy = {vehicle};
		SCR_EntityHelper.GetHierarchyEntityList(vehicle, hierarchy);
		foreach (IEntity hierarchyEntity : hierarchy)
		{
			array<Managed> components = {};
			hierarchyEntity.FindComponents(TurretControllerComponent, components);
			foreach (Managed component : components)
			{
				TurretControllerComponent controller = TurretControllerComponent.Cast(component);
				if (controller)
					controllers.Insert(controller);
			}
		}
		array<ref GC_KilledFromTurretState> states = state.GetTurrets();
		int applyCount = Math.Min(controllers.Count(), states.Count());
		for (int index; index < applyCount; index++)
		{
			vector rawRotation = states[index].GetRawAimingRotation();
			bool ads = states[index].IsWeaponADS();
			controllers[index].SetAimingAngles(rawRotation[0], rawRotation[1]);
			controllers[index].SetWeaponADSInput(ads);
			controllers[index].SetWeaponADS(ads);
			controllers[index].SetCurrentSightsADS(ads);
		}
	}

	protected void ReportVehicleState(IEntity vehicle, GC_KilledFromVehicleState state, string role)
	{
		if (!vehicle || !state)
			return;
		array<ref GC_KilledFromTurretState> states = state.GetTurrets();
		int turretIndex;
		array<IEntity> hierarchy = {vehicle};
		SCR_EntityHelper.GetHierarchyEntityList(vehicle, hierarchy);
		foreach (IEntity hierarchyEntity : hierarchy)
		{
			array<Managed> components = {};
			hierarchyEntity.FindComponents(TurretControllerComponent, components);
			foreach (Managed component : components)
			{
				TurretControllerComponent controller = TurretControllerComponent.Cast(component);
				if (!controller)
					continue;
				vector requestedRotation;
				bool requestedADS;
				if (turretIndex < states.Count())
				{
					requestedRotation = states[turretIndex].GetRawAimingRotation();
					requestedADS = states[turretIndex].IsWeaponADS();
				}
				TurretComponent turret = controller.GetTurretComponent();
				vector actualRotation;
				if (turret)
					actualRotation = turret.GetRawAimingRotation();
				PrintFormat("GC | Killed From | event=vehicle-turret-state role=%1 index=%2 controller=%3 requestedRotation=%4 actualRotation=%5 requestedADS=%6 actualADS=%7 inputADS=%8 sightsADS=%9", role, turretIndex, controller, requestedRotation, actualRotation, requestedADS, controller.IsWeaponADS(), controller.GetWeaponADSInput(), controller.GetCurrentSightsADS());
				turretIndex++;
			}
		}
		PrintFormat("GC | Killed From | event=vehicle-state role=%1 vehicle=%2 capturedTurrets=%3 cloneTurrets=%4 compartmentType=%5 compartmentIndex=%6", role, vehicle, states.Count(), turretIndex, state.GetCompartmentType(), state.GetCompartmentIndex());
	}

	protected IEntity CreateActorClone(ResourceName prefab, vector rootTransform[4], int stance, float dynamicStance, int flags, float lean, vector aimingAngles, array<ref GC_KilledFromBonePose> bonePose, Color outlineColor, string role)
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
		ApplyCharacterState(preview, stance, dynamicStance, flags, lean, aimingAngles);
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

	protected void ApplyCharacterState(IEntity entity, int stance, float dynamicStance, int flags, float lean, vector aimingAngles)
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
		bool ads = (flags & GC_KilledFromSnapshot.FLAG_WEAPON_ADS) != 0;
		controller.SetWeaponRaised((flags & GC_KilledFromSnapshot.FLAG_WEAPON_RAISED) != 0 || ads);
		controller.SetWeaponADS(ads);
		CharacterInputContext inputContext = controller.GetInputContext();
		if (inputContext)
		{
			inputContext.SetLean(lean);
			inputContext.SetAimingAngles(aimingAngles);
			inputContext.SetWeaponADS(ads);
		}
	}

	protected void ReportCharacterState(IEntity entity, string role, float requestedLean, vector requestedAimingAngles, int flags)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;
		CharacterInputContext inputContext = controller.GetInputContext();
		if (!inputContext)
		{
			PrintFormat("GC | Killed From | event=clone-character-state role=%1 result=no-input-context requestedLean=%2 requestedAim=%3 requestedADS=%4", role, requestedLean, requestedAimingAngles, (flags & GC_KilledFromSnapshot.FLAG_WEAPON_ADS) != 0, level: LogLevel.WARNING);
			return;
		}
		PrintFormat("GC | Killed From | event=clone-character-state role=%1 requestedLean=%2 inputLean=%3 currentLean=%4 requestedAim=%5 inputAim=%6 requestedADS=%7 controllerADS=%8 inputADS=%9 weaponRaised=%10",
			role, requestedLean, inputContext.GetLean(), controller.GetCurrentLeanAmount(), requestedAimingAngles, inputContext.GetAimingAngles(),
			(flags & GC_KilledFromSnapshot.FLAG_WEAPON_ADS) != 0, controller.IsWeaponADS(), inputContext.IsWeaponADS(), controller.IsWeaponRaised());
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

	protected int DeactivatePhysicsHierarchy(IEntity entity)
	{
		if (!entity)
			return 0;
		int count;
		Physics physics = entity.GetPhysics();
		if (physics)
		{
			physics.SetActive(ActiveState.INACTIVE);
			count++;
		}
		IEntity child = entity.GetChildren();
		while (child)
		{
			count += DeactivatePhysicsHierarchy(child);
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
		if (m_KillerVehiclePreviewEntity)
		{
			IEntity killerVehicle = m_KillerVehiclePreviewEntity;
			m_KillerVehiclePreviewEntity = null;
			PrintFormat("GC | Killed From | event=preview-destroy role=killer-vehicle preview=%1", killerVehicle);
			delete killerVehicle;
		}
		if (m_VictimVehiclePreviewEntity)
		{
			IEntity victimVehicle = m_VictimVehiclePreviewEntity;
			m_VictimVehiclePreviewEntity = null;
			PrintFormat("GC | Killed From | event=preview-destroy role=victim-vehicle preview=%1", victimVehicle);
			delete victimVehicle;
		}
		m_bPhysicsDeactivationReported = false;
	}

	protected void Update()
	{
		m_aDrawCommands = {};
		if (!m_bEnabled || !m_Snapshot || !m_wCanvas || !m_Workspace || !m_World)
			return;

		// Character and inventory initialization can rebuild physics after spawn.
		// Reassert unused layers and the verified ALL-OFF treatment every update;
		// the configured simulation state remains unchanged.
		IsolatePhysicsHierarchy(m_KillerPreviewEntity);
		IsolatePhysicsHierarchy(m_VictimPreviewEntity);
		IsolatePhysicsHierarchy(m_KillerVehiclePreviewEntity);
		IsolatePhysicsHierarchy(m_VictimVehiclePreviewEntity);
		ApplyCharacterState(m_KillerPreviewEntity, m_Snapshot.GetStance(), m_Snapshot.GetDynamicStance(), m_Snapshot.GetFlags(), m_Snapshot.GetKillerLean(), m_Snapshot.GetKillerAimingAngles());
		ApplyCharacterState(m_VictimPreviewEntity, m_Snapshot.GetVictimStance(), m_Snapshot.GetVictimDynamicStance(), m_Snapshot.GetVictimFlags(), m_Snapshot.GetVictimLean(), m_Snapshot.GetVictimAimingAngles());
		ApplyVehicleState(m_KillerVehiclePreviewEntity, m_Snapshot.GetKillerVehicle(), "killer");
		ApplyVehicleState(m_VictimVehiclePreviewEntity, m_Snapshot.GetVictimVehicle(), "victim");
		int killerDeactivatedPhysics = DeactivatePhysicsHierarchy(m_KillerPreviewEntity);
		int victimDeactivatedPhysics = DeactivatePhysicsHierarchy(m_VictimPreviewEntity);
		int killerVehicleDeactivatedPhysics = DeactivatePhysicsHierarchy(m_KillerVehiclePreviewEntity);
		int victimVehicleDeactivatedPhysics = DeactivatePhysicsHierarchy(m_VictimVehiclePreviewEntity);

		if (!m_bPhysicsDeactivationReported)
		{
			ReportCharacterState(m_KillerPreviewEntity, "killer", m_Snapshot.GetKillerLean(), m_Snapshot.GetKillerAimingAngles(), m_Snapshot.GetFlags());
			ReportCharacterState(m_VictimPreviewEntity, "victim", m_Snapshot.GetVictimLean(), m_Snapshot.GetVictimAimingAngles(), m_Snapshot.GetVictimFlags());
			ReportVehicleState(m_KillerVehiclePreviewEntity, m_Snapshot.GetKillerVehicle(), "killer");
			ReportVehicleState(m_VictimVehiclePreviewEntity, m_Snapshot.GetVictimVehicle(), "victim");
			PrintFormat("GC | Killed From | event=clone-physics-deactivation treatment=ALL-OFF killerBodies=%1 victimBodies=%2 killerVehicleBodies=%3 victimVehicleBodies=%4", killerDeactivatedPhysics, victimDeactivatedPhysics, killerVehicleDeactivatedPhysics, victimVehicleDeactivatedPhysics);
			ReportPhysicsHierarchy(m_KillerPreviewEntity, "killer-post-deactivation");
			ReportPhysicsHierarchy(m_VictimPreviewEntity, "victim-post-deactivation");
			ReportPhysicsHierarchy(m_KillerVehiclePreviewEntity, "killer-vehicle-post-deactivation");
			ReportPhysicsHierarchy(m_VictimVehiclePreviewEntity, "victim-vehicle-post-deactivation");
			m_bPhysicsDeactivationReported = true;
		}

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

		IEntity killerLabelEntity = m_KillerPreviewEntity;
		if (m_KillerVehiclePreviewEntity)
			killerLabelEntity = m_KillerVehiclePreviewEntity;
		IEntity victimLabelEntity = m_VictimPreviewEntity;
		if (m_VictimVehiclePreviewEntity)
			victimLabelEntity = m_VictimVehiclePreviewEntity;
		bool killerLabelVisible = DrawActorLabel(killerLabelEntity, "KILLED YOU", 0xe6a20f2d);
		bool victimLabelVisible = DrawActorLabel(victimLabelEntity, "YOU", 0xe63278b7);
		bool labelVisible = killerLabelVisible || victimLabelVisible;
		if (!m_bVisibilityReported || traceVisible != m_bLastTraceVisible || labelVisible != m_bLastLabelVisible)
		{
			PrintFormat("GC | Killed From | event=render-visibility traceVisible=%1 killerLabel=%2 victimLabel=%3 killerPreview=%4 victimPreview=%5 killerVehicle=%6 victimVehicle=%7 lineStartScreen=%8 lineEndScreen=%9", traceVisible, killerLabelVisible, victimLabelVisible, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null, m_KillerVehiclePreviewEntity != null, m_VictimVehiclePreviewEntity != null, start, end);
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
