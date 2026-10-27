//! One recording-local life. Identity never reuses a native entity ID.
class GC_TelemetryActor
{
	protected GC_TelemetryRecorder m_Recorder;
	protected IEntity m_Entity;
	protected string m_sId;
	protected EntityID m_NativeId;
	protected string m_sIdentity;
	protected string m_sLoadout;
	protected string m_sFaction;
	protected string m_sVehicle;
	protected int m_iSeat = -2;
	protected int m_iCompartment = -2;
	protected int m_iAppearanceRevision;
	protected float m_fLastSample = -1000;
	protected float m_fLastAppearance = -5000;
	protected bool m_bEnded;
	protected ref GC_SpectatorSample m_LastState;
	protected vector m_vWrittenPosition;
	protected vector m_vWrittenAngles;
	protected vector m_vWrittenVelocity;
	protected bool m_bWrittenDiscontinuity;
	protected string m_sWeapon;
	protected int m_iWeapon = -2;
	protected int m_iSights = -2;
	protected float m_fLastWritten = -5000;
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
	EntityID GetNativeId()
	{
		return m_NativeId;
	}

	//------------------------------------------------------------------------------------------------
	void Start(GC_TelemetryRecorder recorder, IEntity entity, string id, bool wasPlayer, string identity)
	{
		m_Recorder = recorder;
		m_Entity = entity;
		m_sId = id;
		m_NativeId = entity.GetID();
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
		GC_TelemetryRecorder.WriteVector(context, "position", entity.GetOrigin());
		GC_TelemetryRecorder.WriteVector(context, "anglesDegrees", entity.GetAngles());
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
	bool IsDue(float now)
	{
		return m_bEnded || now - m_fLastSample >= 200;
	}

	//------------------------------------------------------------------------------------------------
	bool HasEnded()
	{
		return m_bEnded;
	}

	//------------------------------------------------------------------------------------------------
	void Finish()
	{
		Sample(true);
		m_bEnded = true;
		m_LastState = null;
		m_sLoadout = string.Empty;
		m_sWeapon = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void Sample(bool finalState = false)
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
		ChimeraCharacter observed = ChimeraCharacter.Cast(m_Entity);
		if (!finalState && observed && observed.GetCharacterController() && observed.GetCharacterController().IsDead())
		{
			// An uncredited AI death may not enter the player statistics stream.
			Finish();
			JsonSaveContext ended = m_Recorder.BeginRecord("despawn");
			ended.WriteValue("entity", m_sId);
			ended.WriteValue("reason", "dead");
			m_Recorder.CommitRecord(ended);
			return;
		}
		float now = m_Recorder.Now();
		if (!finalState && now - m_fLastSample < 200)
			return;
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Entity);
		GC_SpectatorSample sample;
		if (character)
		{
			sample = new GC_SpectatorSample();
			if (!sample.CaptureCharacter(character))
				return;
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller.IsWeaponADS() || controller.GetInputContext().IsWeaponADS())
				sample.m_iFlags |= GC_SpectatorSample.FLAG_ADS;
		}
		float previousSample = Math.Max(0, m_fLastSample);
		vector position = m_Entity.GetOrigin();
		vector angles = m_Entity.GetAngles();
		vector velocity;
		if (m_Entity.GetPhysics())
			velocity = m_Entity.GetPhysics().GetVelocity();
		bool discontinuity = m_fLastSample < 0 || now - m_fLastSample > 1000 || vector.DistanceSq(m_vLastPosition, position) > 10000;
		// Advance only after a successful observation, not a failed capture attempt.
		m_vLastPosition = position;
		m_fLastSample = now;
		bool changed = finalState || now - m_fLastWritten >= 5000 || discontinuity != m_bWrittenDiscontinuity;
		changed = changed || vector.DistanceSq(position, m_vWrittenPosition) >= 0.000001;
		changed = changed || vector.DistanceSq(angles, m_vWrittenAngles) >= 0.000001;
		changed = changed || vector.DistanceSq(velocity, m_vWrittenVelocity) >= 0.000001;
		if (sample)
			changed = changed || AnimationChanged(sample);
		if (changed)
		{
			// Compare numeric inputs before allocating/serializing JSON at 128 actors.
			JsonSaveContext context = new JsonSaveContext();
			context.SetMaxDecimalPlaces(3);
			context.WriteValue("entity", m_sId);
			GC_TelemetryRecorder.WriteVector(context, "position", position);
			GC_TelemetryRecorder.WriteVector(context, "anglesDegrees", angles);
			GC_TelemetryRecorder.WriteVector(context, "velocity", velocity);
			context.WriteValue("discontinuity", discontinuity);
			string type = "vehicle-state";
			if (sample)
			{
				type = "state";
				GC_TelemetryRecorder.WriteVector(context, "aimRadians", sample.m_vAimAngles);
				GC_TelemetryRecorder.WriteVector(context, "lookRadians", sample.m_vLookAngles);
				context.WriteValue("stance", sample.m_iStance);
				context.WriteValue("command", sample.m_iCommand);
				context.WriteValue("flags", sample.m_iFlags);
				context.WriteValue("lean", sample.m_fLean);
				context.WriteValue("dynamicStance", sample.m_fDynamicStance);
				context.WriteValue("movement", sample.m_fMovement);
			}
			string state = context.SaveToString();
			// Hold the preceding state until this observation window, then interpolate.
			JsonSaveContext window = new JsonSaveContext();
			window.SetMaxDecimalPlaces(6);
			window.WriteValue("previousSampleMs", previousSample);
			string timing = window.SaveToString();
			m_Recorder.CommitData(type, state.Substring(0, state.Length() - 1) + "," + timing.Substring(1, timing.Length() - 1));
			m_LastState = sample;
			m_vWrittenPosition = position;
			m_vWrittenAngles = angles;
			m_vWrittenVelocity = velocity;
			m_bWrittenDiscontinuity = discontinuity;
			m_fLastWritten = now;
		}
		if (!character)
			return;
		SampleEquipment(character);
		SampleSeat(character);
		if (now - m_fLastAppearance >= 5000)
		{
			m_fLastAppearance = now;
			SampleAppearance(finalState);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool AnimationChanged(GC_SpectatorSample sample)
	{
		if (!m_LastState)
			return true;
		return sample.m_iStance != m_LastState.m_iStance || sample.m_iCommand != m_LastState.m_iCommand
			|| sample.m_iFlags != m_LastState.m_iFlags
			|| vector.DistanceSq(sample.m_vAimAngles, m_LastState.m_vAimAngles) >= 0.000001
			|| vector.DistanceSq(sample.m_vLookAngles, m_LastState.m_vLookAngles) >= 0.000001
			|| Math.AbsFloat(sample.m_fLean - m_LastState.m_fLean) >= 0.001
			|| Math.AbsFloat(sample.m_fDynamicStance - m_LastState.m_fDynamicStance) >= 0.001
			|| Math.AbsFloat(sample.m_fMovement - m_LastState.m_fMovement) >= 0.001;
	}

	//------------------------------------------------------------------------------------------------
	protected void SampleEquipment(ChimeraCharacter character)
	{
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
		if (weaponPrefab == m_sWeapon && weaponIndex == m_iWeapon && sightsIndex == m_iSights)
			return;
		JsonSaveContext equipment = m_Recorder.BeginRecord("equipment");
		equipment.WriteValue("entity", m_sId);
		equipment.WriteValue("weaponPrefab", weaponPrefab);
		equipment.WriteValue("weaponIndex", weaponIndex);
		equipment.WriteValue("sightsIndex", sightsIndex);
		m_Recorder.CommitRecord(equipment);
		m_sWeapon = weaponPrefab;
		m_iWeapon = weaponIndex;
		m_iSights = sightsIndex;
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
	protected void SampleAppearance(bool finalState)
	{
		if (!m_Entity.GetVObject())
			return;
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_Entity);
		string identity;
		if (playerId > 0)
			identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (!finalState && identity != m_sIdentity)
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

}
