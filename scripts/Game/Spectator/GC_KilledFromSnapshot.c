//! One captured skeletal bone used by a client-only visual reconstruction.
class GC_KilledFromBonePose
{
	string m_sBoneName;
	vector m_aTransform[4];
	vector m_aLocalTransform[4];
	bool m_bHasLocalTransform;
}

class GC_KilledFromWeaponState
{
	protected ResourceName m_sPrefab;
	protected vector m_aWorldTransform[4];
	protected int m_iManagedWeaponIndex = -1;
	protected int m_iSightsIndex = -1;

	void Capture(IEntity character)
	{
		CharacterWeaponManagerComponent manager = CharacterWeaponManagerComponent.Cast(character.FindComponent(CharacterWeaponManagerComponent));
		if (!manager)
			return;
		BaseWeaponComponent weapon = manager.GetCurrentWeapon();
		if (!weapon || !weapon.GetOwner())
			return;
		IEntity weaponEntity = weapon.GetOwner();
		EntityPrefabData prefabData = weaponEntity.GetPrefabData();
		if (prefabData)
			m_sPrefab = prefabData.GetPrefabName();
		weaponEntity.GetWorldTransform(m_aWorldTransform);
		m_iSightsIndex = manager.GetCurrentSightsIndex();
		array<IEntity> weapons = {};
		manager.GetWeaponsList(weapons);
		m_iManagedWeaponIndex = weapons.Find(weaponEntity);
		PrintFormat("GC | Killed From | event=weapon-capture character=%1 weapon=%2 prefab=%3 managedIndex=%4 sightsIndex=%5 transform=%6", character, weaponEntity, m_sPrefab, m_iManagedWeaponIndex, m_iSightsIndex, m_aWorldTransform);
	}

	bool IsValid() { return m_sPrefab != ResourceName.Empty; }
	ResourceName GetPrefab() { return m_sPrefab; }
	int GetManagedWeaponIndex() { return m_iManagedWeaponIndex; }
	int GetSightsIndex() { return m_iSightsIndex; }
	void GetWorldTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aWorldTransform, transform); }
}

//! One captured turret animation state. Turrets are recorded and replayed in
//! deterministic prefab hierarchy order.
class GC_KilledFromTurretState
{
	protected vector m_vRawAimingRotation;
	protected bool m_bWeaponADS;

	void Capture(TurretControllerComponent controller)
	{
		if (!controller)
			return;
		TurretComponent turret = controller.GetTurretComponent();
		if (turret)
			m_vRawAimingRotation = turret.GetRawAimingRotation();
		m_bWeaponADS = controller.IsWeaponADS() || controller.GetWeaponADSInput() || controller.GetCurrentSightsADS();
	}

	vector GetRawAimingRotation() { return m_vRawAimingRotation; }
	bool IsWeaponADS() { return m_bWeaponADS; }
}

//! Client-local vehicle and seat facts for one reconstructed actor.
class GC_KilledFromVehicleState
{
	protected ResourceName m_sPrefab;
	protected vector m_aRootTransform[4];
	protected vector m_vPosition;
	protected vector m_vAngles;
	protected ECompartmentType m_eCompartmentType;
	protected int m_iCompartmentIndex = -1;
	protected ref array<ref GC_KilledFromTurretState> m_aTurrets = {};

