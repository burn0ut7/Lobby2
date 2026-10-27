//! One captured skeletal bone used by a client-only visual reconstruction.
class GC_KilledFromBonePose
{
	string m_sBoneName;
	vector m_aLocalTransform[4];
	bool m_bHasLocalTransform;
}

class GC_KilledFromWeaponState
{
	protected ResourceName m_sPrefab;
	protected vector m_aWorldTransform[4];
	protected vector m_aActorRelativeTransform[4];
	protected int m_iManagedWeaponIndex = -1;
	protected int m_iSightsIndex = -1;
	protected bool m_bHasCharacterAnimation;
	protected bool m_bWeaponRaisedTag;
	protected bool m_bWeaponADSTag;
	protected bool m_bHasWeaponCommand;
	protected float m_fAimAngleLR;
	protected float m_fAimAngleUD;

	void Capture(IEntity character)
	{
		CharacterAnimationComponent characterAnimation = CharacterAnimationComponent.Cast(character.FindComponent(CharacterAnimationComponent));
		if (characterAnimation)
		{
			m_bHasCharacterAnimation = true;
			m_bWeaponRaisedTag = characterAnimation.IsWeaponRaisedTag();
			m_bWeaponADSTag = characterAnimation.IsWeaponADSTag();
			CharacterCommandHandlerComponent commandHandler = characterAnimation.GetCommandHandler();
			CharacterCommandWeapon weaponCommand;
			if (commandHandler)
				weaponCommand = commandHandler.GetCommandModifier_Weapon();
			if (weaponCommand)
			{
				m_bHasWeaponCommand = true;
				m_fAimAngleLR = weaponCommand.GetAimAngleLR();
				m_fAimAngleUD = weaponCommand.GetAimAngleUD();
			}
		}

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
		vector characterTransform[4];
		character.GetWorldTransform(characterTransform);
		Math3D.MatrixInvMultiply4(characterTransform, m_aWorldTransform, m_aActorRelativeTransform);
		m_iSightsIndex = manager.GetCurrentSightsIndex();
		array<IEntity> weapons = {};
		manager.GetWeaponsList(weapons);
		m_iManagedWeaponIndex = weapons.Find(weaponEntity);
	}

	bool IsValid() { return m_sPrefab != ResourceName.Empty; }
	ResourceName GetPrefab() { return m_sPrefab; }
	int GetManagedWeaponIndex() { return m_iManagedWeaponIndex; }
	int GetSightsIndex() { return m_iSightsIndex; }
	bool HasCharacterAnimation() { return m_bHasCharacterAnimation; }
	bool IsWeaponRaisedTag() { return m_bWeaponRaisedTag; }
	bool IsWeaponADSTag() { return m_bWeaponADSTag; }
	bool HasWeaponCommand() { return m_bHasWeaponCommand; }
	float GetAimAngleLR() { return m_fAimAngleLR; }
	float GetAimAngleUD() { return m_fAimAngleUD; }
	void GetWorldTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aWorldTransform, transform); }
	void GetActorRelativeTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aActorRelativeTransform, transform); }
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

//! Facts required to recreate one killed-from presentation. The authoritative
//! serialized loadout is the sole inventory source used by the renderer; exact
//! pose and inventory travel together in the one-shot owner publication.
class GC_KilledFromSnapshot
{
	static const int FLAG_VALID_KILLER = 1;
	static const int FLAG_WEAPON_RAISED = 2;
	static const int FLAG_WEAPON_ADS = 4;

	protected ResourceName m_sKillerPrefab;
	protected string m_sKillerDisplayName;
	protected bool m_bKillerDisplayNameLocalized;
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
	protected bool m_bHasDamageCapture;
	protected string m_sKillerLoadout;
	protected ResourceName m_sKillerVisualObject;
	protected vector m_aKillerRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aKillerBonePose;
	protected ref GC_KilledFromWeaponState m_KillerWeapon;
	protected ref GC_KilledFromVehicleState m_KillerVehicle;
	protected string m_sVictimLoadout;
	protected ResourceName m_sVictimPrefab;
	protected ResourceName m_sVictimVisualObject;
	protected vector m_aVictimRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aVictimBonePose;
	protected ref GC_KilledFromWeaponState m_VictimWeapon;
	protected ref GC_KilledFromVehicleState m_VictimVehicle;
	protected int m_iVictimStance;
	protected float m_fVictimDynamicStance;
	protected int m_iVictimFlags;

	static const int MAX_NETWORK_BYTES = 524288;

