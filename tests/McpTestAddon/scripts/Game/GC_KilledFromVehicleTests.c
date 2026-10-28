//! Explicit reconstruction fixture. No automatic invocation or production dependency.
class GC_KilledFromVehicleProbe : GC_SpectatorKilledFromVisualizer
{
	protected IEntity m_SourceVehicle;
	protected IEntity m_SourceGunner;
	protected IEntity m_SourceVictim;
	protected TurretControllerComponent m_SourceTurret;
	protected ref GC_KilledFromDamageTracker m_Tracker;

	//------------------------------------------------------------------------------------------------
	string Arrange(ResourceName characterPrefab)
	{
		Cleanup();
		m_World = GetGame().GetWorld();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix("30 0 0", params.Transform);
		params.Transform[3] = "150 1 150";
		m_SourceVehicle = GetGame().SpawnEntityPrefabLocal(Resource.Load("Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et"), m_World, params);
		if (!m_SourceVehicle)
			return "FAIL vehicle spawn";
		params.Transform[3] = "150 1 146";
		m_SourceGunner = GetGame().SpawnEntityPrefabLocal(Resource.Load(characterPrefab), m_World, params);
		params.Transform[3] = "160 1 165";
		m_SourceVictim = GetGame().SpawnEntityPrefabLocal(Resource.Load(characterPrefab), m_World, params);
		if (!m_SourceGunner || !m_SourceVictim)
			return "FAIL actor spawn";
		AIControlComponent.Cast(m_SourceGunner.FindComponent(AIControlComponent)).DeactivateAI();
		AIControlComponent.Cast(m_SourceVictim.FindComponent(AIControlComponent)).DeactivateAI();
		array<IEntity> hierarchy = {m_SourceVehicle};
		SCR_EntityHelper.GetHierarchyEntityList(m_SourceVehicle, hierarchy);
		foreach (IEntity entity : hierarchy)
		{
			m_SourceTurret = TurretControllerComponent.Cast(entity.FindComponent(TurretControllerComponent));
			if (m_SourceTurret)
				break;
		}
		if (!m_SourceTurret || !m_SourceTurret.GetCompartmentSlot())
			return "FAIL turret";
		ChimeraCharacter gunner = ChimeraCharacter.Cast(m_SourceGunner);
		bool entered = gunner.GetCompartmentAccessComponent().GetInVehicle(m_SourceVehicle, m_SourceTurret.GetCompartmentSlot(), true, -1, ECloseDoorAfterActions.INVALID, false);
		m_SourceTurret.SetAimingAngles(0.45, 0.12);
		m_SourceTurret.GetTurretComponent().SetAimingRotation("0.45 0.12 0");
		GetGame().GetCallqueue().CallLater(DriveTurret, 0, true);
		m_Tracker = new GC_KilledFromDamageTracker();
		m_Tracker.Start(m_SourceVictim);
		return "ARRANGED entered=" + entered;
	}

	//------------------------------------------------------------------------------------------------
	protected void DriveTurret()
	{
		if (!m_SourceTurret)
			return;
		m_SourceTurret.SetAimingAngles(0.45, 0.12);
		m_SourceTurret.GetTurretComponent().SetAimingRotation("0.45 0.12 0");
	}

	//------------------------------------------------------------------------------------------------
	string Capture(string kind)
	{
		if (!m_SourceTurret || !m_SourceVictim)
			return "UNAVAILABLE fixture";
		DestroyPreview();
		BaseDamageContext context = new BaseDamageContext();
		context.damageType = EDamageType.KINETIC;
		context.hitPosition = m_SourceVictim.GetOrigin() + "0 1.5 0";
		context.instigator = Instigator.CreateInstigator(m_SourceGunner);
		vector muzzle[4];
		m_SourceTurret.GetWeaponManager().GetCurrentMuzzleTransform(muzzle);
		context.hitDirection = (context.hitPosition - muzzle[3]).Normalized();
		context.impactVelocity = context.hitDirection * 800;
		if (kind == "self" || kind == "self-empty")
			context.instigator = Instigator.CreateInstigator(m_SourceVictim);
		if (kind == "self-empty")
		{
			context.hitPosition = vector.Zero;
			context.hitDirection = vector.Zero;
			context.impactVelocity = vector.Zero;
		}
		m_Tracker.GC_TestDeliverDamage(context);
		if (kind == "respawn")
			m_Tracker.MarkRespawn(m_SourceVictim);
		m_Snapshot = GC_KilledFromSnapshot.Deserialize(GC_KilledFromSnapshot.Serialize(m_Tracker.GetSnapshot()));
		CreatePreview();
		GetGame().GetCallqueue().CallLater(Advance, 0, true);
		return m_Snapshot.GetDebugReport();
	}

	//------------------------------------------------------------------------------------------------
	protected void Advance()
	{
		UpdatePose();
		if (m_bPoseFrozen)
		{
			ApplyVehicleState(m_KillerVehiclePreviewEntity, m_Snapshot.GetKillerVehicle(), "test");
			DeactivatePhysicsHierarchy(m_KillerVehiclePreviewEntity);
		}
	}