	void Capture(IEntity vehicle, BaseCompartmentSlot occupiedCompartment = null)
	{
		if (!vehicle)
			return;
		EntityPrefabData prefabData = vehicle.GetPrefabData();
		if (prefabData)
			m_sPrefab = prefabData.GetPrefabName();
		vehicle.GetWorldTransform(m_aRootTransform);
		m_vPosition = vehicle.GetOrigin();
		m_vAngles = vehicle.GetAngles();

		if (occupiedCompartment)
		{
			m_eCompartmentType = occupiedCompartment.GetType();
			SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (manager)
			{
				array<BaseCompartmentSlot> compartments = {};
				manager.GetCompartmentsOfType(compartments, m_eCompartmentType);
				m_iCompartmentIndex = compartments.Find(occupiedCompartment);
			}
		}

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
				GC_KilledFromTurretState turretState = new GC_KilledFromTurretState();
				turretState.Capture(controller);
				m_aTurrets.Insert(turretState);
			}
		}
	}

	void SetNetworkData(ResourceName prefab, vector position, vector angles, ECompartmentType compartmentType, int compartmentIndex)
	{
		m_sPrefab = prefab;
		m_vPosition = position;
		m_vAngles = angles;
		m_eCompartmentType = compartmentType;
		m_iCompartmentIndex = compartmentIndex;
		Math3D.AnglesToMatrix(angles, m_aRootTransform);
		m_aRootTransform[3] = position;
	}

	bool IsValid() { return m_sPrefab != ResourceName.Empty; }
	bool HasOccupiedCompartment() { return m_iCompartmentIndex >= 0; }
	ResourceName GetPrefab() { return m_sPrefab; }
	vector GetPosition() { return m_vPosition; }
	vector GetAngles() { return m_vAngles; }
	ECompartmentType GetCompartmentType() { return m_eCompartmentType; }
	int GetCompartmentIndex() { return m_iCompartmentIndex; }
	array<ref GC_KilledFromTurretState> GetTurrets() { return m_aTurrets; }
	void GetRootTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aRootTransform, transform); }
	string GetDebugReport()
	{
		return string.Format("valid=%1 prefab=%2 position=%3 angles=%4 compartmentType=%5 compartmentIndex=%6 turrets=%7", IsValid(), m_sPrefab, m_vPosition, m_vAngles, m_eCompartmentType, m_iCompartmentIndex, m_aTurrets.Count());
	}
}

//! Facts required to recreate one killed-from presentation. Network-safe
//! primitive fields provide the authority fallback; visual entries and exact
//! bone poses exist only on the owning client.
class GC_KilledFromSnapshot
{
	static const int FLAG_VALID_KILLER = 1;
	static const int FLAG_WEAPON_RAISED = 2;
	static const int FLAG_WEAPON_ADS = 4;

	protected ResourceName m_sKillerPrefab;
	protected vector m_vKillerPosition;
	protected vector m_vKillerAngles;
	protected vector m_vLineOrigin;
	protected vector m_vHitPosition;
	protected int m_iStance;
	protected float m_fDynamicStance;
	protected float m_fKillerLean;
	protected float m_fVictimLean;
	protected vector m_vKillerAimingAngles;
	protected vector m_vVictimAimingAngles;
	protected string m_sLineOriginMode;
	protected int m_iFlags;
	protected bool m_bHasLocalDamageCapture;
	protected ref array<ref SCR_BasePreviewEntry> m_aKillerPreviewEntries;
	protected ResourceName m_sKillerVisualObject;
	protected vector m_aKillerRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aKillerBonePose;
	protected ref GC_KilledFromWeaponState m_KillerWeapon;
	protected ref GC_KilledFromVehicleState m_KillerVehicle;
	protected ref array<ref SCR_BasePreviewEntry> m_aVictimPreviewEntries;
	protected ResourceName m_sVictimPrefab;
	protected ResourceName m_sVictimVisualObject;
	protected vector m_aVictimRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aVictimBonePose;
	protected ref GC_KilledFromWeaponState m_VictimWeapon;
	protected ref GC_KilledFromVehicleState m_VictimVehicle;
	protected int m_iVictimStance;
	protected float m_fVictimDynamicStance;
	protected int m_iVictimFlags;

	void Capture(IEntity victim, notnull BaseDamageContext damageContext)
	{
		m_bHasLocalDamageCapture = true;
		m_vHitPosition = damageContext.hitPosition;
		IEntity killer;
		if (damageContext.instigator)
		{
			killer = damageContext.instigator.GetInstigatorEntity();
			if (!ChimeraCharacter.Cast(killer))
			{
				int instigatorPlayerId = damageContext.instigator.GetInstigatorPlayerID();
				if (instigatorPlayerId > 0)
				{
					IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(instigatorPlayerId);
					if (ChimeraCharacter.Cast(controlledEntity))
						killer = controlledEntity;
				}
			}
		}

		IEntity damageSource = damageContext.damageSource;
		if (killer)
		{
			CaptureKiller(killer);
			CaptureProjectileLineOrigin(killer, damageContext);
		}
		else if (victim)
		{
			m_vLineOrigin = victim.GetOrigin();
			m_sLineOriginMode = "victim-fallback";
		}

		// For blast damage, the explosive's final position is more truthful than
		// the thrower's current position. Fragment projectiles retain their own
		// impact path, so they continue to point back to the instigator.
		if (damageSource && damageContext.damageType == EDamageType.EXPLOSIVE && !HasKillerVehicle())
		{
			m_vLineOrigin = damageSource.GetOrigin();
			m_sLineOriginMode = "explosive-source";
		}

		CaptureVictimVisual(victim);
	}

