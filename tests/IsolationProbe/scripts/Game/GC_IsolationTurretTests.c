//! Optional addon-independent mounted-weapon baseline. No automatic firing.
#ifdef WORKBENCH
modded class SCR_TurretControllerComponent
{
	protected float m_fGC_IsolationTurretPulse;
	protected bool m_bGC_IsolationTurretRelease;

	//------------------------------------------------------------------------------------------------
	void GC_IsolationTurretPulse()
	{
		m_fGC_IsolationTurretPulse = 1;
		m_bGC_IsolationTurretRelease = true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && (m_fGC_IsolationTurretPulse > 0 || m_bGC_IsolationTurretRelease))
		{
			if (m_fGC_IsolationTurretPulse > 0)
			{
				am.SetActionValue("TurretFire", 1);
				m_fGC_IsolationTurretPulse -= dt;
			}
			else
			{
				am.SetActionValue("TurretFire", 0);
				m_bGC_IsolationTurretRelease = false;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_IsolationProbeComponent
{
	protected IEntity m_GC_IsolationTurretEntity;
	protected SCR_TurretControllerComponent m_GC_IsolationTurret;

	//------------------------------------------------------------------------------------------------
	protected SCR_TurretControllerComponent GC_FindIsolationTurret(IEntity entity)
	{
		SCR_TurretControllerComponent turret = SCR_TurretControllerComponent.Cast(entity.FindComponent(SCR_TurretControllerComponent));
		if (turret)
			return turret;
		IEntity child = entity.GetChildren();
		while (child)
		{
			turret = GC_FindIsolationTurret(child);
			if (turret)
				return turret;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	string TurretSetup()
	{
		if (!Replication.IsServer() || m_GC_IsolationTurretEntity || !GetGame().GetPlayerController())
			return "UNAVAILABLE authority/player or fixture already exists";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character || character.GetName() != "IsolationCharacter" || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE fixture character";
		Resource prefab = Resource.Load("{73530808E4B455BA}Prefabs/Weapons/Tripods/Tripod_M3_M2HB.et");
		if (!prefab || !prefab.IsValid())
			return "UNAVAILABLE stock turret prefab";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = character.GetOrigin() + "0 0 3";
		m_GC_IsolationTurretEntity = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!m_GC_IsolationTurretEntity)
			return "FAILED spawn";
		m_GC_IsolationTurret = GC_FindIsolationTurret(m_GC_IsolationTurretEntity);
		if (!m_GC_IsolationTurret || !m_GC_IsolationTurret.GetCompartmentSlot())
			return "FAILED controller/compartment; inspect retained fixture";
		character.GetCompartmentAccessComponent().GetInVehicle(m_GC_IsolationTurretEntity,
			m_GC_IsolationTurret.GetCompartmentSlot(), true, -1, ECloseDoorAfterActions.INVALID, false);
		return "DISPATCHED compartment entry; inspect later";
	}

	//------------------------------------------------------------------------------------------------
	string TurretInput(string action)
	{
		if (!m_GC_IsolationTurret || !GetGame().GetPlayerController())
			return "UNAVAILABLE fixture turret/player";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE character access";
		bool occupied = character.GetCompartmentAccessComponent().GetCompartment() == m_GC_IsolationTurret.GetCompartmentSlot();
		BaseWeaponManagerComponent weapons = m_GC_IsolationTurret.GetWeaponManager();
		if (!weapons || !weapons.GetCurrentWeapon() || !weapons.GetCurrentWeapon().GetCurrentMagazine())
			return "UNAVAILABLE weapon/magazine";
		if (action == "fire" && occupied)
			m_GC_IsolationTurret.GC_IsolationTurretPulse();
		else if (action != "snapshot")
			return "REJECTED action or unoccupied turret";
		return string.Format("occupied=%1 ammo=%2", occupied, weapons.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount());
	}
}
#endif
