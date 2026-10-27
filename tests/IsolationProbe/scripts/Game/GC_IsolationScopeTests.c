//! Explicit stock PiP comparison fixture, usable with or without Lobby2 loaded.
#ifdef WORKBENCH
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_IsolationADSRemaining;
	protected bool m_bGC_IsolationADSRelease;

	//------------------------------------------------------------------------------------------------
	void GC_IsolationHoldADS()
	{
		m_fGC_IsolationADSRemaining = 20;
		m_bGC_IsolationADSRelease = true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && owner.GetName() == "IsolationScopeCharacter" && GetGame().GetPlayerController()
			&& GetGame().GetPlayerController().GetControlledEntity() == owner)
		{
			if (m_fGC_IsolationADSRemaining > 0)
			{
				am.SetActionValue("CharacterWeaponADSHold", 1);
				m_fGC_IsolationADSRemaining -= dt;
			}
			else if (m_bGC_IsolationADSRelease)
			{
				am.SetActionValue("CharacterWeaponADSHold", 0);
				m_bGC_IsolationADSRelease = false;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class SCR_2DPIPSightsComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_IsolationScopeSnapshot()
	{
		float pipFOV = -1;
		if (m_PIPCamera)
			pipFOV = m_PIPCamera.GetVerticalFOV();
		return string.Format("enabled=%1 pipFOV=%2 reticle=%3 edge=%4 zeroPitch=%5", IsPIPEnabled(),
			pipFOV, m_fReticleScale, m_fEdgeScale, m_fCurrentCameraPitch);
	}
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string ScopeInput(string action)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!Replication.IsServer() || !player || GetOwner().GetName() != "IsolationProbe")
			return "UNAVAILABLE isolation authority/player";
		if (action == "spawn")
		{
			if (GetGame().GetWorld().FindEntityByName("IsolationScopeCharacter"))
				return "REJECTED fixture already exists";
			ResourceName resourceName = Workbench.GetResourceName("Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_SR.et");
			Resource prefab = Resource.Load(resourceName);
			if (!prefab || !prefab.IsValid())
				return "UNAVAILABLE Senior Rifleman prefab";
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = "110 1 115";
			IEntity spawned = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
			if (!spawned)
				return "FAILED spawn";
			spawned.SetName("IsolationScopeCharacter");
			player.SetInitialMainEntity(spawned);
			return string.Format("possessed=%1", player.GetControlledEntity() == spawned);
		}
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || character.GetName() != "IsolationScopeCharacter")
			return "UNAVAILABLE scope fixture character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		BaseWeaponManagerComponent weapons = controller.GetWeaponManagerComponent();
		if (!weapons || !weapons.GetCurrentWeapon())
			return "UNAVAILABLE weapon";
		if (action == "ads")
		{
			controller.SetWeaponRaised(true);
			controller.GC_IsolationHoldADS();
		}
		else if (action == "zeroing")
			controller.SetSightsRange(3);
		else if (action != "snapshot")
			return "REJECTED action";
		SCR_2DPIPSightsComponent optic = SCR_2DPIPSightsComponent.Cast(weapons.GetCurrentWeapon().GetSightsAt(weapons.GetCurrentSightsIndex()));
		if (!optic)
			return "UNAVAILABLE PiP optic";
		return optic.GC_IsolationScopeSnapshot();
	}
}
#endif