	//! One-shot server fallback for killers that were not streamed to the owning
	//! client. It deliberately performs no continuous damage tracking.
	void CaptureDeathFallback(IEntity victim, IEntity killer)
	{
		if (victim)
		{
			m_vHitPosition = victim.GetOrigin();
			m_vLineOrigin = m_vHitPosition;
		}
		if (killer)
			CaptureKiller(killer);
	}

	//! Client damage facts carry the precise impact/explosive origin. Prefer its
	//! killer pose too when that entity existed on the owning client.
	void MergeLocalDamage(GC_KilledFromSnapshot localDamage)
	{
		if (!localDamage)
			return;

		if (localDamage.m_bHasLocalDamageCapture)
		{
			m_bHasLocalDamageCapture = true;
			m_vLineOrigin = localDamage.m_vLineOrigin;
			m_vHitPosition = localDamage.m_vHitPosition;
			m_sLineOriginMode = localDamage.m_sLineOriginMode;
		}
		m_aVictimPreviewEntries = localDamage.m_aVictimPreviewEntries;
		m_sVictimPrefab = localDamage.m_sVictimPrefab;
		m_sVictimVisualObject = localDamage.m_sVictimVisualObject;
		Math3D.MatrixCopy(localDamage.m_aVictimRootTransform, m_aVictimRootTransform);
		m_aVictimBonePose = localDamage.m_aVictimBonePose;
		m_VictimWeapon = localDamage.m_VictimWeapon;
		m_VictimVehicle = localDamage.m_VictimVehicle;
		m_fVictimLean = localDamage.m_fVictimLean;
		m_vVictimAimingAngles = localDamage.m_vVictimAimingAngles;
		m_iVictimStance = localDamage.m_iVictimStance;
		m_fVictimDynamicStance = localDamage.m_fVictimDynamicStance;
		m_iVictimFlags = localDamage.m_iVictimFlags;
		if (!localDamage.HasKillerPreview())
			return;

		m_sKillerPrefab = localDamage.m_sKillerPrefab;
		m_vKillerPosition = localDamage.m_vKillerPosition;
		m_vKillerAngles = localDamage.m_vKillerAngles;
		m_iStance = localDamage.m_iStance;
		m_fDynamicStance = localDamage.m_fDynamicStance;
		m_fKillerLean = localDamage.m_fKillerLean;
		m_vKillerAimingAngles = localDamage.m_vKillerAimingAngles;
		m_iFlags = localDamage.m_iFlags;
		m_aKillerPreviewEntries = localDamage.m_aKillerPreviewEntries;
		m_sKillerVisualObject = localDamage.m_sKillerVisualObject;
		Math3D.MatrixCopy(localDamage.m_aKillerRootTransform, m_aKillerRootTransform);
		m_aKillerBonePose = localDamage.m_aKillerBonePose;
		m_KillerWeapon = localDamage.m_KillerWeapon;
		m_KillerVehicle = localDamage.m_KillerVehicle;
	}

	protected void CaptureKiller(IEntity killer)
	{
		m_vKillerPosition = killer.GetOrigin();
		m_vKillerAngles = killer.GetAngles();
		m_vLineOrigin = m_vKillerPosition;
		m_sLineOriginMode = "killer-origin-fallback";

		BaseCompartmentSlot occupiedCompartment;
		IEntity vehicle = ResolveVehicle(killer, occupiedCompartment);
		if (vehicle)
		{
			m_KillerVehicle = new GC_KilledFromVehicleState();
			m_KillerVehicle.Capture(vehicle, occupiedCompartment);
			m_vLineOrigin = vehicle.GetOrigin();
			m_sLineOriginMode = "vehicle-origin-fallback";
		}

		ChimeraCharacter character = ChimeraCharacter.Cast(killer);
		if (character || !vehicle)
		{
			EntityPrefabData prefabData = killer.GetPrefabData();
			if (prefabData)
				m_sKillerPrefab = prefabData.GetPrefabName();
			CaptureVisual(killer, "killer", m_aKillerPreviewEntries, m_sKillerVisualObject, m_aKillerRootTransform, m_aKillerBonePose);
		}
		if (m_sKillerPrefab != string.Empty || HasKillerVehicle())
			m_iFlags |= FLAG_VALID_KILLER;

		if (!character)
			return;

		CaptureCharacterState(character, "killer", m_iStance, m_fDynamicStance, m_fKillerLean, m_vKillerAimingAngles, m_iFlags);
		m_KillerWeapon = new GC_KilledFromWeaponState();
		m_KillerWeapon.Capture(character);
	}

