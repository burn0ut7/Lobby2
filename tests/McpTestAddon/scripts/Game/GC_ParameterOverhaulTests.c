//! Optional explicit fixtures. Production Lobby2 never loads or runs these.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestParameterState()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		string result = string.Format("phase=%1 remaining=%2 locked=%3 admin=%4 revision=%5", GetPhaseName(GetLobbyPhase()), GC_GetParameterLockRemaining(), GC_AreParametersLocked(), SCR_Global.IsAdmin(playerId), GetScenarioParameterRevision());
		foreach (int index, GC_LobbyParameterDefinition definition : GetScenarioParameterDefinitions())
			result += string.Format("\nindex=%1 side=%2 editable=%3 value=%4", index, definition.GetOwnerFactionKey(), CanModifyScenarioParameter(playerId, index), GetScenarioParameterCatalog().FindValue(index).GetNormalizedValue());
		foreach (Faction faction : GetRegisteredFactions())
			result += string.Format("\nside=%1 commander=%2 tickets=%3", faction.GetFactionKey(), GetFactionCommanderPlayerId(faction.GetFactionKey()), GC_GetFactionRespawnTickets(faction.GetFactionKey()));
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Requester -2 uses the real local owning-controller RPC; -1 tests unresolved identity.
	string GC_TestParameterRequest(int index, int number, string text, int requester, int revision)
	{
		array<ref GC_LobbyParameterValue> values = {};
		foreach (GC_LobbyParameterValue current : GetScenarioParameterValues())
		{
			GC_LobbyParameterValue copy = new GC_LobbyParameterValue();
			copy.CopyFrom(current);
			if (copy.GetDefinitionIndex() == index)
			{
				if (copy.GetType() == GC_ELobbyParameterType.INTEGER)
					copy.SetInteger(index, number);
				else if (copy.GetType() == GC_ELobbyParameterType.STRING)
					copy.SetString(index, text);
				else if (copy.GetType() == GC_ELobbyParameterType.SELECTION)
					copy.SetSelection(index, text);
			}
			values.Insert(copy);
		}
		if (revision < 0)
			revision = GetScenarioParameterRevision();
		if (requester == -2)
		{
			GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).RequestApplyScenarioParameters(revision, values);
			return "DISPATCHED owning-controller request; read later state";
		}
		int before = GetScenarioParameterRevision();
		bool accepted = TryApplyScenarioParameters(requester, revision, values);
		return string.Format("accepted=%1 before=%2 after=%3", accepted, before, GetScenarioParameterRevision());
	}

	//------------------------------------------------------------------------------------------------
	//! Runtime-only authored-setting fixture; does not advance the phase clock.
	string GC_TestParameterWindow(int seconds)
	{
		if (!Replication.IsServer() || GetLobbyPhase() != SCR_EGameModeState.PREGAME || seconds < 30 || seconds > 3600)
			return "REJECTED fixture";
		m_iParameterLockSeconds = seconds;
		return "FIXTURE briefing window=" + seconds.ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestParameterCommander(string factionKey)
	{
		GC_Slot slot = GetFactionCommanderSlot(factionKey);
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		if (!slot)
			return "UNAVAILABLE command slot";
		return ExecuteAdminAssignSlot(playerId, playerId, GetPlayerVoiceIdentity(playerId), slot.GetID(), slot.GetUUID());
	}
}
#endif
