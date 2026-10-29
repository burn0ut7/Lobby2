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
		vector center = "1200 0 2740";
		center[1] = m_World.GetSurfaceY(center[0], center[2]) + 0.2;
		params.Transform[3] = center;
		m_SourceVehicle = GetGame().SpawnEntityPrefabLocal(Resource.Load("Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et"), m_World, params);
		if (!m_SourceVehicle)
			return "FAIL vehicle spawn";
		params.Transform[3] = center + "0 0 -4";
		m_SourceGunner = GetGame().SpawnEntityPrefabLocal(Resource.Load(characterPrefab), m_World, params);
		params.Transform[3] = center + "-10 0 8";
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
		m_SourceTurret.GetTurretComponent().SetBaseRotation(-90);
		m_SourceTurret.SetAimingAngles(0.15, 0.12);
		m_SourceTurret.GetTurretComponent().SetAimingRotation("0.15 0.12 0");
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
		m_SourceTurret.GetTurretComponent().SetBaseRotation(-90);
		m_SourceTurret.SetAimingAngles(0.15, 0.12);
		m_SourceTurret.GetTurretComponent().SetAimingRotation("0.15 0.12 0");
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
		if (kind == "bleed")
			context.damageType = EDamageType.BLEEDING;
		if (kind != "respawn-empty")
			m_Tracker.GC_TestDeliverDamage(context);
		if (kind == "respawn" || kind == "respawn-empty")
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
		vector actorExpected[4];
		vector actorActual[4];
		m_Snapshot.GetKillerRootTransform(actorExpected);
		gunner.GetWorldTransform(actorActual);
		float actorError;
		for (int row; row < 4; row++)
			actorError = Math.Max(actorError, vector.Distance(actorExpected[row], actorActual[row]));
		result += " actorMatrixError=" + actorError;
		vector muzzle[4];
		turret.GetWeaponManager().GetCurrentMuzzleTransform(muzzle);
		vector rotation = turret.GetTurretComponent().GetAimingRotation() * Math.DEG2RAD;
		vector expectedRotation = m_Snapshot.GetKillerVehicle().GetTurrets()[0].GetRawAimingRotation();
		result += string.Format(" sourceBase=%1 cloneBase=%2 capturedBase=%3", m_SourceTurret.GetTurretComponent().GetBaseRotation(), turret.GetTurretComponent().GetBaseRotation(), m_Snapshot.GetKillerVehicle().GetTurrets()[0].GetBaseRotation());
		result += string.Format(" sourceRotation=%1 wanted=%2 raw=%3", m_SourceTurret.GetTurretComponent().GetAimingRotation(), m_SourceTurret.GetTurretComponent().GetAimingRotationWanted(), m_SourceTurret.GetTurretComponent().GetRawAimingRotation());
		return result + string.Format(" matrixError=%1 aimError=%2 muzzleError=%3 seat=%4 aim=%5 actualMuzzle=%6 expectedMuzzle=%7 turretOrigin=%8 sourceTurretOrigin=%9", matrixError, vector.Distance(rotation, expectedRotation), vector.Distance(muzzle[3], m_Snapshot.GetLineOrigin()), seat.GetOccupant() == gunner, rotation, muzzle[3], m_Snapshot.GetLineOrigin(), turret.GetOwner().GetOrigin(), m_SourceTurret.GetOwner().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	string CheckAttribution()
	{
		if (!m_SourceVictim || !m_SourceGunner)
			return "UNAVAILABLE fixture";
		GC_KilledFromDamageTracker tracker = new GC_KilledFromDamageTracker();
		tracker.Start(m_SourceVictim);
		BaseDamageContext context = new BaseDamageContext();
		context.damageType = EDamageType.KINETIC;
		context.instigator = Instigator.CreateInstigator(m_SourceGunner);
		context.hitPosition = m_SourceVictim.GetOrigin() + "0 1.5 0";
		tracker.GC_TestDeliverDamage(context);
		ref GC_KilledFromSnapshot shot = tracker.GetSnapshot();
		string guard = tracker.GC_TestGuard(context);
		context.hitPosition = vector.Zero;
		context.hitDirection = vector.Zero;
		context.impactVelocity = vector.Zero;
		tracker.GC_TestDeliverDamage(context);
		bool duplicate = tracker.GetSnapshot() == shot;
		context.damageType = EDamageType.BLEEDING;
		tracker.GC_TestDeliverDamage(context);
		bool bleeding = tracker.GetSnapshot() == shot;
		context.damageType = EDamageType.HEALING;
		tracker.GC_TestDeliverDamage(context);
		bool healing = tracker.GetSnapshot() == shot;
		tracker.MarkRespawn(m_SourceVictim);
		bool respawn = tracker.GetSnapshot() == shot;
		context.instigator = Instigator.CreateInstigator(m_SourceVictim);
		context.damageType = EDamageType.KINETIC;
		tracker.GC_TestDeliverDamage(context);
		bool terminal = tracker.GetSnapshot() == shot;
		tracker.Destroy();

		tracker = new GC_KilledFromDamageTracker();
		tracker.Start(m_SourceVictim);
		tracker.GC_TestDeliverDamage(context);
		bool self = tracker.GetSnapshot().IsSelfInflicted() && !tracker.GetSnapshot().HasKillerPreview() && !tracker.GetSnapshot().HasShotLine();
		context.instigator = Instigator.CreateInstigator(m_SourceGunner);
		context.hitPosition = m_SourceVictim.GetOrigin();
		tracker.GC_TestDeliverDamage(context);
		bool latest = !tracker.GetSnapshot().IsSelfInflicted() && tracker.GetSnapshot().HasKillerPreview();
		context.instigator = Instigator.CreateInstigator(null);
		context.hitPosition = vector.Zero;
		context.hitDirection = vector.Zero;
		context.impactVelocity = vector.Zero;
		tracker.GC_TestDeliverDamage(context);
		bool unknown = !tracker.GetSnapshot().HasKillerPreview();
		tracker.Destroy();

		tracker = new GC_KilledFromDamageTracker();
		tracker.Start(m_SourceVictim);
		tracker.MarkRespawn(m_SourceVictim);
		bool emptyRespawn = tracker.GetSnapshot().IsSelfInflicted() && !tracker.GetSnapshot().HasShotLine();
		tracker.Destroy();
		return guard + string.Format(" duplicate=%1 bleeding=%2 healing=%3 respawn=%4 terminal=%5 self=%6 latest=%7 unknown=%8 emptyRespawn=%9", duplicate, bleeding, healing, respawn, terminal, self, latest, unknown, emptyRespawn);
	}

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
		m_SourceVehicle.SetOrigin(m_SourceVehicle.GetOrigin() + "100 0 100");
		m_SourceVictim.SetOrigin(m_SourceVictim.GetOrigin() + "100 0 100");
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
	string GC_TestGuard(BaseDamageContext context)
	{
		return string.Format("guard: valid=%1 entitySame=%2 playerSame=%3 frame=%4 lastFrame=%5 type=%6 lastType=%7 victimSame=%8", m_Snapshot.HasKillerPreview(), context.instigator.GetInstigatorEntity() == m_LastInstigator, context.instigator.GetInstigatorPlayerID() == m_iLastInstigatorPlayerId, GetGame().GetWorld().GetFrameNumber(), m_iLastDamageFrame, context.damageType, m_eLastDamageType, context.instigator.GetInstigatorEntity() == m_Victim);
	}

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
		if (action == "capture" || action == "self" || action == "self-empty" || action == "respawn" || action == "respawn-empty" || action == "bleed")
			return m_GC_VehicleDeathProbe.Capture(action);
		if (action == "checks")
			return m_GC_VehicleDeathProbe.CheckAttribution();
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
		transform[3] = "1223 47 2733";
		if (m_KilledFromSnapshot && m_KilledFromSnapshot.IsSelfInflicted())
		{
			Math3D.AnglesToMatrix("-45 -18 0", transform);
			transform[3] = m_KilledFromSnapshot.GetHitPosition() + "8 4 -8";
		}
		m_SpectatorCamera.SetTransform(transform);
		return "camera=" + m_SpectatorCamera.GetOrigin();
	}
}
