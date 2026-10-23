//! Explicit optional fixture for comparing stock firing across loaded addon sets.
//! No Lobby2 dependencies, automatic input, or direct weapon Fire calls.
#ifdef WORKBENCH
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_IsolationFireRemaining;
	protected bool m_bGC_IsolationFireRelease;

	//------------------------------------------------------------------------------------------------
	void GC_IsolationFireInput()
	{
		m_fGC_IsolationFireRemaining = 1;
		m_bGC_IsolationFireRelease = true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (player && owner.GetName() == "IsolationCharacter" && GetGame().GetPlayerController()
			&& GetGame().GetPlayerController().GetControlledEntity() == owner)
		{
			if (m_fGC_IsolationFireRemaining > 0)
			{
				am.SetActionValue("CharacterFire", 1);
				m_fGC_IsolationFireRemaining -= dt;
			}
			else if (m_bGC_IsolationFireRelease)
			{
				am.SetActionValue("CharacterFire", 0);
				m_bGC_IsolationFireRelease = false;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string WeaponInput(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "unavailable-player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || character.GetName() != "IsolationCharacter")
			return "unavailable-fixture-character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller || !controller.GetWeaponManagerComponent())
			return "unavailable-weapon-manager";
		BaseWeaponComponent weapon = controller.GetWeaponManagerComponent().GetCurrentWeapon();
		if (!weapon || !weapon.GetCurrentMagazine())
			return "unavailable-weapon-magazine";
		if (action == "raise")
			controller.SetWeaponRaised(true);
		else if (action == "fire")
			controller.GC_IsolationFireInput();
		else if (action != "snapshot")
			return "rejected-action";
		return string.Format("ammo=%1 raised=%2 canFire=%3", weapon.GetCurrentMagazine().GetAmmoCount(), controller.IsWeaponRaised(), controller.CanFire());
	}
}
#endif
