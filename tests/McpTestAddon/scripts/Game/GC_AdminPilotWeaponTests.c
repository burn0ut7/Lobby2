//! Optional pilot input before compartment processing; never calls native weapon firing.
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_TestPilotPulse;
	protected bool m_bGC_TestPilotRelease;
	protected int m_iGC_TestPilotFrames;
	protected int m_iGC_TestPilotActiveFrames;
	protected int m_iGC_TestPilotTriggeredFrames;
	protected int m_iGC_TestPilotSurvivingFrames;

	//------------------------------------------------------------------------------------------------
	bool GC_TestPilotPulse()
	{
		if (m_fGC_TestPilotPulse > 0 || m_bGC_TestPilotRelease)
			return false;
		m_fGC_TestPilotPulse = 1;
		m_bGC_TestPilotRelease = true;
		m_iGC_TestPilotFrames = 0;
		m_iGC_TestPilotActiveFrames = 0;
		m_iGC_TestPilotTriggeredFrames = 0;
		m_iGC_TestPilotSurvivingFrames = 0;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPilotPulseState()
	{
		return string.Format("TurretFire pending=%1 frames=%2 active=%3 triggered=%4 afterInherited=%5",
			m_fGC_TestPilotPulse > 0 || m_bGC_TestPilotRelease, m_iGC_TestPilotFrames,
			m_iGC_TestPilotActiveFrames, m_iGC_TestPilotTriggeredFrames, m_iGC_TestPilotSurvivingFrames);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		bool observing = player && am && m_fGC_TestPilotPulse > 0;
		if (player && am && (m_fGC_TestPilotPulse > 0 || m_bGC_TestPilotRelease))
		{
			if (m_fGC_TestPilotPulse > 0)
			{
				am.SetActionValue("TurretFire", 1);
				m_fGC_TestPilotPulse -= dt;
				m_iGC_TestPilotFrames++;
				if (am.IsActionActive("TurretFire"))
					m_iGC_TestPilotActiveFrames++;
				if (am.GetActionTriggered("TurretFire"))
					m_iGC_TestPilotTriggeredFrames++;
			}
			else
			{
				am.SetActionValue("TurretFire", 0);
				m_bGC_TestPilotRelease = false;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
		if (observing && am.GetActionTriggered("TurretFire"))
			m_iGC_TestPilotSurvivingFrames++;
	}
}

//! Optional stock-aircraft setup, with read-only loaded-rocket observations.
modded class GC_LobbyFreezeComponent
{
	protected IEntity m_GC_TestPilotAircraft;

	//------------------------------------------------------------------------------------------------
	string GC_TestPilotSetup(int playerId = 0)
	{
		if (!Replication.IsServer() || GetInstance() != this || m_GC_TestPilotAircraft)
			return "UNAVAILABLE active authority without existing aircraft required";
		PlayerController player = GetGame().GetPlayerController();
		if (playerId > 0)
			player = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!player)
			return "UNAVAILABLE local player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE controlled character";
		Resource prefab = Resource.Load("{3C6B3ED0C3AC30D5}Prefabs/Vehicles/Helicopters/Mi8MT/Mi8MT_armed_gunship_HE.et");
		if (!prefab || !prefab.IsValid())
			return "UNAVAILABLE stock gunship prefab";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = character.GetOrigin() + "20 0 0";
		m_GC_TestPilotAircraft = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!m_GC_TestPilotAircraft)
			return "FAILED aircraft spawn";
		BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(
			m_GC_TestPilotAircraft.FindComponent(BaseCompartmentManagerComponent));
		if (!compartments)
			return "FAILED aircraft compartment manager; fixture retained";
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		BaseCompartmentSlot pilot;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (slot.GetCompartmentName() == "PilotCompartment")
				pilot = slot;
		}
		if (!pilot)
			return string.Format("FAILED named pilot compartment; slots=%1 fixture retained", slots.Count());
		character.GetCompartmentAccessComponent().GetInVehicle(m_GC_TestPilotAircraft,
			pilot, true, -1, ECloseDoorAfterActions.INVALID, false);
		return "DISPATCHED native pilot entry; inspect occupied compartment";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPilotWeapon(string action, int playerId = 0)
	{
		if (GetInstance() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE active local player";
		PlayerController player = GetGame().GetPlayerController();
		if (playerId > 0)
		{
			if (!Replication.IsServer() || action != "snapshot")
				return "REJECTED remote observation requires authority and snapshot";
			player = GetGame().GetPlayerManager().GetPlayerController(playerId);
		}
		if (!player)
			return "UNAVAILABLE player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || !character.GetCompartmentAccessComponent())
			return "UNAVAILABLE character";
		BaseCompartmentSlot compartment = character.GetCompartmentAccessComponent().GetCompartment();
		if (!compartment)
			return "UNAVAILABLE occupied compartment";
		TurretControllerComponent turret = compartment.GetAttachedTurret();
		if (!turret || !turret.GetWeaponManager())
			return string.Format("UNAVAILABLE pilot attached turret; compartment=%1 autoConnect=%2 turret=%3",
				compartment.GetCompartmentName(), compartment.GetAutoConnectTurret(), turret);
		SCR_CharacterControllerComponent input = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!input)
			return "UNAVAILABLE character input controller";
		if (action == "fire")
		{
			if (!input.GC_TestPilotPulse())
				return "REJECTED pulse pending";
		}
		else if (action != "snapshot")
			return "REJECTED unsupported action";

		array<IEntity> weapons = {};
		turret.GetWeaponManager().GetWeaponsList(weapons);
		int loaded;
		int barrels;
		int rocketPods;
		foreach (IEntity entity : weapons)
		{
			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(entity.FindComponent(BaseWeaponComponent));
			if (!weapon)
				continue;
			RocketEjectorMuzzleComponent muzzle = RocketEjectorMuzzleComponent.Cast(weapon.GetCurrentMuzzle());
			if (!muzzle)
				continue;
			rocketPods++;
			array<IEntity> projectiles = {};
			barrels += muzzle.GetLoadedEntities(projectiles);
			foreach (IEntity projectile : projectiles)
			{
				if (projectile)
					loaded++;
			}
		}
		string result = string.Format("freeze=%1 occupied=1 overridden=%2 weapons=%3 pods=%4 barrels=%5 loaded=%6 input=%7",
			IsFreezeActive(), turret.GetIsOverridden(), weapons.Count(), rocketPods, barrels, loaded,
			input.GC_TestPilotPulseState());
		return result + string.Format(" seat=%1 controller=%2 turret=%3", compartment.GetCompartmentName(),
			compartment.GetController(), turret);
	}
}