	protected void CaptureCharacterState(IEntity entity, string role, out int stance, out float dynamicStance, out float lean, out vector aimingAngles, inout int flags)
	{
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
		{
			PrintFormat("GC | Killed From | event=character-state-capture role=%1 result=no-controller entity=%2", role, entity, level: LogLevel.WARNING);
			return;
		}
		stance = controller.GetStance();
		dynamicStance = controller.GetDynamicStance();
		float currentLean = controller.GetCurrentLeanAmount();
		lean = currentLean;
		CharacterInputContext inputContext = controller.GetInputContext();
		float inputLean;
		bool inputADS;
		if (inputContext)
		{
			inputLean = inputContext.GetLean();
			lean = inputLean;
			aimingAngles = inputContext.GetAimingAngles();
			inputADS = inputContext.IsWeaponADS();
		}
		if (controller.IsWeaponRaised())
			flags |= FLAG_WEAPON_RAISED;
		if (controller.IsWeaponADS() || inputADS)
			flags |= FLAG_WEAPON_ADS;
		PrintFormat("GC | Killed From | event=character-state-capture role=%1 result=complete entity=%2 controller=%3 inputContext=%4 stance=%5 dynamicStance=%6 currentLean=%7 inputLean=%8 selectedLean=%9",
			role, entity, controller, inputContext, stance, dynamicStance, currentLean, inputLean, lean);
		PrintFormat("GC | Killed From | event=character-state-capture-input role=%1 aimingAngles=%2 controllerRaised=%3 controllerADS=%4 inputADS=%5 flags=%6",
			role, aimingAngles, controller.IsWeaponRaised(), controller.IsWeaponADS(), inputADS, flags);
	}

	protected IEntity ResolveVehicle(IEntity entity, out BaseCompartmentSlot occupiedCompartment)
	{
		occupiedCompartment = null;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
			if (access)
			{
				occupiedCompartment = access.GetCompartment();
				IEntity compartmentOwner = access.GetVehicleCompartmentManagerOwner();
				if (occupiedCompartment && compartmentOwner)
					return compartmentOwner;
			}
		}

