#ifdef WORKBENCH
modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPlayerJump(string action)
	{
		vector position;
		if (action == "resolve" || action == "invalid")
		{
			int target = GetPlayerId();
			if (action == "invalid")
				target = 999999;
			string error = GC_ResolveSpectatorPlayerPosition(target, position);
			return "error=" + error + " position=" + position.ToString();
		}
		if (!m_SpectatorCamera)
			return "UNAVAILABLE camera";
		if (action == "away")
		{
			m_SpectatorCamera.SetFreeMode();
			m_SpectatorCamera.SetOrigin(m_SpectatorCamera.GetOrigin() + "1500 100 1500");
		}
		if (action == "request")
			GC_RequestSpectatorPlayerPosition(GetPlayerId());
		if (action == "stale")
			GC_RpcDoSpectatorPlayerPosition(m_iGC_SpectatorJumpRequest - 1, "1 999 1", "");
		string resolved = GC_ResolveSpectatorPlayerPosition(GetPlayerId(), position);
		return string.Format("eligible=%1 request=%2 mode=%3 camera=%4 target=%5 distance=%6 error=%7", GC_CanJumpSpectatorCamera(), m_iGC_SpectatorJumpRequest,
			m_SpectatorCamera.GetMode(), m_SpectatorCamera.GetOrigin(), position, vector.Distance(m_SpectatorCamera.GetOrigin(), position), resolved);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSpectatorPlayerJump(string action)
	{
		if (GetManager() != this || RplSession.Mode() == RplMode.Dedicated)
			return "UNAVAILABLE disposable embedded test";
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (action == "die")
		{
			if (!Replication.IsServer() || GetLobbyPhase() != SCR_EGameModeState.GAME)
				return "UNAVAILABLE game phase";
			IEntity character = controller.GetControlledEntity();
			SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
			if (!damage || character.FindComponent(GC_LobbySpectatorBodyComponent))
				return "UNAVAILABLE playing character";
			damage.SetHealthScaled(0);
			return "Native death dispatched; observe spectator state";
		}
		return controller.GC_TestPlayerJump(action);
	}
}
#endif
