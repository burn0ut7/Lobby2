#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSpectatorHotkey(string action, float value)
	{
		if (action != "GC_ToggleSpectatorProjectileTracers" && action != "GC_ToggleSpectatorKilledFrom")
			return "Invalid test action";
		InputManager input = GetGame().GetInputManager();
		if (value == 0 || value == 1)
			input.SetActionValue(action, value);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		return string.Format("context=%1 tracers=%2 killedFrom=%3", input.IsContextActive("GC_Lobby_Spectator"), controller.AreSpectatorProjectileTracersEnabled(), controller.IsSpectatorKilledFromEnabled());
	}
}
#endif
