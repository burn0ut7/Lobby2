class GC_KilledFromBoneOperation
{
	TNodeId m_iBoneIndex;
	vector m_aMatrix[4];
}

enum GC_EKilledFromWeaponPrototype
{
	LIVE_CONTROL,
	POSE_THEN_FREEZE,
	GAMEPLAY_SHARED_ADS_AIM
}

//! Client-only killed-from preview and line renderer. No entity created here is
//! inserted on the server or exposed as replicated gameplay state.
class GC_SpectatorKilledFromVisualizer
{
	protected static const float LINE_WIDTH = 1.5;
	protected static const float KILLER_LABEL_WORLD_OFFSET = 2.2;
	protected static const float MARKER_WIDTH = 256.0;
	protected static const float MARKER_HEIGHT = 76.0;
	protected static const ResourceName MARKER_LAYOUT = "{6A5F000000000100}UI/layouts/Spectator/Lobby_Spectator_Killed_From_Marker.layout";
	protected CanvasWidget m_wCanvas;
	protected Widget m_wKillerMarker;
	protected Widget m_wVictimMarker;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected IEntity m_KillerPreviewEntity;
	protected IEntity m_VictimPreviewEntity;
	protected IEntity m_KillerVehiclePreviewEntity;
	protected IEntity m_VictimVehiclePreviewEntity;
	protected IEntity m_MuzzleWeapon;
	protected vector m_vMuzzlePosition;
	protected string m_sMuzzleMode;
	protected bool m_bMuzzleResolved;
	protected bool m_bVictimFrozenPreview;
	protected bool m_bEnabled;
	protected bool m_bVisibilityReported;
	protected bool m_bPhysicsDeactivationReported;
	protected bool m_bLastTraceVisible;
	protected bool m_bLastLabelVisible;
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, Widget markerParent, WorkspaceWidget workspace, BaseWorld world, GC_KilledFromSnapshot snapshot, bool enabled)
	{
		if (!canvas || !markerParent || !workspace || !world)
		{
			PrintFormat("GC | Killed From | event=visualizer-start result=rejected canvas=%1 markerParent=%2 workspace=%3 world=%4", canvas, markerParent, workspace, world, level: LogLevel.ERROR);
			return false;
		}

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_wKillerMarker = workspace.CreateWidgets(MARKER_LAYOUT, markerParent);
		m_wVictimMarker = workspace.CreateWidgets(MARKER_LAYOUT, markerParent);
		if (!m_wKillerMarker || !m_wVictimMarker)
		{
			DestroyMarkers();
			Print("GC | Killed From | event=visualizer-start result=rejected reason=marker-layout", LogLevel.ERROR);
			return false;
		}
		ConfigureMarker(m_wKillerMarker, "KILLED YOU", Color.FromRGBA(235, 45, 70, 255));
		ConfigureMarker(m_wVictimMarker, "YOU", Color.FromRGBA(65, 170, 255, 255));
		m_Snapshot = snapshot;
		UpdateKillerMarkerText();
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
		UpdateKillerMarkerText();
		m_bVisibilityReported = false;
		m_bPhysicsDeactivationReported = false;
		DestroyPreview();
		ClearLine();
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
		DestroyMarkers();
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
		if (killerVehicle && killerVehicle.IsValid())
			m_KillerPreviewEntity = CreateActorClone(m_Snapshot.GetKillerPrefab(), killerTransform, m_Snapshot.GetKillerBonePose(), Color.FromRGBA(235, 45, 70, 255), "killer");
		else
			m_KillerPreviewEntity = CreateExperimentalActorClone(m_Snapshot.GetKillerPrefab(), killerTransform, m_Snapshot.GetKillerBonePose(), m_Snapshot.GetKillerWeapon(), GC_EKilledFromWeaponPrototype.LIVE_CONTROL, Color.FromRGBA(235, 45, 70, 255), "killer-k0");

		vector victimTransform[4];
		m_Snapshot.GetVictimRootTransform(victimTransform);
		if (!victimVehicle || !victimVehicle.IsValid())
			m_VictimPreviewEntity = CreateExperimentalActorClone(m_Snapshot.GetVictimPrefab(), victimTransform, m_Snapshot.GetVictimBonePose(), m_Snapshot.GetVictimWeapon(), GC_EKilledFromWeaponPrototype.POSE_THEN_FREEZE, Color.FromRGBA(65, 170, 255, 255), "victim-frozen");
		if (m_VictimPreviewEntity)
			m_bVictimFrozenPreview = true;
		else
			m_VictimPreviewEntity = CreateActorClone(m_Snapshot.GetVictimPrefab(), victimTransform, m_Snapshot.GetVictimBonePose(), Color.FromRGBA(65, 170, 255, 255), "victim-fallback");
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

	protected IEntity CreateExperimentalActorClone(ResourceName prefab, vector rootTransform[4], array<ref GC_KilledFromBonePose> bonePose, GC_KilledFromWeaponState weaponState, int mode, Color outlineColor, string role)
	{
		IEntity preview = CreateActorClone(prefab, rootTransform, bonePose, outlineColor, role);
		if (!preview)
			return null;

		IEntity weapon = SelectWeapon(preview, weaponState, role, true);
		preview.Update();
		CharacterAnimationComponent characterAnimation = CharacterAnimationComponent.Cast(preview.FindComponent(CharacterAnimationComponent));
		int missingBones;
		int failedBones;
		int unavailableLocalBones;
		int appliedBones;
		bool animationDeactivated;
		if (mode == GC_EKilledFromWeaponPrototype.LIVE_CONTROL)
			ApplyWeaponAnimationGraph(preview, weaponState, mode, role, true);

		appliedBones = ApplyBonePose(preview, bonePose, missingBones, failedBones, unavailableLocalBones);

		if (mode == GC_EKilledFromWeaponPrototype.POSE_THEN_FREEZE && characterAnimation)
		{
			characterAnimation.Deactivate(preview);
			animationDeactivated = true;
		}
		if (mode == GC_EKilledFromWeaponPrototype.LIVE_CONTROL)
			ApplyWeaponTransform(preview, weapon, weaponState, role, true);
		IsolatePhysicsHierarchy(weapon);
		int deactivatedPhysics = DeactivatePhysicsHierarchy(preview);
		PrintFormat("GC | Killed From | event=animation-treatment-create role=%1 index=%2 mode=%3 actor=%4 weapon=%5 animation=%6", role, mode, GetWeaponPrototypeName(mode), preview, weapon, characterAnimation);
		PrintFormat("GC | Killed From | event=animation-treatment-state role=%1 index=%2 mode=%3 animationOff=%4", role, mode, GetWeaponPrototypeName(mode), animationDeactivated);
		PrintFormat("GC | Killed From | event=animation-treatment-pose role=%1 index=%2 mode=%3 appliedBones=%4 missingBones=%5 failedBones=%6 unavailableLocal=%7 deactivatedPhysics=%8 sourceMutation=none", role, mode, GetWeaponPrototypeName(mode), appliedBones, missingBones, failedBones, unavailableLocalBones, deactivatedPhysics);
		return preview;
	}

	protected IEntity CreateActorClone(ResourceName prefab, vector rootTransform[4], array<ref GC_KilledFromBonePose> bonePose, Color outlineColor, string role)
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
		PrintFormat("GC | Killed From | event=actor-clone result=spawned role=%1 preview=%2 prefab=%3 isolatedPhysics=%4 capturedBones=%5 boneReplay=scheduled sourceMutation=none", role, preview, prefab, isolatedPhysics, capturedBones);
		PrintFormat("GC | Killed From | event=actor-clone-transform role=%1 requested=%2 readback=%3", role, rootTransform, transformReadback);
		return preview;
	}

	//! K2 treatment: precompute each source target relative to the clone's evaluated
	//! local bone matrix, then submit every delta together so parent writes cannot
	//! contaminate child calculations.
	protected int ApplyBonePose(IEntity entity, array<ref GC_KilledFromBonePose> bonePose, out int missingBones, out int failedBones, out int unavailableLocalBones)
	{
		missingBones = 0;
		failedBones = 0;
		unavailableLocalBones = 0;
		if (!entity || !bonePose)
			return 0;
		Animation animation = entity.GetAnimation();
		if (!animation)
			return 0;
		ref array<ref GC_KilledFromBoneOperation> operations = {};
		foreach (GC_KilledFromBonePose pose : bonePose)
		{
			if (!pose)
				continue;
			TNodeId boneIndex = animation.GetBoneIndex(pose.m_sBoneName);
			if (boneIndex < 0)
			{
				missingBones++;
				continue;
			}
			vector currentTransform[4];
			if (!pose.m_bHasLocalTransform || !animation.GetBoneLocalMatrix(boneIndex, currentTransform))
			{
				unavailableLocalBones++;
				continue;
			}

			GC_KilledFromBoneOperation operation = new GC_KilledFromBoneOperation();
			operation.m_iBoneIndex = boneIndex;
			Math3D.MatrixInvMultiply4(currentTransform, pose.m_aLocalTransform, operation.m_aMatrix);
			operations.Insert(operation);
		}

		foreach (GC_KilledFromBoneOperation operation : operations)
		{
			if (!animation.SetBoneMatrix(entity, operation.m_iBoneIndex, operation.m_aMatrix))
			{
				failedBones++;
				continue;
			}
		}
		entity.Update();
		return operations.Count() - failedBones;
	}

	protected IEntity SelectWeapon(IEntity actor, GC_KilledFromWeaponState state, string role, bool report)
	{
		if (!actor || !state || !state.IsValid())
			return null;
		CharacterWeaponManagerComponent manager = CharacterWeaponManagerComponent.Cast(actor.FindComponent(CharacterWeaponManagerComponent));
		if (!manager)
			return null;
		array<IEntity> weapons = {};
		manager.GetWeaponsList(weapons);
		IEntity selectedWeapon;
		foreach (IEntity candidate : weapons)
		{
			EntityPrefabData prefabData = candidate.GetPrefabData();
			if (prefabData && prefabData.GetPrefabName() == state.GetPrefab())
			{
				selectedWeapon = candidate;
				break;
			}
		}
		if (!selectedWeapon)
		{
			int managedIndex = state.GetManagedWeaponIndex();
			if (managedIndex >= 0 && managedIndex < weapons.Count())
				selectedWeapon = weapons[managedIndex];
		}
		if (!selectedWeapon)
		{
			if (report)
				PrintFormat("GC | Killed From | event=weapon-replay role=%1 result=no-match requestedPrefab=%2 requestedIndex=%3 cloneWeapons=%4", role, state.GetPrefab(), state.GetManagedWeaponIndex(), weapons.Count(), level: LogLevel.WARNING);
			return null;
		}

		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(selectedWeapon.FindComponent(BaseWeaponComponent));
		bool selected;
		if (weaponComponent)
			selected = manager.SelectWeapon(weaponComponent);
		manager.SetVisibleCurrentWeapon(true);
		if (report)
			PrintFormat("GC | Killed From | event=weapon-select role=%1 result=applied weapon=%2 requestedPrefab=%3 selected=%4 requestedSights=%5 actualSights=%6", role, selectedWeapon, state.GetPrefab(), selected, state.GetSightsIndex(), manager.GetCurrentSightsIndex());
		return selectedWeapon;
	}

	protected void ApplyWeaponTransform(IEntity actor, IEntity weapon, GC_KilledFromWeaponState state, string role, bool report)
	{
		if (!actor || !weapon || !state)
			return;
		vector actorTransform[4];
		vector actorRelativeTransform[4];
		vector exactTransform[4];
		actor.GetWorldTransform(actorTransform);
		state.GetActorRelativeTransform(actorRelativeTransform);
		Math3D.MatrixMultiply4(actorTransform, actorRelativeTransform, exactTransform);
		weapon.SetWorldTransform(exactTransform);
		weapon.Update();
		if (report)
		{
			vector readback[4];
			weapon.GetWorldTransform(readback);
			PrintFormat("GC | Killed From | event=weapon-transform role=%1 mode=ACTOR-RELATIVE weapon=%2 requested=%3 readback=%4", role, weapon, exactTransform, readback);
		}
	}

	protected bool ApplyWeaponAnimationGraph(IEntity actor, GC_KilledFromWeaponState state, int mode, string role, bool report)
	{
		if (!actor || !state)
			return false;
		CharacterAnimationComponent animation = CharacterAnimationComponent.Cast(actor.FindComponent(CharacterAnimationComponent));
		if (!animation)
		{
			if (report)
				PrintFormat("GC | Killed From | event=weapon-animation role=%1 mode=%2 result=no-character-animation", role, GetWeaponPrototypeName(mode), level: LogLevel.WARNING);
			return false;
		}

		TAnimGraphVariable adsVariable = animation.BindVariableBool("ADS");
		TAnimGraphVariable lowVariable = animation.BindVariableBool("Low");
		TAnimGraphVariable obstructionVariable = animation.BindVariableBool("WeaponObstruction");
		bool ads = state.IsWeaponADSTag();
		bool shared = true;
		if (shared)
		{
			if (adsVariable != -1)
				animation.SetSharedVariableBool(adsVariable, ads, false);
			if (lowVariable != -1 && ads)
				animation.SetSharedVariableBool(lowVariable, false, false);
			if (obstructionVariable != -1 && ads)
				animation.SetSharedVariableBool(obstructionVariable, false, false);
		}
		else
		{
			if (adsVariable != -1)
				animation.SetVariableBool(adsVariable, ads);
			if (lowVariable != -1 && ads)
				animation.SetVariableBool(lowVariable, false);
			if (obstructionVariable != -1 && ads)
				animation.SetVariableBool(obstructionVariable, false);
		}

		TAnimGraphVariable aimXVariable = -1;
		TAnimGraphVariable aimYVariable = -1;
		if (state.HasWeaponCommand())
		{
			aimXVariable = animation.BindVariableFloat("AimX");
			aimYVariable = animation.BindVariableFloat("AimY");
			if (aimXVariable != -1)
				animation.SetSharedVariableFloat(aimXVariable, state.GetAimAngleLR(), false);
			if (aimYVariable != -1)
				animation.SetSharedVariableFloat(aimYVariable, state.GetAimAngleUD(), false);
			animation.SetAnimAimY(state.GetAimAngleUD());
		}
		actor.Update();
		if (report)
		{
			PrintFormat("GC | Killed From | event=weapon-animation role=%1 mode=%2 result=applied sourceAnimation=%3 sourceRaised=%4 sourceADS=%5 sourceWeaponCommand=%6 aimLR=%7 aimUD=%8", role, GetWeaponPrototypeName(mode), state.HasCharacterAnimation(), state.IsWeaponRaisedTag(), ads, state.HasWeaponCommand(), state.GetAimAngleLR(), state.GetAimAngleUD());
			PrintFormat("GC | Killed From | event=weapon-animation-bindings role=%1 mode=%2 adsVar=%3 lowVar=%4 obstructionVar=%5 aimXVar=%6 aimYVar=%7 readbackRaised=%8 readbackADS=%9", role, GetWeaponPrototypeName(mode), adsVariable, lowVariable, obstructionVariable, aimXVariable, aimYVariable, animation.IsWeaponRaisedTag(), animation.IsWeaponADSTag());
		}
		return adsVariable != -1;
	}

	protected string GetWeaponPrototypeName(int mode)
	{
		switch (mode)
		{
			case GC_EKilledFromWeaponPrototype.LIVE_CONTROL: return "K0-LIVE";
			case GC_EKilledFromWeaponPrototype.POSE_THEN_FREEZE: return "VICTIM-POSE-ANIMOFF";
			case GC_EKilledFromWeaponPrototype.GAMEPLAY_SHARED_ADS_AIM: return "GAMEPLAY-ADS-AIM";
		}
		return "UNKNOWN";
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
		m_bVictimFrozenPreview = false;
		m_MuzzleWeapon = null;
		m_bMuzzleResolved = false;
		m_sMuzzleMode = string.Empty;
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
		array<ref GC_KilledFromBonePose> killerBonePose = m_Snapshot.GetKillerBonePose();
		array<ref GC_KilledFromBonePose> victimBonePose = m_Snapshot.GetVictimBonePose();
		int killerMissingBones;
		int killerFailedBones;
		int killerUnavailableLocalBones;
		ApplyWeaponAnimationGraph(m_KillerPreviewEntity, m_Snapshot.GetKillerWeapon(), GC_EKilledFromWeaponPrototype.LIVE_CONTROL, "killer-k0", !m_bPhysicsDeactivationReported);
		int killerAppliedBones = ApplyBonePose(m_KillerPreviewEntity, killerBonePose, killerMissingBones, killerFailedBones, killerUnavailableLocalBones);
		int victimMissingBones;
		int victimFailedBones;
		int victimUnavailableLocalBones;
		int victimAppliedBones;
		if (!m_bVictimFrozenPreview)
			victimAppliedBones = ApplyBonePose(m_VictimPreviewEntity, victimBonePose, victimMissingBones, victimFailedBones, victimUnavailableLocalBones);
		ApplyVehicleState(m_KillerVehiclePreviewEntity, m_Snapshot.GetKillerVehicle(), "killer");
		ApplyVehicleState(m_VictimVehiclePreviewEntity, m_Snapshot.GetVictimVehicle(), "victim");
		IEntity killerWeapon = SelectWeapon(m_KillerPreviewEntity, m_Snapshot.GetKillerWeapon(), "killer-k0", !m_bPhysicsDeactivationReported);
		ApplyWeaponTransform(m_KillerPreviewEntity, killerWeapon, m_Snapshot.GetKillerWeapon(), "killer-k0", !m_bPhysicsDeactivationReported);
		IEntity victimWeapon;
		if (!m_bVictimFrozenPreview)
		{
			victimWeapon = SelectWeapon(m_VictimPreviewEntity, m_Snapshot.GetVictimWeapon(), "victim", !m_bPhysicsDeactivationReported);
			ApplyWeaponTransform(m_VictimPreviewEntity, victimWeapon, m_Snapshot.GetVictimWeapon(), "victim", !m_bPhysicsDeactivationReported);
			ApplyWeaponAnimationGraph(m_VictimPreviewEntity, m_Snapshot.GetVictimWeapon(), GC_EKilledFromWeaponPrototype.GAMEPLAY_SHARED_ADS_AIM, "victim", !m_bPhysicsDeactivationReported);
		}
		IsolatePhysicsHierarchy(killerWeapon);
		IsolatePhysicsHierarchy(victimWeapon);
		int killerDeactivatedPhysics = DeactivatePhysicsHierarchy(m_KillerPreviewEntity);
		int victimDeactivatedPhysics = DeactivatePhysicsHierarchy(m_VictimPreviewEntity);
		int killerVehicleDeactivatedPhysics = DeactivatePhysicsHierarchy(m_KillerVehiclePreviewEntity);
		int victimVehicleDeactivatedPhysics = DeactivatePhysicsHierarchy(m_VictimVehiclePreviewEntity);

		if (!m_bPhysicsDeactivationReported)
		{
			int killerCapturedBones;
			if (killerBonePose)
				killerCapturedBones = killerBonePose.Count();
			int victimCapturedBones;
			if (victimBonePose)
				victimCapturedBones = victimBonePose.Count();
			PrintFormat("GC | Killed From | event=clone-bone-pose treatment=K2-LOCAL-DELTA role=killer captured=%1 applied=%2 missing=%3 failed=%4 unavailableLocal=%5", killerCapturedBones, killerAppliedBones, killerMissingBones, killerFailedBones, killerUnavailableLocalBones);
			PrintFormat("GC | Killed From | event=clone-bone-pose treatment=K2-LOCAL-DELTA role=victim captured=%1 applied=%2 missing=%3 failed=%4 unavailableLocal=%5", victimCapturedBones, victimAppliedBones, victimMissingBones, victimFailedBones, victimUnavailableLocalBones);
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
		vector lineOrigin = m_Snapshot.GetLineOrigin();
		string muzzleMode = "captured-fallback";
		IEntity muzzleRoot = killerWeapon;
		bool allowBounds = true;
		if (m_KillerVehiclePreviewEntity)
		{
			muzzleRoot = m_KillerVehiclePreviewEntity;
			allowBounds = false;
		}
		if (muzzleRoot != m_MuzzleWeapon)
		{
			m_MuzzleWeapon = muzzleRoot;
			m_bMuzzleResolved = false;
		}
		if (!m_bMuzzleResolved && ResolveWeaponMuzzlePosition(muzzleRoot, m_Snapshot.GetHitPosition(), allowBounds, m_vMuzzlePosition, m_sMuzzleMode))
			m_bMuzzleResolved = true;
		if (m_bMuzzleResolved)
		{
			lineOrigin = m_vMuzzlePosition;
			muzzleMode = "clone-" + m_sMuzzleMode;
		}
		vector start;
		vector end;
		if (ProjectTraceToScreen(lineOrigin, m_Snapshot.GetHitPosition(), start, end))
		{
			traceVisible = true;
			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = Color.RED;
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
		bool killerLabelVisible = UpdateActorMarker(m_wKillerMarker, killerLabelEntity);
		bool victimLabelVisible = UpdateActorMarker(m_wVictimMarker, victimLabelEntity);
		bool labelVisible = killerLabelVisible || victimLabelVisible;
		if (!m_bVisibilityReported || traceVisible != m_bLastTraceVisible || labelVisible != m_bLastLabelVisible)
		{
			PrintFormat("GC | Killed From | event=render-visibility traceVisible=%1 killerLabel=%2 victimLabel=%3 killerPreview=%4 victimPreview=%5 killerVehicle=%6 victimVehicle=%7", traceVisible, killerLabelVisible, victimLabelVisible, m_KillerPreviewEntity != null, m_VictimPreviewEntity != null, m_KillerVehiclePreviewEntity != null, m_VictimVehiclePreviewEntity != null);
			PrintFormat("GC | Killed From | event=render-line muzzleMode=%1 lineOrigin=%2 lineStartScreen=%3 lineEndScreen=%4", muzzleMode, lineOrigin, start, end);
			m_bVisibilityReported = true;
			m_bLastTraceVisible = traceVisible;
			m_bLastLabelVisible = labelVisible;
		}
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	//! Clip the world-space trace against the active camera near plane before
	//! projecting it. Canvas handles the remaining screen-edge clipping, so a
	//! side view does not discard the whole trace when only one endpoint is behind.
	protected bool ProjectTraceToScreen(vector worldStart, vector worldEnd, out vector screenStart, out vector screenEnd)
	{
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return false;

		CameraBase camera = cameraManager.CurrentCamera();
		if (!camera)
			return false;

		vector cameraTransform[4];
		camera.GetWorldTransform(cameraTransform);
		vector cameraPosition = cameraTransform[3];
		vector cameraForward = cameraTransform[2];
		float startDepth = vector.Dot(worldStart - cameraPosition, cameraForward);
		float endDepth = vector.Dot(worldEnd - cameraPosition, cameraForward);
		float clipDepth = camera.GetNearPlane() + 0.01;
		if (startDepth <= clipDepth && endDepth <= clipDepth)
			return false;

		if (startDepth <= clipDepth)
		{
			float startBlend = (clipDepth - startDepth) / (endDepth - startDepth);
			worldStart = worldStart + (worldEnd - worldStart) * startBlend;
		}
		else if (endDepth <= clipDepth)
		{
			float endBlend = (clipDepth - endDepth) / (startDepth - endDepth);
			worldEnd = worldEnd + (worldStart - worldEnd) * endBlend;
		}

		screenStart = m_Workspace.ProjWorldToScreen(worldStart, m_World);
		screenEnd = m_Workspace.ProjWorldToScreen(worldEnd, m_World);
		return screenStart[2] > 0.0 && screenEnd[2] > 0.0;
	}

	protected bool ResolveWeaponMuzzlePosition(IEntity weapon, vector hitPosition, bool allowBounds, out vector muzzlePosition, out string mode)
	{
		if (!weapon)
			return false;

		array<IEntity> hierarchy = {weapon};
		SCR_EntityHelper.GetHierarchyEntityList(weapon, hierarchy);
		float bestDistanceSq = 1.0e30;
		foreach (IEntity entity : hierarchy)
		{
			SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(entity.FindComponent(SCR_MuzzleEffectComponent));
			IEntity particleEntity;
			if (muzzleEffect)
				particleEntity = muzzleEffect.GetParticleEntity();
			if (particleEntity)
			{
				vector candidate = particleEntity.GetOrigin();
				float distanceSq = (candidate - hitPosition).LengthSq();
				if (distanceSq < bestDistanceSq)
				{
					bestDistanceSq = distanceSq;
					muzzlePosition = candidate;
					mode = "particle";
				}
			}

			Animation animation = entity.GetAnimation();
			if (!animation)
				continue;
			array<string> boneNames = {};
			animation.GetBoneNames(boneNames);
			vector entityTransform[4];
			entity.GetWorldTransform(entityTransform);
			foreach (string boneName : boneNames)
			{
				if (!boneName.Contains("muzzle") && !boneName.Contains("Muzzle") && !boneName.Contains("barrel") && !boneName.Contains("Barrel"))
					continue;

				vector boneTransform[4];
				if (!animation.GetBoneMatrix(animation.GetBoneIndex(boneName), boneTransform))
					continue;
				vector boneWorldTransform[4];
				Math3D.MatrixMultiply4(entityTransform, boneTransform, boneWorldTransform);
				vector candidate = boneWorldTransform[3];
				float distanceSq = (candidate - hitPosition).LengthSq();
				if (distanceSq >= bestDistanceSq)
					continue;

				bestDistanceSq = distanceSq;
				muzzlePosition = candidate;
				mode = string.Format("bone-%1", boneName);
			}
		}
		if (bestDistanceSq < 1.0e30)
			return true;
		if (!allowBounds)
			return false;

		vector mins;
		vector maxs;
		weapon.GetBounds(mins, maxs);
		vector weaponTransform[4];
		weapon.GetWorldTransform(weaponTransform);
		for (int cornerIndex; cornerIndex < 8; cornerIndex++)
		{
			vector localCorner = mins;
			if (cornerIndex % 2 == 1)
				localCorner[0] = maxs[0];
			if ((cornerIndex / 2) % 2 == 1)
				localCorner[1] = maxs[1];
			if ((cornerIndex / 4) % 2 == 1)
				localCorner[2] = maxs[2];
			vector candidate = weaponTransform[3]
				+ weaponTransform[0] * localCorner[0]
				+ weaponTransform[1] * localCorner[1]
				+ weaponTransform[2] * localCorner[2];
			float distanceSq = (candidate - hitPosition).LengthSq();
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			muzzlePosition = candidate;
		}
		mode = "forward-bound";
		return true;
	}

	protected void ConfigureMarker(Widget marker, string text, Color accentColor)
	{
		if (!marker)
			return;
		TextWidget label = TextWidget.Cast(marker.FindAnyWidget("LobbySpectatorKilledFromMarkerText"));
		if (label)
			label.SetText(text);
		ImageWidget outline = ImageWidget.Cast(marker.FindAnyWidget("LobbySpectatorKilledFromMarkerOutline"));
		if (outline)
			outline.SetColor(accentColor);
		PanelWidget labelBackground = PanelWidget.Cast(marker.FindAnyWidget("LobbySpectatorKilledFromMarkerLabel"));
		if (labelBackground)
			labelBackground.SetColor(accentColor);
		marker.SetVisible(false);
	}

	protected void UpdateKillerMarkerText()
	{
		if (!m_wKillerMarker)
			return;

		string markerText = "KILLED YOU";
		if (m_Snapshot && !m_Snapshot.GetKillerDisplayName().IsEmpty())
		{
			string killerDisplayName = m_Snapshot.GetKillerDisplayName();
			if (m_Snapshot.IsKillerDisplayNameLocalized())
				killerDisplayName = WidgetManager.Translate(killerDisplayName);
			markerText = killerDisplayName + " KILLED YOU";
		}

		TextWidget label = TextWidget.Cast(m_wKillerMarker.FindAnyWidget("LobbySpectatorKilledFromMarkerText"));
		if (label)
			label.SetText(markerText);
	}

	protected bool UpdateActorMarker(Widget marker, IEntity actor)
	{
		if (!marker || !actor)
		{
			if (marker)
				marker.SetVisible(false);
			return false;
		}

		vector worldPosition = actor.GetOrigin();
		worldPosition[1] = worldPosition[1] + KILLER_LABEL_WORLD_OFFSET;
		vector screenPosition = m_Workspace.ProjWorldToScreen(worldPosition, m_World);
		bool visible = screenPosition[2] > 0.0;
		marker.SetVisible(visible);
		if (!visible)
			return false;

		FrameSlot.SetPos(marker,
			m_Workspace.DPIUnscale(screenPosition[0]) - MARKER_WIDTH * 0.5,
			m_Workspace.DPIUnscale(screenPosition[1]) - MARKER_HEIGHT);
		return true;
	}

	protected void DestroyMarkers()
	{
		if (m_wKillerMarker)
			m_wKillerMarker.RemoveFromHierarchy();
		if (m_wVictimMarker)
			m_wVictimMarker.RemoveFromHierarchy();
		m_wKillerMarker = null;
		m_wVictimMarker = null;
	}

	protected void ClearLine()
	{
		m_aDrawCommands = {};
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);
		if (m_wKillerMarker)
			m_wKillerMarker.SetVisible(false);
		if (m_wVictimMarker)
			m_wVictimMarker.SetVisible(false);
	}
}
