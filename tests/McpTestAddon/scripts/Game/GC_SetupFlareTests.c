#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSetupFlare(string action)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		return SCR_CharacterControllerComponent.Cast(character.GetCharacterController()).GC_TestFlare(action);
	}
}
#endif
