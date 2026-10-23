//! Optional input fixture. It never calls the native weapon fire setter.
modded class SCR_TurretControllerComponent
{
	protected float m_fGC_TestTurretPulse;
	protected bool m_bGC_TestTurretRelease;
	protected int m_iGC_TestTurretInputFrames;

	//------------------------------------------------------------------------------------------------
	bool GC_TestTurretPulse()
	{
		if (m_fGC_TestTurretPulse > 0 || m_bGC_TestTurretRelease)
			return false;
		m_fGC_TestTurretPulse = 1;
		m_bGC_TestTurretRelease = true;
		m_iGC_TestTurretInputFrames = 0;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTurretState()
	{
		BaseWeaponManagerComponent weapons = GetWeaponManager();
		if (!weapons || !weapons.GetCurrentWeapon() || !weapons.GetCurrentWeapon().GetCurrentMagazine())
			return "UNAVAILABLE turret weapon/magazine";
		return string.Format("ammo=%1 pulse=%2 inputFrames=%3", weapons.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount(),
			m_fGC_TestTurretPulse > 0 || m_bGC_TestTurretRelease, m_iGC_TestTurretInputFrames);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && (m_fGC_TestTurretPulse > 0 || m_bGC_TestTurretRelease))
		{
			if (m_fGC_TestTurretPulse > 0)
			{
				am.SetActionValue("TurretFire", 1);
				m_fGC_TestTurretPulse -= dt;
				m_iGC_TestTurretInputFrames++;
			}
			else if (m_bGC_TestTurretRelease)
			{
				am.SetActionValue("TurretFire", 0);
				m_bGC_TestTurretRelease = false;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_LobbyFreezeComponent
{
	protected IEntity m_GC_TestTurret;
	protected SCR_TurretControllerComponent m_GC_TestTurretController;

	//------------------------------------------------------------------------------------------------
	protected SCR_TurretControllerComponent GC_TestFindTurret(IEntity entity)
	{
		SCR_TurretControllerComponent turret = SCR_TurretControllerComponent.Cast(entity.FindComponent(SCR_TurretControllerComponent));
		if (turret)
			return turret;
		IEntity child = entity.GetChildren();
		while (child)
		{
			turret = GC_TestFindTurret(child);
			if (turret)
				return turret;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Spawn and enter one stock turret for a local authority test; no role changes.
	string GC_TestTurretSetup()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "UNAVAILABLE local player";
		return GC_TestTurretSetupPlayer(player.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTurretSetupPlayer(int playerId)
	{
		if (!Replication.IsServer() || GetInstance() != this || m_GC_TestTurret)
			return "UNAVAILABLE active authority without existing fixture required";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!character || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE controlled character";
		Resource prefab = Resource.Load("{73530808E4B455BA}Prefabs/Weapons/Tripods/Tripod_M3_M2HB.et");
		if (!prefab || !prefab.IsValid())
			return "UNAVAILABLE stock turret prefab";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = character.GetOrigin() + "0 0 3";
		m_GC_TestTurret = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!m_GC_TestTurret)
			return "FAILED turret spawn";
		m_GC_TestTurretController = GC_TestFindTurret(m_GC_TestTurret);
		if (!m_GC_TestTurretController || !m_GC_TestTurretController.GetCompartmentSlot())
			return "FAILED turret controller/compartment; fixture retained for inspection";
		m_GC_TestTurretController.GetOwner().SetName("GC_TestStagingTurret");
		if (GetGame().GetPlayerController() && GetGame().GetPlayerController().GetControlledEntity() == character)
		{
			character.GetCompartmentAccessComponent().GetInVehicle(m_GC_TestTurret,
				m_GC_TestTurretController.GetCompartmentSlot(), true, -1, ECloseDoorAfterActions.INVALID, false);
			return "DISPATCHED stock compartment entry; inspect later snapshot";
		}
		return "SPAWNED remote fixture; dispatch owner entry after replication";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTurretEnterPlayer(int playerId)
	{
		if (!Replication.IsServer() || GetInstance() != this || !m_GC_TestTurret)
			return "UNAVAILABLE authority fixture";
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		RplComponent replication = RplComponent.Cast(m_GC_TestTurret.FindComponent(RplComponent));
		if (!player || !replication)
			return "UNAVAILABLE player or replicated turret";
		player.GC_TestEnterTurret(replication.Id());
		return "DISPATCHED owner entry; inspect occupied compartment";
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestEnterTurretEntity(IEntity entity)
	{
		if (GetInstance() != this || !entity || !GetGame().GetPlayerController())
			return;
		SCR_TurretControllerComponent turret = GC_TestFindTurret(entity);
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!turret || !character || !character.GetCompartmentAccessComponent())
			return;
		character.GetCompartmentAccessComponent().GetInVehicle(entity,
			turret.GetCompartmentSlot(), true, -1, ECloseDoorAfterActions.INVALID, false);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTurret(string action)
	{
		if (GetInstance() != this)
			return "UNAVAILABLE active mode";
		if (action == "server-snapshot")
		{
			if (!Replication.IsServer() || !m_GC_TestTurretController)
				return "UNAVAILABLE authority fixture turret";
			return string.Format("freeze=%1 %2", IsFreezeActive(), m_GC_TestTurretController.GC_TestTurretState());
		}
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "UNAVAILABLE local player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE character compartment access";
		BaseCompartmentSlot compartment = character.GetCompartmentAccessComponent().GetCompartment();
		if (!compartment)
			return "UNAVAILABLE occupied compartment";
		SCR_TurretControllerComponent turret = SCR_TurretControllerComponent.Cast(compartment.GetController());
		if (!turret)
			return "UNAVAILABLE occupied turret controller";
		bool occupied = compartment == turret.GetCompartmentSlot();
		if (action == "fire")
		{
			if (!occupied || !turret.GC_TestTurretPulse())
				return "REJECTED not occupied or pulse pending";
		}
		else if (action != "snapshot")
			return "REJECTED unsupported action";
		return string.Format("freeze=%1 occupied=%2 %3", IsFreezeActive(), occupied, turret.GC_TestTurretState());
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_TestEnterTurret(RplId turretId)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager())
			Rpc(GC_TestRpcEnterTurret, turretId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_TestRpcEnterTurret(RplId turretId)
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (!freeze || GetGame().GetPlayerController() != this)
			return;
		RplComponent replication = RplComponent.Cast(Replication.FindItem(turretId));
		if (replication)
			freeze.GC_TestEnterTurretEntity(replication.GetEntity());
	}
}
