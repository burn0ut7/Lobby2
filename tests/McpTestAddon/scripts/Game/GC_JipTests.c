//! Explicit fixtures only; production handlers still own admission and movement.
modded class GC_LobbyGameMode
{
	void GC_TestJipConfig(int policy, int cutoff, float elapsed)
	{
		m_eJipMode = policy;
		m_iJipDenySeconds = cutoff;
		m_fTimeElapsed = elapsed;
	}

	string GC_TestJipFactions()
	{
		array<string> previous = m_aJipFactions;
		m_aJipFactions = {};
		bool emptyAllows = IsJipFactionAllowed("US") && IsJipFactionAllowed("USSR");
		m_aJipFactions = {"US"};
		bool filtered = IsJipFactionAllowed("US") && !IsJipFactionAllowed("USSR");
		m_aJipFactions = previous;
		return string.Format("emptyAllowsAll=%1 filterMatchesOnly=%2", emptyAllows, filtered);
	}
}

modded class GC_JipSystem
{
	string GC_TestAdmission(int playerId, float joinedAt, bool reset)
	{
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (identity.IsEmpty())
			return "UNAVAILABLE authenticated identity";
		if (reset)
			m_mPlayers.Remove(identity);
		m_mConnections.Set(playerId, joinedAt);
		GC_JipPlayer record = Register(playerId);
		return string.Format("eligible=%1 offered=%2 consumed=%3", record.m_bEligible, record.m_bOffered, record.m_bConsumed);
	}

	string GC_TestSnapshot(int playerId)
	{
		GC_JipPlayer record = m_mPlayers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId));
		if (!record)
			return "record=absent";
		return string.Format("eligible=%1 offered=%2 consumed=%3 character=%4 elapsed=%5 pending=%6", record.m_bEligible,
			record.m_bOffered, record.m_bConsumed, record.m_Character, Elapsed(), record.m_bPending);
	}

	void GC_TestTick()
	{
		Tick();
	}
}

modded class GC_LobbyManager
{
	string GC_TestJip(string action, int value)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!mode || !controller || !Replication.IsServer() || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE local authenticated admin authority";
		GC_JipSystem jip = mode.GetJip();
		int playerId = controller.GetPlayerId();
		if (action == "advance")
			controller.RequestLobbyPhaseAdvance(GC_LobbyPhase.GetNext(mode.GetState()));
		else if (action == "factions")
			return mode.GC_TestJipFactions();
		else if (action == "transport")
		{
			IEntity character = controller.GetControlledEntity();
			vector destination;
			if (mode.GetState() != SCR_EGameModeState.GAME || !character || character.GetRootParent() != character
				|| !SCR_WorldTools.FindEmptyTerrainPosition(destination, character.GetOrigin() + "20 0 0", 5))
				return "UNAVAILABLE live on-foot character/landing";
			controller.GC_DispatchJip(Replication.FindItemId(character), destination, ECharacterStance.STAND);
			return "DISPATCHED transport-only fixture destination=" + destination.ToString();
		}
		else if (action == "slot")
		{
			GC_Slot slot = GetSlot(value);
			if (!slot)
				return "UNAVAILABLE slot";
			controller.RequestAdminAssignSlot(playerId, GetPlayerVoiceIdentity(playerId), value, slot.GetUUID());
		}
		else if (action == "policy")
		{
			if (mode.GetState() != SCR_EGameModeState.GAME)
				return "UNAVAILABLE GAME";
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 0, 200);
			string early = jip.GC_TestAdmission(playerId, 90, true);
			string earlyAgain = jip.GC_TestAdmission(playerId, 200, false);
			string edge = jip.GC_TestAdmission(playerId, 120, true);
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 200, 200);
			string cutoff = jip.GC_TestAdmission(playerId, 120, true);
			mode.GC_TestJipConfig(GC_EJipMode.DISABLED, 0, 200);
			string disabled = jip.GC_TestAdmission(playerId, 120, true);
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 0, 200);
			jip.GC_TestAdmission(playerId, 120, true);
			return string.Format("90s: %1; same account later: %2; 120s: %3; exact cutoff: %4; disabled: %5", early, earlyAgain, edge, cutoff, disabled);
		}
		else if (action == "offer")
		{
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 0, 200);
			jip.GC_TestAdmission(playerId, 120, true);
			jip.GC_TestTick();
		}
		else if (action == "peer-offer")
		{
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 0, 200);
			string result = jip.GC_TestAdmission(value, 120, true);
			jip.GC_TestTick();
			return result + " " + jip.GC_TestSnapshot(value);
		}
		else if (action == "peer-snapshot")
			return jip.GC_TestSnapshot(value);
		else if (action == "auto")
		{
			mode.GC_TestJipConfig(GC_EJipMode.AUTO, 0, 200);
			jip.GC_TestAdmission(playerId, 120, true);
			jip.GC_TestTick();
		}
		else if (action == "press")
			controller.GC_OnJipPressed();
		else if (action == "input-down")
			GetGame().GetInputManager().SetActionValue("GC_Jip", 1);
		else if (action == "input-up")
			GetGame().GetInputManager().SetActionValue("GC_Jip", 0);
		else if (action == "request")
			controller.GC_RequestJip();
		else if (action == "cutoff")
			mode.GC_TestJipConfig(GC_EJipMode.MANUAL, 200, 200);
		else if (action != "snapshot")
			return "UNAVAILABLE action";
		return string.Format("phase=%1 player=%2 group=%3 %4 message=%5", mode.GetState(), playerId, GetPlayerGroup(playerId), jip.GC_TestSnapshot(playerId), controller.GC_TestJipMessage());
	}
}

modded class GC_LobbyPlayerController
{
	string GC_TestJipMessage()
	{
		return m_sGC_JipMessage;
	}
}
