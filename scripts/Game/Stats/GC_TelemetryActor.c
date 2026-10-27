//! One recording-local life. Identity never reuses a native entity ID.
class GC_TelemetryActor
{
	protected GC_TelemetryRecorder m_Recorder;
	protected IEntity m_Entity;
	protected string m_sId;
	protected string m_sNativeId;
	protected string m_sIdentity;
	protected string m_sLoadout;
	protected string m_sFaction;
	protected string m_sVehicle;
	protected int m_iSeat = -2;
	protected int m_iCompartment = -2;
	protected int m_iAppearanceRevision;
	protected float m_fLastSample = -1000;
	protected float m_fLastPose = -1000;
	protected float m_fLastAppearance = -5000;
	protected bool m_bEnded;
	protected bool m_bSkeleton;
	protected ref array<string> m_aBoneNames = {};
	protected ref array<TNodeId> m_aBoneIds = {};
	protected vector m_vLastPosition;

	//------------------------------------------------------------------------------------------------
	IEntity GetEntity()
	{
		return m_Entity;
	}

	//------------------------------------------------------------------------------------------------
	string GetId()
	{
		return m_sId;
	}

	//------------------------------------------------------------------------------------------------
	string GetNativeId()
	{
		return m_sNativeId;
	}

	//------------------------------------------------------------------------------------------------
	void Start(GC_TelemetryRecorder recorder, IEntity entity, string id, bool wasPlayer, string identity)
	{
		m_Recorder = recorder;
		m_Entity = entity;
		m_sId = id;
		m_sNativeId = entity.GetID().ToString();
		if (wasPlayer)
			m_sIdentity = identity;
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			m_sIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string kind = "ai";
		if (Vehicle.Cast(entity))
			kind = "vehicle";
		else if (playerId > 0 || wasPlayer)
			kind = "player";
		JsonSaveContext context = recorder.BeginRecord("spawn");
		context.WriteValue("entity", id);
		context.WriteValue("kind", kind);
		string prefab;
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();
		context.WriteValue("prefab", prefab);
		context.WriteValue("playerIdentity", m_sIdentity);
		context.WriteValue("faction", FactionKey());
		vector transform[4];
		entity.GetWorldTransform(transform);
		context.WriteValue("transform", GC_TelemetryRecorder.FlattenMatrix(transform));
		recorder.CommitRecord(context);
	}

