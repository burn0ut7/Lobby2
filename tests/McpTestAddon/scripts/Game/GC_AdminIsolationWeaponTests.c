//! Loads after Lobby2, so injected input reaches the production freeze hook.
#ifdef WORKBENCH
modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	//! Outer test-addon input reaches the production turret hook before stock processing.
	string GC_TestLoadedTurretFire()
	{
		if (!m_GC_IsolationTurret || !GetGame().GetPlayerController())
			return "UNAVAILABLE fixture turret/player";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character || !character.GetCompartmentAccessComponent()
			|| character.GetCompartmentAccessComponent().GetCompartment() != m_GC_IsolationTurret.GetCompartmentSlot())
			return "UNAVAILABLE occupied fixture turret";
		if (!m_GC_IsolationTurret.GC_TestTurretPulse())
			return "REJECTED pulse pending";
		return TurretInput("snapshot");
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadedTurretOwnership()
	{
		return string.Format("lobbyManager=%1 freezeComponent=%2", GC_LobbyManager.GetManager() != null,
			GC_LobbyFreezeComponent.GetInstance() != null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLoadedFire()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || !player.GetControlledEntity() || player.GetControlledEntity().GetName() != "IsolationCharacter")
			return "unavailable-fixture-character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(player.GetControlledEntity().FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return "unavailable-controller";
		controller.GC_TestFireInput(1);
		return WeaponInput("snapshot");
	}
}
#endif
