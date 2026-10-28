class GC_KilledFromBoneOperation
{
	TNodeId m_iBoneIndex;
	vector m_aMatrix[4];
}

//! Client-only killed-from preview and line renderer. No entity created here is
//! inserted on the server or exposed as replicated gameplay state.
class GC_SpectatorKilledFromVisualizer
{
	protected static const float LINE_WIDTH = 1.5;
	protected static const float KILLER_LABEL_WORLD_OFFSET = 2.2;
	protected static const float MARKER_WIDTH = 256.0;
	protected static const float MARKER_HEIGHT = 76.0;
	protected static const float MARKER_LABEL_HEIGHT = 26.0;
	protected static const float MARKER_LABEL_Y = 50.0;
	protected static const float MARKER_LABEL_MIN_WIDTH = 52.0;
	protected static const float MARKER_LABEL_HORIZONTAL_PADDING = 24.0;
	protected static const ResourceName MARKER_LAYOUT = "{6A5F000000000100}UI/layouts/Spectator/Lobby_Spectator_Killed_From_Marker.layout";
	protected CanvasWidget m_wCanvas;
	protected Widget m_wKillerMarker;
	protected Widget m_wVictimMarker;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected ref Color m_KillerFactionColor;
	protected ref Color m_VictimFactionColor;
	protected IEntity m_KillerPreviewEntity;
	protected IEntity m_VictimPreviewEntity;
	protected IEntity m_KillerVehiclePreviewEntity;
	protected IEntity m_VictimVehiclePreviewEntity;
	protected int m_iPreviewFrames;
	protected bool m_bPoseFrozen;
	protected bool m_bEnabled;
	protected bool m_bVisibilityReported;
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
		m_Snapshot = snapshot;
		UpdateMarkerPresentation();
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
		UpdateMarkerPresentation();
		m_bVisibilityReported = false;
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
			m_KillerVehiclePreviewEntity = CreateVehicleClone(killerVehicle, "killer-vehicle");
		GC_KilledFromVehicleState victimVehicle = m_Snapshot.GetVictimVehicle();
		if (victimVehicle && victimVehicle.IsValid())
			m_VictimVehiclePreviewEntity = CreateVehicleClone(victimVehicle, "victim-vehicle");