	//------------------------------------------------------------------------------------------------
	protected string FactionKey()
	{
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(m_Entity.FindComponent(FactionAffiliationComponent));
		if (affiliation && affiliation.GetAffiliatedFaction())
			return affiliation.GetAffiliatedFaction().GetFactionKey();
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void Sample()
	{
		if (m_bEnded)
			return;
		if (!m_Entity || m_Entity.IsDeleted())
		{
			JsonSaveContext removed = m_Recorder.BeginRecord("despawn");
			removed.WriteValue("entity", m_sId);
			removed.WriteValue("reason", "deleted");
			m_Recorder.CommitRecord(removed);
			m_bEnded = true;
			return;
		}
		float now = m_Recorder.Now();
		if (now - m_fLastSample < 200)
			return;
		vector transform[4];
		m_Entity.GetWorldTransform(transform);
		vector velocity;
		if (m_Entity.GetPhysics())
			velocity = m_Entity.GetPhysics().GetVelocity();
		bool discontinuity = m_fLastSample < 0 || now - m_fLastSample > 1000 || vector.DistanceSq(m_vLastPosition, transform[3]) > 10000;
		m_vLastPosition = transform[3];
		m_fLastSample = now;
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Entity);
		string type = "vehicle-state";
		if (character)
			type = "state";
		JsonSaveContext context = m_Recorder.BeginRecord(type);
		context.WriteValue("entity", m_sId);
		context.WriteValue("transform", GC_TelemetryRecorder.FlattenMatrix(transform));
		GC_TelemetryRecorder.WriteVector(context, "velocity", velocity);
		context.WriteValue("speedMps", velocity.Length());
		context.WriteValue("discontinuity", discontinuity);
		if (character)
		{
			GC_SpectatorSample sample = new GC_SpectatorSample();
			if (!sample.CaptureCharacter(character))
				return;
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller.IsWeaponADS() || controller.GetInputContext().IsWeaponADS())
				sample.m_iFlags |= GC_SpectatorSample.FLAG_ADS;
			GC_TelemetryRecorder.WriteVector(context, "aimRadians", sample.m_vAimAngles);
			GC_TelemetryRecorder.WriteVector(context, "lookRadians", sample.m_vLookAngles);
			context.WriteValue("stance", sample.m_iStance);
			context.WriteValue("command", sample.m_iCommand);
			context.WriteValue("flags", sample.m_iFlags);
			context.WriteValue("lean", sample.m_fLean);
			context.WriteValue("dynamicStance", sample.m_fDynamicStance);
			context.WriteValue("movement", sample.m_fMovement);
			CharacterWeaponManagerComponent manager = CharacterWeaponManagerComponent.Cast(character.FindComponent(CharacterWeaponManagerComponent));
			string weaponPrefab;
			int weaponIndex = -1;
			int sightsIndex = -1;
			if (manager)
			{
				BaseWeaponComponent weapon = manager.GetCurrentWeapon();
				if (weapon && weapon.GetOwner() && weapon.GetOwner().GetPrefabData())
				{
					weaponPrefab = weapon.GetOwner().GetPrefabData().GetPrefabName();
					array<IEntity> weapons = {};
					manager.GetWeaponsList(weapons);
					weaponIndex = weapons.Find(weapon.GetOwner());
					sightsIndex = manager.GetCurrentSightsIndex();
				}
			}
			context.WriteValue("weaponPrefab", weaponPrefab);
			context.WriteValue("weaponIndex", weaponIndex);
			context.WriteValue("sightsIndex", sightsIndex);
		}
		m_Recorder.CommitRecord(context);
		if (!character)
			return;
		SampleSeat(character);
		if (now - m_fLastAppearance >= 5000)
		{
			m_fLastAppearance = now;
			SampleAppearance();
		}
		if (m_Recorder.CapturesSkeleton() && now - m_fLastPose >= 1000)
		{
			m_fLastPose = now;
			SamplePose();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SampleSeat(ChimeraCharacter character)
	{
		string vehicleId;
		int seatIndex = -1;
		int compartmentType = -1;
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (access && access.IsInCompartment() && access.GetCompartment())
		{
			IEntity vehicle = access.GetVehicleCompartmentManagerOwner();
			if (!vehicle)
				return;
			SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (manager)
			{
				array<BaseCompartmentSlot> compartments = {};
				ECompartmentType type = access.GetCompartment().GetType();
				manager.GetCompartmentsOfType(compartments, type);
				seatIndex = compartments.Find(access.GetCompartment());
				if (seatIndex < 0)
					return;
				compartmentType = type;
				vehicleId = m_Recorder.EntityId(vehicle);
				if (vehicleId.IsEmpty())
					return;
			}
		}
		if (vehicleId == m_sVehicle && seatIndex == m_iSeat && compartmentType == m_iCompartment)
			return;
		m_sVehicle = vehicleId;
		m_iSeat = seatIndex;
		m_iCompartment = compartmentType;
		JsonSaveContext context = m_Recorder.BeginRecord("seat");
		context.WriteValue("entity", m_sId);
		context.WriteValue("vehicle", vehicleId);
		context.WriteValue("seatIndex", seatIndex);
		context.WriteValue("compartmentType", compartmentType);
		m_Recorder.CommitRecord(context);
	}

	//------------------------------------------------------------------------------------------------
	protected void SampleAppearance()
	{
		if (!m_Entity.GetVObject())
			return;
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_Entity);
		string identity;
		if (playerId > 0)
			identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (identity != m_sIdentity)
		{
			m_sIdentity = identity;
			JsonSaveContext owner = m_Recorder.BeginRecord("ownership");
			owner.WriteValue("entity", m_sId);
			owner.WriteValue("playerIdentity", identity);
			m_Recorder.CommitRecord(owner);
		}
		if (!SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK)
			SCR_ArsenalManagerComponent.GetArsenalLoadoutComponentsToCheck(SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK);
		JsonSaveContext gear = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(m_Entity, gear))
			return;
		string loadout = gear.SaveToString();
		if (loadout.Length() > 65536)
		{
			m_Recorder.Stop("overflow");
			return;
		}
		string faction = FactionKey();
		if (loadout == m_sLoadout && faction == m_sFaction)
			return;
		m_sLoadout = loadout;
		m_sFaction = faction;
		JsonSaveContext context = m_Recorder.BeginRecord("appearance");
		context.WriteValue("entity", m_sId);
		context.WriteValue("revision", m_iAppearanceRevision++);
		context.WriteValue("faction", faction);
		context.WriteValue("visualResource", m_Entity.GetVObject().GetResourceName());
		context.WriteValue("loadoutFormat", "SCR_PlayerArsenalLoadout");
		context.WriteValue("loadout", loadout);
		m_Recorder.CommitRecord(context);
	}

	//------------------------------------------------------------------------------------------------
	protected void SamplePose()
	{
		Animation animation = m_Entity.GetAnimation();
		if (!animation || !m_Entity.GetVObject())
			return;
		if (!m_bSkeleton)
		{
			m_aBoneNames.Clear();
			m_aBoneIds.Clear();
			array<string> names = {};
			animation.GetBoneNames(names);
			foreach (string name : names)
			{
				TNodeId bone = animation.GetBoneIndex(name);
				vector matrix[4];
				if (!animation.GetBoneLocalMatrix(bone, matrix))
					continue;
				m_aBoneNames.Insert(name);
				m_aBoneIds.Insert(bone);
			}
			if (m_aBoneNames.IsEmpty() || m_aBoneNames.Count() > 512)
				return;
			JsonSaveContext skeleton = m_Recorder.BeginRecord("skeleton");
			skeleton.WriteValue("skeletonId", m_sId);
			skeleton.WriteValue("visualResource", m_Entity.GetVObject().GetResourceName());
			skeleton.WriteValue("bones", m_aBoneNames);
			m_bSkeleton = m_Recorder.CommitRecord(skeleton) > 0;
			if (!m_bSkeleton)
				return;
		}
		array<ref array<float>> matrices = {};
		foreach (TNodeId id : m_aBoneIds)
		{
			vector local[4];
			if (!animation.GetBoneLocalMatrix(id, local))
				return;
			matrices.Insert(GC_TelemetryRecorder.FlattenMatrix(local));
		}
		JsonSaveContext context = m_Recorder.BeginRecord("pose");
		context.WriteValue("entity", m_sId);
		context.WriteValue("skeletonId", m_sId);
		context.WriteValue("localMatrices", matrices);
		GC_KilledFromWeaponState weapon = new GC_KilledFromWeaponState();
		weapon.Capture(m_Entity);
		if (weapon.IsValid())
		{
			vector relative[4];
			weapon.GetActorRelativeTransform(relative);
			context.WriteValue("weaponRelativeTransform", GC_TelemetryRecorder.FlattenMatrix(relative));
		}
		m_Recorder.CommitRecord(context);
	}
}