	//------------------------------------------------------------------------------------------------
	string Read()
	{
		if (!m_Snapshot)
			return "UNAVAILABLE snapshot";
		string result = string.Format("self=%1 line=%2 killer=%3 victim=%4 vehicle=%5 frozen=%6", m_Snapshot.IsSelfInflicted(), m_Snapshot.HasShotLine(), m_KillerPreviewEntity != null, m_VictimPreviewEntity != null, m_KillerVehiclePreviewEntity != null, m_bPoseFrozen);
		if (!m_KillerVehiclePreviewEntity)
			return result;
		vector expected[4];
		vector actual[4];
		m_Snapshot.GetKillerVehicle().GetRootTransform(expected);
		m_KillerVehiclePreviewEntity.GetWorldTransform(actual);
		float matrixError;
		for (int index; index < 4; index++)
			matrixError = Math.Max(matrixError, vector.Distance(expected[index], actual[index]));
		ChimeraCharacter gunner = ChimeraCharacter.Cast(m_KillerPreviewEntity);
		BaseCompartmentSlot seat;
		if (gunner)
			seat = gunner.GetCompartmentAccessComponent().GetCompartment();
		TurretControllerComponent turret;
		if (seat)
			turret = TurretControllerComponent.Cast(seat.GetOwner().FindComponent(TurretControllerComponent));
		if (!turret)
			return result + " FAIL cloned gunner seat/turret";
		vector muzzle[4];
		turret.GetWeaponManager().GetCurrentMuzzleTransform(muzzle);
		vector rotation = turret.GetTurretComponent().GetAimingRotation() * Math.DEG2RAD;
		vector expectedRotation = m_Snapshot.GetKillerVehicle().GetTurrets()[0].GetRawAimingRotation();
		result += string.Format(" sourceRotation=%1 wanted=%2 raw=%3", m_SourceTurret.GetTurretComponent().GetAimingRotation(), m_SourceTurret.GetTurretComponent().GetAimingRotationWanted(), m_SourceTurret.GetTurretComponent().GetRawAimingRotation());
		return result + string.Format(" matrixError=%1 aimError=%2 muzzleError=%3 seat=%4 aim=%5 actualMuzzle=%6 expectedMuzzle=%7 turretOrigin=%8 sourceTurretOrigin=%9", matrixError, vector.Distance(rotation, expectedRotation), vector.Distance(muzzle[3], m_Snapshot.GetLineOrigin()), seat.GetOccupant() == gunner, rotation, muzzle[3], m_Snapshot.GetLineOrigin(), turret.GetOwner().GetOrigin(), m_SourceTurret.GetOwner().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	string Publish()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!m_Snapshot || !controller || !Replication.IsServer())
			return "UNAVAILABLE snapshot/authority";
		DestroyPreview();
		GetGame().GetCallqueue().Remove(Advance);
		controller.SendKilledFromSnapshot(m_Snapshot);
		controller.SetSpectatorKilledFromEnabled(true);
		return "DISPATCHED owner publication";
	}

	//------------------------------------------------------------------------------------------------
	void MoveSources()
	{
		m_SourceVehicle.SetOrigin("250 1 250");
		m_SourceVictim.SetOrigin("260 1 265");
	}

	//------------------------------------------------------------------------------------------------
	void Cleanup()
	{
		GetGame().GetCallqueue().Remove(DriveTurret);
		GetGame().GetCallqueue().Remove(Advance);
		DestroyPreview();
		if (m_Tracker)
			m_Tracker.Destroy();
		m_Tracker = null;
		if (m_SourceGunner)
			delete m_SourceGunner;
		if (m_SourceVictim)
			delete m_SourceVictim;
		if (m_SourceVehicle)
			delete m_SourceVehicle;
	}
}

modded class GC_KilledFromDamageTracker
{
	//------------------------------------------------------------------------------------------------
	void GC_TestDeliverDamage(BaseDamageContext context)
	{
		OnDamage(context);
	}
}

modded class GC_LobbyManager
{
	protected ref GC_KilledFromVehicleProbe m_GC_VehicleDeathProbe;

	//------------------------------------------------------------------------------------------------
	string GC_TestVehicleDeath(string action)
	{
		if (GetManager() != this || !Replication.IsServer())
			return "UNAVAILABLE authority";
		if (!m_GC_VehicleDeathProbe)
			m_GC_VehicleDeathProbe = new GC_KilledFromVehicleProbe();
		if (action == "arrange")
			return m_GC_VehicleDeathProbe.Arrange(m_Roster.GetSlot(1).GetResolvedPrefab());
		if (action == "capture" || action == "self" || action == "self-empty" || action == "respawn")
			return m_GC_VehicleDeathProbe.Capture(action);
		if (action == "read")
			return m_GC_VehicleDeathProbe.Read();
		if (action == "publish")
			return m_GC_VehicleDeathProbe.Publish();
		if (action == "view")
			return GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).GC_TestDeathView();
		if (action == "move-sources")
		{
			m_GC_VehicleDeathProbe.MoveSources();
			return "MOVED live sources; snapshot must remain at impact";
		}
		if (action == "cleanup")
		{
			m_GC_VehicleDeathProbe.Cleanup();
			return "CLEANED";
		}
		return "REJECTED action";
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestDeathView()
	{
		if (!m_SpectatorCamera)
			return "UNAVAILABLE spectator camera";
		m_SpectatorCamera.TrySetMode(GC_ESpectatorCameraMode.FREE);
		vector transform[4];
		Math3D.AnglesToMatrix("-55 -12 0", transform);
		transform[3] = "173 7 143";
		if (m_KilledFromSnapshot && m_KilledFromSnapshot.IsSelfInflicted())
		{
			Math3D.AnglesToMatrix("-45 -18 0", transform);
			transform[3] = m_KilledFromSnapshot.GetHitPosition() + "8 4 -8";
		}
		m_SpectatorCamera.SetTransform(transform);
		return "camera=" + m_SpectatorCamera.GetOrigin();
	}
}
