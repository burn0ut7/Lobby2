#ifdef WORKBENCH
modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string CheckAdminMenuInactive()
	{
		if (GC_LobbyGameMode.GetGameMode() || GC_LobbyManager.GetManager())
			return "UNAVAILABLE unrelated mode required";
		MenuBase before = GetGame().GetMenuManager().GetTopMenu();
		GC_AdminToolsComponent.Open();
		MenuBase after = GetGame().GetMenuManager().GetTopMenu();
		return string.Format("menuUnchanged=%1 adminMenu=%2 adminInput=%3", before == after,
			GC_AdminToolsMenu.Cast(after) != null, GetGame().GetInputManager().IsContextActive("GC_AdminTools"));
	}

	//------------------------------------------------------------------------------------------------
	string CheckInactiveMode()
	{
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!mode || GC_LobbyGameMode.GetGameMode() || GC_LobbyManager.GetManager())
			return "rejected: requires an unrelated mode";
		SCR_EGameModeState before = mode.GetState();
		bool accepted = mode.GC_SetLobbyState(SCR_EGameModeState.BRIEFING);
		return string.Format("lobbyMode=0 lobbyManager=0 lobbyTransitionAccepted=%1 stateUnchanged=%2", accepted, mode.GetState() == before);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPlayableSpawnGuard()
	{
		foreach (SCR_AIGroup group : GetRegisteredGroups())
		{
			array<AIAgent> members = {};
			group.GetAgents(members);
			if (!group.IsPlayable() || !members.IsEmpty())
				continue;
			group.SpawnUnits();
			group.GetAgents(members);
			return string.Format("activeLobby=%1 agentsAfterSpawn=%2", GetManager() == this, members.Count());
		}
		return "rejected: no empty playable group";
	}

	//------------------------------------------------------------------------------------------------
	//! Terminal check: end this Play session after calling. Uses real teardown.
	string GC_TestShutdownIsolation()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || GetManager() != this)
			return "rejected: no active local lobby";
		Shutdown();
		return string.Format("managerReleased=%1 %2", GetManager() == null, controller.GC_TestReleasedState());
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestReleasedState()
	{
		SCR_VONController voice = SCR_VONController.Cast(FindComponent(SCR_VONController));
		bool voiceDisabled;
		if (voice)
			voiceDisabled = voice.IsVONDisabled();
		return string.Format("inputBound=%1 voiceBound=%2 voiceOwned=%3 nativeVoiceDisabled=%4 cameraPresent=%5 publishing=%6", m_LobbyInputManager != null, m_LobbyObservedVoice != null, m_bLobbyDisabledNativeVoice, voiceDisabled, m_SpectatorCamera != null, m_bPublishNetworkCamera);
	}
}
#endif