	//------------------------------------------------------------------------------------------------
	//! Sent once to the dead player's owner, never as a continuous pose stream.
	static string Serialize(GC_KilledFromSnapshot snapshot)
	{
		JsonSaveContext context = new JsonSaveContext();
		context.SetMaxDecimalPlaces(6);
		context.WriteValue("version", 1);
		if (!context.WriteValue("snapshot", snapshot))
			return string.Empty;
		string data = context.SaveToString();
		if (data.Length() > MAX_NETWORK_BYTES)
		{
			PrintFormat("GC | Killed From | snapshot exceeds transport limit bytes=%1", data.Length(), level: LogLevel.ERROR);
			return string.Empty;
		}
		return data;
	}

	//------------------------------------------------------------------------------------------------
	static GC_KilledFromSnapshot Deserialize(string data)
	{
		if (data.IsEmpty() || data.Length() > MAX_NETWORK_BYTES)
			return null;
		JsonLoadContext context = new JsonLoadContext();
		int version;
		GC_KilledFromSnapshot snapshot;
		if (!context.LoadFromString(data) || !context.ReadValue("version", version) || version != 1)
			return null;
		if (!context.ReadValue("snapshot", snapshot))
			return null;
		return snapshot;
	}

	void Capture(IEntity victim, notnull BaseDamageContext damageContext)
	{
		m_bHasDamageCapture = true;
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

	//! Terminal capture when deletion has no preceding recorded damage event.
	void CaptureDeathFallback(IEntity victim, IEntity killer)
	{
		if (victim)
		{
			m_vHitPosition = victim.GetOrigin();
			m_vLineOrigin = m_vHitPosition;
		}
		if (killer)
		{
			CaptureKiller(killer);
			BaseWeaponManagerComponent weapons = BaseWeaponManagerComponent.Cast(killer.FindComponent(BaseWeaponManagerComponent));
			vector muzzle[4];
			if (weapons && weapons.GetCurrentMuzzleTransform(muzzle))
			{
				m_vLineOrigin = muzzle[3];
				m_sLineOriginMode = "captured-muzzle";
			}
		}
		CaptureVictimVisual(victim);
	}


	protected void CaptureKiller(IEntity killer)
	{
		m_sKillerDisplayName = string.Empty;
		m_bKillerDisplayNameLocalized = false;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int killerPlayerId = playerManager.GetPlayerIdFromControlledEntity(killer);
			if (killerPlayerId > 0)
				m_sKillerDisplayName = playerManager.GetPlayerName(killerPlayerId);
		}
		if (m_sKillerDisplayName.IsEmpty())
		{
			SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.GetEditableEntity(killer);
			SCR_EditableEntityUIInfo editableInfo;
			if (editableEntity)
				editableInfo = SCR_EditableEntityUIInfo.Cast(editableEntity.GetInfo(killer));
			if (editableInfo)
			{
				m_sKillerDisplayName = editableInfo.GetName();
				m_bKillerDisplayNameLocalized = !m_sKillerDisplayName.IsEmpty();
			}
		}

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
			CaptureVisual(killer, "killer", m_sKillerLoadout, m_sKillerVisualObject, m_aKillerRootTransform, m_aKillerBonePose);
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

		BaseWeaponManagerComponent shotWeaponManager = BaseWeaponManagerComponent.Cast(killer.FindComponent(BaseWeaponManagerComponent));
		vector muzzle[4];
		if (shotWeaponManager && shotWeaponManager.GetCurrentMuzzleTransform(muzzle))
		{
			m_vLineOrigin = muzzle[3];
			m_sLineOriginMode = "captured-muzzle";
			return;
		}

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

	//! Capture at impact, before possession loss and the later death presentation.
	void CaptureVictimVisual(IEntity victim)
	{
		m_iVictimFlags = 0;
		m_VictimVehicle = null;
		m_VictimWeapon = null;
		CaptureVisual(victim, "victim", m_sVictimLoadout, m_sVictimVisualObject, m_aVictimRootTransform, m_aVictimBonePose);
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

	protected void CaptureVisual(IEntity entity, string role, out string loadout, out ResourceName visualObjectName, out vector rootTransform[4], out array<ref GC_KilledFromBonePose> bonePose)
	{
		loadout = string.Empty;
		visualObjectName = ResourceName.Empty;
		bonePose = {};
		if (!entity)
			return;

		// Preserve the entity's literal world matrix. The generic preview helper
		// deliberately rewrites near-vertical roots, which is inappropriate for
		// a forensic killed-from reconstruction.
		entity.GetWorldTransform(rootTransform);
		if (!SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK)
			SCR_ArsenalManagerComponent.GetArsenalLoadoutComponentsToCheck(SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK);
		JsonSaveContext loadoutContext = new JsonSaveContext();
		bool loadoutCaptured = SCR_PlayerArsenalLoadout.ReadLoadoutString(entity, loadoutContext);
		if (loadoutCaptured)
		{
			loadout = loadoutContext.SaveToString();
		}
		else
		{
			PrintFormat("GC | Killed From | event=loadout-capture role=%1 result=failed entity=%2", role, entity, level: LogLevel.ERROR);
		}
		VObject visualObject = entity.GetVObject();
		if (visualObject)
			visualObjectName = visualObject.GetResourceName();

		Animation animation = entity.GetAnimation();
		if (!animation)
			return;

		array<string> boneNames = {};
		animation.GetBoneNames(boneNames);
		foreach (string boneName : boneNames)
		{
			TNodeId boneIndex = animation.GetBoneIndex(boneName);
			GC_KilledFromBonePose pose = new GC_KilledFromBonePose();
			pose.m_sBoneName = boneName;
			pose.m_bHasLocalTransform = animation.GetBoneLocalMatrix(boneIndex, pose.m_aLocalTransform);
			if (!pose.m_bHasLocalTransform)
				continue;
			bonePose.Insert(pose);
		}
	}

	ResourceName GetKillerPrefab() { return m_sKillerPrefab; }
	string GetKillerDisplayName() { return m_sKillerDisplayName; }
	bool IsKillerDisplayNameLocalized() { return m_bKillerDisplayNameLocalized; }
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
	string GetKillerLoadout() { return m_sKillerLoadout; }
	string GetVictimLoadout() { return m_sVictimLoadout; }
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
		string report = string.Format("valid=%1 prefab=%2 killerName=%3 killerNameLocalized=%4 killerPosition=%5 killerAngles=%6 lineOrigin=%7 hitPosition=%8",
			HasKillerPreview(), m_sKillerPrefab, m_sKillerDisplayName, m_bKillerDisplayNameLocalized, m_vKillerPosition, m_vKillerAngles, m_vLineOrigin, m_vHitPosition);
		report = report + string.Format(" stance=%1 dynamicStance=%2 killerLean=%3 victimLean=%4 killerAim=%5 victimAim=%6 weaponRaised=%7 ads=%8", m_iStance, m_fDynamicStance,
			m_fKillerLean, m_fVictimLean, m_vKillerAimingAngles, m_vVictimAimingAngles, (m_iFlags & FLAG_WEAPON_RAISED) != 0, (m_iFlags & FLAG_WEAPON_ADS) != 0);
		report = report + string.Format(" flags=%1 localDamage=%2 lineOriginMode=%3", m_iFlags, m_bHasDamageCapture, m_sLineOriginMode);
		report = report + string.Format(" killerVisual=%1 killerObject=%2 killerBones=%3 killerLoadoutBytes=%4 victimPrefab=%5 victimVisual=%6 victimObject=%7 victimBones=%8 victimLoadoutBytes=%9", HasKillerVisual(), m_sKillerVisualObject, killerBoneCount, m_sKillerLoadout.Length(), m_sVictimPrefab, HasVictimVisual(), m_sVictimVisualObject, victimBoneCount, m_sVictimLoadout.Length());
		return report + string.Format(" killerVehicle={%1} victimVehicle={%2}", killerVehicleReport, victimVehicleReport);
	}
}

//! One event-driven authority adapter per player victim. It
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


	protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;
		IEntity rawInstigatorEntity;
		int rawInstigatorPlayerId;
		if (damageContext.instigator)
		{
			rawInstigatorEntity = damageContext.instigator.GetInstigatorEntity();
			rawInstigatorPlayerId = damageContext.instigator.GetInstigatorPlayerID();
		}

		if (damageContext.damageType == EDamageType.HEALING || damageContext.damageType == EDamageType.REGENERATION)
		{
			return;
		}

		bool passiveDamage = damageContext.damageType == EDamageType.BLEEDING || damageContext.damageType == EDamageType.FIRE;
		if (passiveDamage && m_Snapshot)
		{
			return;
		}

		// Some projectiles emit a second direct callback with the same external
		// instigator but no impact facts. It must not overwrite the complete shot
		// retained milliseconds earlier.
		bool emptyImpact = damageContext.hitPosition.LengthSq() < 0.0001 && damageContext.hitDirection.LengthSq() < 0.0001 && damageContext.impactVelocity.LengthSq() < 0.0001;
		if (!passiveDamage && emptyImpact && m_LastExternalSnapshot)
		{
			return;
		}

		int captureStart = System.GetTickCount();
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

		PrintFormat("GC | Killed From | event=damage-capture server=%1 player=%2 type=%3 captureMs=%4 external=%5",
			Replication.IsServer(), victimPlayerId, damageContext.damageType, System.GetTickCount(captureStart), m_LastExternalSnapshot == snapshot);
	}
}