		IEntity hierarchyEntity = entity;
		while (hierarchyEntity)
		{
			if (Vehicle.Cast(hierarchyEntity))
				return hierarchyEntity;
			hierarchyEntity = hierarchyEntity.GetParent();
		}
		return null;
	}

	//! Reconstruct the incoming projectile ray from the impact facts. Projecting
	//! the attacker onto that ray starts the trace at firing height rather than
	//! at the character entity's ground-level origin.
	protected void CaptureProjectileLineOrigin(IEntity killer, notnull BaseDamageContext damageContext)
	{
		if (damageContext.damageType == EDamageType.EXPLOSIVE && !HasKillerVehicle())
			return;

		vector trajectory = damageContext.impactVelocity;
		m_sLineOriginMode = "impact-velocity-ray";
		if (trajectory.LengthSq() < 0.0001)
		{
			trajectory = damageContext.hitDirection;
			m_sLineOriginMode = "hit-direction-ray";
		}
		if (trajectory.LengthSq() < 0.0001)
		{
			m_sLineOriginMode = "killer-origin-fallback";
			return;
		}

		trajectory.Normalize();
		array<vector> shotAnchors = {killer.GetOrigin()};
		CharacterWeaponManagerComponent characterWeaponManager = CharacterWeaponManagerComponent.Cast(killer.FindComponent(CharacterWeaponManagerComponent));
		if (characterWeaponManager)
		{
			array<WeaponSlotComponent> weaponSlots = {};
			characterWeaponManager.GetWeaponsSlots(weaponSlots);
			foreach (WeaponSlotComponent weaponSlot : weaponSlots)
			{
				if (weaponSlot && weaponSlot.GetWeaponEntity())
					shotAnchors.Insert(weaponSlot.GetWeaponEntity().GetOrigin());
			}
		}

		BaseCompartmentSlot occupiedCompartment;
		IEntity vehicle = ResolveVehicle(killer, occupiedCompartment);
		if (vehicle)
		{
			shotAnchors.Insert(vehicle.GetOrigin());
			array<IEntity> hierarchy = {vehicle};
			SCR_EntityHelper.GetHierarchyEntityList(vehicle, hierarchy);
			foreach (IEntity hierarchyEntity : hierarchy)
			{
				array<Managed> components = {};
				hierarchyEntity.FindComponents(TurretControllerComponent, components);
				foreach (Managed component : components)
				{
					TurretControllerComponent turret = TurretControllerComponent.Cast(component);
					if (!turret || !turret.GetWeaponManager())
						continue;
					array<BaseWeaponComponent> weapons = {};
					turret.GetWeaponManager().GetWeapons(weapons);
					foreach (BaseWeaponComponent weapon : weapons)
					{
						if (weapon && weapon.GetOwner())
							shotAnchors.Insert(weapon.GetOwner().GetOrigin());
					}
				}
			}
		}

		float bestDistanceSq = 1.0e30;
		foreach (vector shotAnchor : shotAnchors)
		{
			float distanceAlongTrajectory = vector.Dot(shotAnchor - damageContext.hitPosition, trajectory);
			vector projectedAnchor = damageContext.hitPosition + trajectory * distanceAlongTrajectory;
			float distanceSq = (shotAnchor - projectedAnchor).LengthSq();
			if (distanceSq >= bestDistanceSq)
				continue;
			bestDistanceSq = distanceSq;
			m_vLineOrigin = projectedAnchor;
		}
		if (vehicle)
			m_sLineOriginMode = "vehicle-weapon-ray";
		else
			m_sLineOriginMode = "character-weapon-ray";
	}

	//! Refresh immediately before local possession leaves the dead character.
	//! This preserves the terminal death location and pose even when the retained
	//! attacker came from an earlier hit that caused unconsciousness or bleed-out.
	void CaptureVictimVisual(IEntity victim)
	{
		m_iVictimFlags = 0;
		m_VictimVehicle = null;
		m_VictimWeapon = null;
		CaptureVisual(victim, "victim", m_aVictimPreviewEntries, m_sVictimVisualObject, m_aVictimRootTransform, m_aVictimBonePose);
		EntityPrefabData victimPrefabData;
		if (victim)
			victimPrefabData = victim.GetPrefabData();
		if (victimPrefabData)
			m_sVictimPrefab = victimPrefabData.GetPrefabName();
		ChimeraCharacter victimCharacter = ChimeraCharacter.Cast(victim);
		if (!victimCharacter)
			return;
		BaseCompartmentSlot occupiedCompartment;
		IEntity victimVehicle = ResolveVehicle(victim, occupiedCompartment);
		if (victimVehicle)
		{
			m_VictimVehicle = new GC_KilledFromVehicleState();
			m_VictimVehicle.Capture(victimVehicle, occupiedCompartment);
		}
		CaptureCharacterState(victimCharacter, "victim", m_iVictimStance, m_fVictimDynamicStance, m_fVictimLean, m_vVictimAimingAngles, m_iVictimFlags);
		m_VictimWeapon = new GC_KilledFromWeaponState();
		m_VictimWeapon.Capture(victimCharacter);
	}

	protected void CaptureVisual(IEntity entity, string role, out array<ref SCR_BasePreviewEntry> entries, out ResourceName visualObjectName, out vector rootTransform[4], out array<ref GC_KilledFromBonePose> bonePose)
	{
		entries = {};
		visualObjectName = ResourceName.Empty;
		bonePose = {};
		if (!entity)
			return;

		// Preserve the entity's literal world matrix. The generic preview helper
		// deliberately rewrites near-vertical roots, which is inappropriate for
		// a forensic killed-from reconstruction.
		entity.GetWorldTransform(rootTransform);
		EntitySpawnParams spawnParams;
		// Ignore character preview-component shortcuts so the captured render
		// hierarchy includes the head, worn gear, weapon, and their bone pivots.
		entries = SCR_GenericPreviewEntity.GetPreviewEntriesFromEntity(entity, spawnParams, EPreviewEntityFlag.IGNORE_PREFAB);
		VObject visualObject = entity.GetVObject();
		if (visualObject)
			visualObjectName = visualObject.GetResourceName();

		Animation animation = entity.GetAnimation();
		if (!animation)
			return;

		array<string> boneNames = {};
		animation.GetBoneNames(boneNames);
		int localBoneCount;
		foreach (string boneName : boneNames)
		{
			TNodeId boneIndex = animation.GetBoneIndex(boneName);
			GC_KilledFromBonePose pose = new GC_KilledFromBonePose();
			pose.m_sBoneName = boneName;
			if (!animation.GetBoneMatrix(boneIndex, pose.m_aTransform))
				continue;
			pose.m_bHasLocalTransform = animation.GetBoneLocalMatrix(boneIndex, pose.m_aLocalTransform);
			if (pose.m_bHasLocalTransform)
				localBoneCount++;
			bonePose.Insert(pose);
			if (bonePose.Count() <= 8)
			{
				vector localRotation[3] = {pose.m_aLocalTransform[0], pose.m_aLocalTransform[1], pose.m_aLocalTransform[2]};
				vector localAngles;
				float localScale = Math3D.MatrixToAnglesAndScale(localRotation, localAngles);
				PrintFormat("GC | Killed From | event=bone-capture-sample role=%1 index=%2 name=%3 modelTranslation=%4 hasLocal=%5 localTranslation=%6 localAngles=%7 localScale=%8", role, bonePose.Count() - 1, boneName, pose.m_aTransform[3], pose.m_bHasLocalTransform, pose.m_aLocalTransform[3], localAngles, localScale);
			}
		}
		PrintFormat("GC | Killed From | event=bone-capture-summary role=%1 entity=%2 boneNames=%3 modelMatrices=%4 localMatrices=%5 visualObject=%6 rootTransform=%7", role, entity, boneNames.Count(), bonePose.Count(), localBoneCount, visualObjectName, rootTransform);
	}

	void SetNetworkData(ResourceName killerPrefab, vector killerPosition, vector killerAngles, vector lineOrigin, vector hitPosition, int stance, float dynamicStance, int flags)
	{
		m_sKillerPrefab = killerPrefab;
		m_vKillerPosition = killerPosition;
		m_vKillerAngles = killerAngles;
		m_vLineOrigin = lineOrigin;
		m_vHitPosition = hitPosition;
		m_iStance = stance;
		m_fDynamicStance = dynamicStance;
		m_iFlags = flags;
	}

	void SetNetworkKillerVehicleData(ResourceName prefab, vector position, vector angles, ECompartmentType compartmentType, int compartmentIndex)
	{
		if (prefab == ResourceName.Empty)
			return;
		m_KillerVehicle = new GC_KilledFromVehicleState();
		m_KillerVehicle.SetNetworkData(prefab, position, angles, compartmentType, compartmentIndex);
		m_iFlags |= FLAG_VALID_KILLER;
	}

	ResourceName GetKillerPrefab() { return m_sKillerPrefab; }
	ResourceName GetVictimPrefab() { return m_sVictimPrefab; }
	vector GetKillerPosition() { return m_vKillerPosition; }
	vector GetKillerAngles() { return m_vKillerAngles; }
	vector GetLineOrigin() { return m_vLineOrigin; }
	vector GetHitPosition() { return m_vHitPosition; }
	int GetStance() { return m_iStance; }
	float GetDynamicStance() { return m_fDynamicStance; }
	float GetKillerLean() { return m_fKillerLean; }
	vector GetKillerAimingAngles() { return m_vKillerAimingAngles; }
	int GetFlags() { return m_iFlags; }
	int GetVictimStance() { return m_iVictimStance; }
	float GetVictimDynamicStance() { return m_fVictimDynamicStance; }
	float GetVictimLean() { return m_fVictimLean; }
	vector GetVictimAimingAngles() { return m_vVictimAimingAngles; }
	int GetVictimFlags() { return m_iVictimFlags; }
	GC_KilledFromVehicleState GetKillerVehicle() { return m_KillerVehicle; }
	GC_KilledFromVehicleState GetVictimVehicle() { return m_VictimVehicle; }
	GC_KilledFromWeaponState GetKillerWeapon() { return m_KillerWeapon; }
	GC_KilledFromWeaponState GetVictimWeapon() { return m_VictimWeapon; }
	bool HasKillerVehicle() { return m_KillerVehicle && m_KillerVehicle.IsValid(); }
	bool HasVictimVehicle() { return m_VictimVehicle && m_VictimVehicle.IsValid(); }
	bool HasKillerPreview() { return (m_iFlags & FLAG_VALID_KILLER) != 0; }
	bool HasKillerVisual() { return m_sKillerVisualObject != ResourceName.Empty; }
	bool HasVictimVisual() { return m_sVictimVisualObject != ResourceName.Empty; }
	array<ref SCR_BasePreviewEntry> GetKillerPreviewEntries() { return m_aKillerPreviewEntries; }
	array<ref SCR_BasePreviewEntry> GetVictimPreviewEntries() { return m_aVictimPreviewEntries; }
	ResourceName GetKillerVisualObject() { return m_sKillerVisualObject; }
	ResourceName GetVictimVisualObject() { return m_sVictimVisualObject; }
	array<ref GC_KilledFromBonePose> GetKillerBonePose() { return m_aKillerBonePose; }
	array<ref GC_KilledFromBonePose> GetVictimBonePose() { return m_aVictimBonePose; }
	void GetKillerRootTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aKillerRootTransform, transform); }
	void GetVictimRootTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aVictimRootTransform, transform); }
	string GetDebugReport()
	{
		int killerBoneCount;
		if (m_aKillerBonePose)
			killerBoneCount = m_aKillerBonePose.Count();
		int victimBoneCount;
		if (m_aVictimBonePose)
			victimBoneCount = m_aVictimBonePose.Count();
		string killerVehicleReport = "none";
		if (m_KillerVehicle)
			killerVehicleReport = m_KillerVehicle.GetDebugReport();
		string victimVehicleReport = "none";
		if (m_VictimVehicle)
			victimVehicleReport = m_VictimVehicle.GetDebugReport();
		string report = string.Format("valid=%1 prefab=%2 killerPosition=%3 killerAngles=%4 lineOrigin=%5 hitPosition=%6",
			HasKillerPreview(), m_sKillerPrefab, m_vKillerPosition, m_vKillerAngles, m_vLineOrigin, m_vHitPosition);
		report = report + string.Format(" stance=%1 dynamicStance=%2 killerLean=%3 victimLean=%4 killerAim=%5 victimAim=%6 weaponRaised=%7 ads=%8", m_iStance, m_fDynamicStance,
			m_fKillerLean, m_fVictimLean, m_vKillerAimingAngles, m_vVictimAimingAngles, (m_iFlags & FLAG_WEAPON_RAISED) != 0, (m_iFlags & FLAG_WEAPON_ADS) != 0);
		report = report + string.Format(" flags=%1 localDamage=%2 lineOriginMode=%3", m_iFlags, m_bHasLocalDamageCapture, m_sLineOriginMode);
		return report + string.Format(" killerVisual=%1 killerObject=%2 killerBones=%3 victimPrefab=%4 victimVisual=%5 victimObject=%6 victimBones=%7 killerVehicle={%8} victimVehicle={%9}", HasKillerVisual(), m_sKillerVisualObject, killerBoneCount, m_sVictimPrefab, HasVictimVisual(), m_sVictimVisualObject, victimBoneCount, killerVehicleReport, victimVehicleReport);
	}
}