		vector killerTransform[4];
		m_Snapshot.GetKillerRootTransform(killerTransform);
		m_KillerPreviewEntity = CreateActorClone(m_Snapshot.GetKillerPrefab(), m_Snapshot.GetKillerLoadout(), killerTransform, m_Snapshot.GetKillerBonePose(), "killer");
		vector victimTransform[4];
		m_Snapshot.GetVictimRootTransform(victimTransform);
		m_VictimPreviewEntity = CreateActorClone(m_Snapshot.GetVictimPrefab(), m_Snapshot.GetVictimLoadout(), victimTransform, m_Snapshot.GetVictimBonePose(), "victim");
		SelectWeapon(m_KillerPreviewEntity, m_Snapshot.GetKillerWeapon(), "killer", true);
		SelectWeapon(m_VictimPreviewEntity, m_Snapshot.GetVictimWeapon(), "victim", true);
		m_iPreviewFrames = 0;
		m_bPoseFrozen = false;
		EnsureActorSeat(m_KillerPreviewEntity, m_KillerVehiclePreviewEntity, killerVehicle, "killer");
		EnsureActorSeat(m_VictimPreviewEntity, m_VictimVehiclePreviewEntity, victimVehicle, "victim");
		PrintFormat("GC | Killed From | event=preview-create result=complete killer=%1 victim=%2 killerVehicle=%3 victimVehicle=%4 snapshot={%5}", m_KillerPreviewEntity, m_VictimPreviewEntity, m_KillerVehiclePreviewEntity, m_VictimVehiclePreviewEntity, m_Snapshot.GetDebugReport());
	}

	protected IEntity CreateVehicleClone(GC_KilledFromVehicleState state, string role)
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
		DeactivatePhysicsHierarchy(preview);
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
		if (access.GetCompartment() == targetCompartment)
			return true;
		bool entered = access.GetInVehicle(vehicle, targetCompartment, true, -1, ECloseDoorAfterActions.INVALID, false);
		PrintFormat("GC | Killed From | event=vehicle-seat role=%1 result=%2 actor=%3 vehicle=%4 type=%5 index=%6 target=%7 actualVehicle=%8 actualCompartment=%9", role, entered, actor, vehicle, state.GetCompartmentType(), compartmentIndex, targetCompartment, access.GetVehicleCompartmentManagerOwner(), access.GetCompartment());
		return entered;
	}

	protected void ApplyVehicleState(IEntity vehicle, GC_KilledFromVehicleState state, string role)
	{
		if (!vehicle || !state)
			return;
		vector rootTransform[4];
		state.GetRootTransform(rootTransform);
		vehicle.SetWorldTransform(rootTransform);
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
			TurretComponent turret = controllers[index].GetTurretComponent();
			if (turret)
				turret.SetAimingRotation(rawRotation);
			controllers[index].SetWeaponADSInput(ads);
			controllers[index].SetWeaponADS(ads);
			controllers[index].SetCurrentSightsADS(ads);
		}
	}

	//! Spawn the fully initialized character, then replace its authored inventory
	//! with the exact recursively serialized death-time loadout. This retains the
	//! gameplay skeleton/animation path needed for ADS while reflecting changed
	//! clothing, headgear, weapons, optics, and attachments.
	protected IEntity CreateActorClone(ResourceName prefab, string loadout, vector rootTransform[4], array<ref GC_KilledFromBonePose> bonePose, string role)
	{
		if (prefab == ResourceName.Empty || loadout.IsEmpty() || !bonePose || bonePose.IsEmpty())
		{
			PrintFormat("GC | Killed From | event=actor-clone result=skipped role=%1 reason=missing-capture prefab=%2 loadoutBytes=%3", role, prefab, loadout.Length(), level: LogLevel.WARNING);
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
			PrintFormat("GC | Killed From | event=actor-clone result=failed role=%1 stage=spawn prefab=%2", role, prefab, level: LogLevel.ERROR);
			return null;
		}

		JsonLoadContext loadoutContext = new JsonLoadContext();
		bool loadoutApplied = loadoutContext.LoadFromString(loadout) && GC_KilledFromLoadoutApplicator.Apply(preview, loadoutContext, role);
		if (!loadoutApplied)
		{
			PrintFormat("GC | Killed From | event=actor-clone result=failed role=%1 stage=loadout prefab=%2 loadoutBytes=%3", role, prefab, loadout.Length(), level: LogLevel.ERROR);
			delete preview;
			return null;
		}

		AIControlComponent aiControl = AIControlComponent.Cast(preview.FindComponent(AIControlComponent));
		if (aiControl)
			aiControl.DeactivateAI();
		preview.Update();
		int isolatedPhysics = IsolatePhysicsHierarchy(preview);
		int capturedBones;
		if (bonePose)
			capturedBones = bonePose.Count();
		vector transformReadback[4];
		preview.GetWorldTransform(transformReadback);
		PrintFormat("GC | Killed From | event=actor-clone result=spawned role=%1 preview=%2 prefab=%3 loadoutBytes=%4 isolatedPhysics=%5 capturedBones=%6 boneReplay=scheduled sourceMutation=none", role, preview, prefab, loadout.Length(), isolatedPhysics, capturedBones);
		PrintFormat("GC | Killed From | event=actor-clone-transform role=%1 requested=%2 readback=%3", role, rootTransform, transformReadback);
		return preview;
	}

	//! Precompute each source target relative to the clone's evaluated
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
			vector currentTransform[4];
			if (!pose.m_bHasLocalTransform)
			{
				unavailableLocalBones++;
				continue;
			}
			// TNodeId is a signed identifier, not an array index. Negative IDs
			// include valid hand/weapon bones; let the animation API resolve it.
			if (!animation.GetBoneLocalMatrix(boneIndex, currentTransform))
			{
				missingBones++;
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
		if (!actor || !state)
			return null;
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(actor.FindComponent(CharacterControllerComponent));
		if (!state.IsValid())
		{
			if (controller)
				controller.TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedDeliberate, true);
			return null;
		}
		CharacterWeaponManagerComponent manager = CharacterWeaponManagerComponent.Cast(actor.FindComponent(CharacterWeaponManagerComponent));
		if (!manager)
			return null;
		array<IEntity> weapons = {};
		manager.GetWeaponsList(weapons);
		IEntity selectedWeapon;
		int managedIndex = state.GetManagedWeaponIndex();
		if (managedIndex >= 0 && managedIndex < weapons.Count())
		{
			IEntity candidate = weapons[managedIndex];
			EntityPrefabData prefabData;
			if (candidate)
				prefabData = candidate.GetPrefabData();
			if (prefabData && prefabData.GetPrefabName() == state.GetPrefab())
				selectedWeapon = candidate;
		}
		if (!selectedWeapon)
		{
			if (report)
				PrintFormat("GC | Killed From | event=weapon-replay role=%1 result=no-match requestedPrefab=%2 requestedIndex=%3 cloneWeapons=%4", role, state.GetPrefab(), state.GetManagedWeaponIndex(), weapons.Count(), level: LogLevel.WARNING);
			return null;
		}

		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(selectedWeapon.FindComponent(BaseWeaponComponent));
		bool selected;
		if (weaponComponent && controller)
			selected = controller.TryEquipRightHandItem(selectedWeapon, EEquipItemType.EEquipTypeWeapon, true);
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

	//------------------------------------------------------------------------------------------------
	//! Inventory and the native command initialize before the frozen pose is set.
	//! Applying offsets during spawn lets the first native animation update erase them.
	protected void FreezeActor(IEntity actor, vector rootTransform[4], array<ref GC_KilledFromBonePose> pose, GC_KilledFromWeaponState weaponState, string role)
	{
		if (!actor)
			return;
		CharacterAnimationComponent animation = CharacterAnimationComponent.Cast(actor.FindComponent(CharacterAnimationComponent));
		if (animation)
			animation.Deactivate(actor);
		actor.SetWorldTransform(rootTransform);
		CharacterWeaponManagerComponent weapons = CharacterWeaponManagerComponent.Cast(actor.FindComponent(CharacterWeaponManagerComponent));
		IEntity weapon;
		if (weapons && weapons.GetCurrentWeapon())
			weapon = weapons.GetCurrentWeapon().GetOwner();
		if (weaponState)
		{
			array<IEntity> inventoryWeapons = {};
			if (weapons)
				weapons.GetWeaponsList(inventoryWeapons);
			bool correctWeapon = !weaponState.IsValid() && !weapon;
			if (weaponState.IsValid() && weapon)
				correctWeapon = inventoryWeapons.Find(weapon) == weaponState.GetManagedWeaponIndex() && SCR_ResourceNameUtils.GetPrefabName(weapon) == weaponState.GetPrefab();
			if (!correctWeapon)
			{
				PrintFormat("GC | Killed From | event=pose-frozen role=%1 result=failed reason=weapon-not-ready", role, level: LogLevel.ERROR);
				delete actor;
				return;
			}
		}
		IsolatePhysicsHierarchy(actor);
		DeactivatePhysicsHierarchy(actor);
		int missing;
		int failed;
		int unavailable;
		int applied = ApplyBonePose(actor, pose, missing, failed, unavailable);
		// The held item is parented to an animated bone. Restore its transform
		// after that parent's pose has changed, otherwise it moves twice.
		ApplyWeaponTransform(actor, weapon, weaponState, role, false);
		PrintFormat("GC | Killed From | event=pose-frozen role=%1 applied=%2 missing=%3 failed=%4 unavailable=%5", role, applied, missing, failed, unavailable);
	}

	//------------------------------------------------------------------------------------------------
	protected void FreezePreview()
	{
		if (m_bPoseFrozen || !m_Snapshot)
			return;
		vector root[4];
		m_Snapshot.GetKillerRootTransform(root);
		FreezeActor(m_KillerPreviewEntity, root, m_Snapshot.GetKillerBonePose(), m_Snapshot.GetKillerWeapon(), "killer");
		m_Snapshot.GetVictimRootTransform(root);
		FreezeActor(m_VictimPreviewEntity, root, m_Snapshot.GetVictimBonePose(), m_Snapshot.GetVictimWeapon(), "victim");
		// Local reconstructions must not appear as live editor/spectator targets.
		array<IEntity> previews = {m_KillerPreviewEntity, m_VictimPreviewEntity, m_KillerVehiclePreviewEntity, m_VictimVehiclePreviewEntity};
		foreach (IEntity preview : previews)
		{
			if (!preview)
				continue;
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(preview);
			if (editable)
			{
				editable.SetEntityFlag(EEditableEntityFlag.LOCAL, true);
				editable.RemoveParentEntity();
			}
		}
		m_bPoseFrozen = true;
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
		m_bPoseFrozen = false;
		m_iPreviewFrames = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePose()
	{
		if (m_bPoseFrozen)
			return;
		// Allow one complete native frame after spawn before writing bone offsets.
		m_iPreviewFrames++;
		if (m_iPreviewFrames < 2)
			return;
		// Teleporting into a compartment is still a native character command.
		// Deactivating animation before it completes cancels the seat attachment.
		bool waitingForSeat;
		array<IEntity> actors = {m_KillerPreviewEntity, m_VictimPreviewEntity};
		array<ref GC_KilledFromVehicleState> vehicles = {m_Snapshot.GetKillerVehicle(), m_Snapshot.GetVictimVehicle()};
		for (int index; index < actors.Count(); index++)
		{
			if (!actors[index] || !vehicles[index] || !vehicles[index].HasOccupiedCompartment())
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(actors[index]);
			if (character && !character.GetCompartmentAccessComponent().GetCompartment())
				waitingForSeat = true;
		}
		if (waitingForSeat && m_iPreviewFrames < 120)
			return;
		if (waitingForSeat)
			Print("GC | Killed From | event=vehicle-seat result=failed reason=attachment-timeout", LogLevel.ERROR);
		FreezePreview();
	}

	protected void Update()
	{
		m_aDrawCommands = {};
		if (!m_bEnabled || !m_Snapshot || !m_wCanvas || !m_Workspace || !m_World)
			return;

		UpdatePose();
		if (!m_bPoseFrozen)
			return;
		ApplyVehicleState(m_KillerVehiclePreviewEntity, m_Snapshot.GetKillerVehicle(), "killer");
		ApplyVehicleState(m_VictimVehiclePreviewEntity, m_Snapshot.GetVictimVehicle(), "victim");
		DeactivatePhysicsHierarchy(m_KillerVehiclePreviewEntity);
		DeactivatePhysicsHierarchy(m_VictimVehiclePreviewEntity);

		// World.OutlineEntity submits an outline for the current frame only. Keep
		// every killed-from clone registered after animation and inventory updates
		// replace child render objects.
		ChimeraWorld outlineWorld = ChimeraWorld.CastFrom(m_World);
		if (outlineWorld)
		{
			if (m_KillerPreviewEntity)
				outlineWorld.OutlineEntity(m_KillerPreviewEntity, m_KillerFactionColor, 1.0, 0.2, true, 2);
			if (m_KillerVehiclePreviewEntity)
				outlineWorld.OutlineEntity(m_KillerVehiclePreviewEntity, m_KillerFactionColor, 1.0, 0.2, true, 2);
			if (m_VictimPreviewEntity)
				outlineWorld.OutlineEntity(m_VictimPreviewEntity, m_VictimFactionColor, 1.0, 0.2, true, 2);
			if (m_VictimVehiclePreviewEntity)
				outlineWorld.OutlineEntity(m_VictimVehiclePreviewEntity, m_VictimFactionColor, 1.0, 0.2, true, 2);
		}

		bool traceVisible;
		vector lineOrigin = m_Snapshot.GetLineOrigin();
		string muzzleMode = "captured";
		vector start;
		vector end;
		if (m_Snapshot.HasShotLine() && ProjectTraceToScreen(lineOrigin, m_Snapshot.GetHitPosition(), start, end))
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

		// Canvas vertices use render-target pixels; FrameSlot markers use reference units.
		screenStart = m_Workspace.ProjWorldToScreenNative(worldStart, m_World);
		screenEnd = m_Workspace.ProjWorldToScreenNative(worldEnd, m_World);
		if (screenStart[2] <= 0.0 || screenEnd[2] <= 0.0)
			return false;

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
		SizeMarkerLabel(marker);
		marker.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateMarkerPresentation()
	{
		m_KillerFactionColor = Color.FromInt(Color.WHITE);
		m_VictimFactionColor = Color.FromInt(Color.WHITE);
		if (m_Snapshot)
		{
			m_KillerFactionColor = Color.FromInt(m_Snapshot.GetKillerFactionColor());
			m_VictimFactionColor = Color.FromInt(m_Snapshot.GetVictimFactionColor());
		}

		string markerText = "KILLED YOU";
		if (m_Snapshot && !m_Snapshot.GetKillerDisplayName().IsEmpty())
		{
			string killerDisplayName = m_Snapshot.GetKillerDisplayName();
			if (m_Snapshot.IsKillerDisplayNameLocalized())
				killerDisplayName = WidgetManager.Translate(killerDisplayName);
			markerText = killerDisplayName + " KILLED YOU";
		}

		ConfigureMarker(m_wKillerMarker, markerText, m_KillerFactionColor);
		string victimText = "YOU";
		if (m_Snapshot && m_Snapshot.IsSelfInflicted())
			victimText = "YOU — SELF-INFLICTED";
		ConfigureMarker(m_wVictimMarker, victimText, m_VictimFactionColor);
	}

	protected void SizeMarkerLabel(Widget marker)
	{
		if (!marker || !m_Workspace)
			return;

		TextWidget label = TextWidget.Cast(marker.FindAnyWidget("LobbySpectatorKilledFromMarkerText"));
		Widget labelBackground = marker.FindAnyWidget("LobbySpectatorKilledFromMarkerLabel");
		if (!label || !labelBackground)
			return;

		float textWidth;
		float textHeight;
		label.GetTextSize(textWidth, textHeight);
		float labelWidth = m_Workspace.DPIUnscale(textWidth) + MARKER_LABEL_HORIZONTAL_PADDING;
		labelWidth = Math.Max(MARKER_LABEL_MIN_WIDTH, Math.Min(MARKER_WIDTH, labelWidth));
		FrameSlot.SetPos(labelBackground, (MARKER_WIDTH - labelWidth) * 0.5, MARKER_LABEL_Y);
		FrameSlot.SetSize(labelBackground, labelWidth, MARKER_LABEL_HEIGHT);
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
			screenPosition[0] - MARKER_WIDTH * 0.5,
			screenPosition[1] - MARKER_HEIGHT);
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