//! One owning-client adapter for the locally controlled playable entity. It
//! retains the latest damage plus the last direct external damage source.
class GC_KilledFromDamageTracker
{
	protected IEntity m_Victim;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected ref GC_KilledFromSnapshot m_LastExternalSnapshot;

	bool Start(IEntity victim)
	{
		if (!victim)
		{
			Print("GC | Killed From | event=tracker-start result=rejected reason=no-victim", LogLevel.WARNING);
			return false;
		}

		m_DamageManager = SCR_DamageManagerComponent.Cast(victim.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
		{
			PrintFormat("GC | Killed From | event=tracker-start result=rejected reason=no-damage-manager victim=%1", victim, level: LogLevel.WARNING);
			return false;
		}

		m_Victim = victim;
		m_DamageManager.GetOnDamage().Insert(OnDamage);
		PrintFormat("GC | Killed From | event=tracker-start result=subscribed victim=%1 damageManager=%2", m_Victim, m_DamageManager);
		return true;
	}

	void Destroy()
	{
		GC_KilledFromSnapshot retainedSnapshot = GetSnapshot();
		if (retainedSnapshot)
			PrintFormat("GC | Killed From | event=tracker-stop victim=%1 retainedSnapshot={%2}", m_Victim, retainedSnapshot.GetDebugReport());
		else
			PrintFormat("GC | Killed From | event=tracker-stop victim=%1 retainedSnapshot=none", m_Victim);

		if (m_DamageManager)
			m_DamageManager.GetOnDamage().Remove(OnDamage);
		m_Victim = null;
		m_DamageManager = null;
		m_Snapshot = null;
		m_LastExternalSnapshot = null;
	}

	GC_KilledFromSnapshot GetSnapshot()
	{
		if (m_LastExternalSnapshot)
			return m_LastExternalSnapshot;
		return m_Snapshot;
	}

	void CaptureTerminalVictim()
	{
		if (!m_Snapshot)
			m_Snapshot = new GC_KilledFromSnapshot();
		m_Snapshot.CaptureVictimVisual(m_Victim);
		if (m_LastExternalSnapshot && m_LastExternalSnapshot != m_Snapshot)
			m_LastExternalSnapshot.CaptureVictimVisual(m_Victim);
	}

	protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity rawInstigatorEntity;
		int rawInstigatorPlayerId;
		if (damageContext.instigator)
		{
			rawInstigatorEntity = damageContext.instigator.GetInstigatorEntity();
			rawInstigatorPlayerId = damageContext.instigator.GetInstigatorPlayerID();
		}
		PrintFormat("GC | Killed From | event=damage-raw type=%1 victim=%2 damageSource=%3 instigator=%4 instigatorPlayer=%5 hitPosition=%6 hitDirection=%7 impactVelocity=%8 retainedExternal=%9", damageContext.damageType, m_Victim, damageContext.damageSource, rawInstigatorEntity, rawInstigatorPlayerId, damageContext.hitPosition, damageContext.hitDirection, damageContext.impactVelocity, m_LastExternalSnapshot != null);

		if (damageContext.damageType == EDamageType.HEALING || damageContext.damageType == EDamageType.REGENERATION)
		{
			PrintFormat("GC | Killed From | event=damage result=ignored reason=non-damage type=%1 victim=%2", damageContext.damageType, m_Victim);
			return;
		}

		bool passiveDamage = damageContext.damageType == EDamageType.BLEEDING || damageContext.damageType == EDamageType.FIRE;
		if (passiveDamage && m_Snapshot)
		{
			PrintFormat("GC | Killed From | event=damage result=ignored reason=passive-after-capture type=%1 victim=%2 retained={%3}", damageContext.damageType, m_Victim, GetSnapshot().GetDebugReport());
			return;
		}

		// Some projectiles emit a second direct callback with the same external
		// instigator but no impact facts. It must not overwrite the complete shot
		// retained milliseconds earlier.
		bool emptyImpact = damageContext.hitPosition.LengthSq() < 0.0001 && damageContext.hitDirection.LengthSq() < 0.0001 && damageContext.impactVelocity.LengthSq() < 0.0001;
		if (!passiveDamage && emptyImpact && m_LastExternalSnapshot)
		{
			PrintFormat("GC | Killed From | event=damage result=ignored reason=incomplete-duplicate type=%1 victim=%2 damageSource=%3 instigator=%4 instigatorPlayer=%5 retained={%6}", damageContext.damageType, m_Victim, damageContext.damageSource, rawInstigatorEntity, rawInstigatorPlayerId, m_LastExternalSnapshot.GetDebugReport(), level: LogLevel.WARNING);
			return;
		}

		GC_KilledFromSnapshot snapshot = new GC_KilledFromSnapshot();
		snapshot.Capture(m_Victim, damageContext);
		m_Snapshot = snapshot;

		// A later bleed-out, unconscious respawn, or self-attributed damage tick
		// must not replace the player/AI/vehicle that directly caused the state.
		IEntity instigatorEntity = rawInstigatorEntity;
		int instigatorPlayerId = rawInstigatorPlayerId;
		int victimPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_Victim);
		bool externalEntity = instigatorEntity && instigatorEntity != m_Victim;
		bool externalPlayer = instigatorPlayerId > 0 && instigatorPlayerId != victimPlayerId;
		if (!passiveDamage && (externalEntity || externalPlayer))
			m_LastExternalSnapshot = snapshot;

		PrintFormat("GC | Killed From | event=damage result=captured type=%1 victim=%2 victimPlayer=%3 damageSource=%4 instigator=%5 instigatorPlayer=%6 passive=%7 retainedAsExternal=%8 snapshot={%9}",
			damageContext.damageType, m_Victim, victimPlayerId, damageContext.damageSource, instigatorEntity, instigatorPlayerId, passiveDamage,
			m_LastExternalSnapshot == snapshot, snapshot.GetDebugReport());
	}
}
