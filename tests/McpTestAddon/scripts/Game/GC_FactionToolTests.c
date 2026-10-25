#ifdef WORKBENCH
modded class GC_AdminToolsComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFactionForm(string operation, string value)
	{
		if (operation == "open")
		{
			m_iTool = 0;
			Refresh();
		}
		if (operation == "to")
			m_To.SelectPlayer(value.ToInt());
		if (operation == "faction")
		{
			foreach (SCR_ButtonBaseComponent button, string key : m_mFactionButtons)
			{
				if (key == value)
				{
					OnFactionSelected(button);
					break;
				}
			}
		}
		return string.Format("tool=%1 faction=%2 confirmed=%3 pending=%4 status=%5", m_iTool, m_sTargetFaction,
			m_bConfirmFactionTeleport, m_iPending, m_Status.GetText());
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Revoke existing native roles and attempt exact restoration after rejection checks.
	//! End the disposable Play session when native role restoration is refused (restored=0).
	string GC_TestFactionLoggedOut(string factionKey)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		PlayerController host = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !host || !SCR_Global.IsAdmin(host.GetPlayerId()))
			return "UNAVAILABLE authenticated local authority";
		int playerId = host.GetPlayerId();
		bool administrator = players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		bool sessionAdministrator = players.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		string before = GC_TestAdminHealthSnapshot(playerId);
		if (administrator)
			players.ClearPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (sessionAdministrator)
			players.ClearPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		bool loggedOut = !SCR_Global.IsAdmin(playerId);
		string heal = "UNAVAILABLE native role revocation";
		string teleport = heal;
		if (loggedOut)
		{
			heal = GC_TestFactionReject(playerId, factionKey, playerId, false, false);
			teleport = GC_TestFactionReject(playerId, factionKey, playerId, false, true);
		}
		string after = GC_TestAdminHealthSnapshot(playerId);
		if (administrator)
			players.GivePlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (sessionAdministrator)
			players.GivePlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		return string.Format("connected=%1 loggedOut=%2 heal=[%3] teleport=[%4] unchanged=%5 restored=%6",
			players.IsPlayerConnected(playerId), loggedOut, heal, teleport, before == after, SCR_Global.IsAdmin(playerId));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFactionTool(string operation, string value)
	{
		if (operation == "open")
			GC_AdminToolsComponent.Open();
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		Widget root;
		if (menu)
			root = menu.GetRootWidget();
		GC_AdminToolsComponent panel;
		if (root)
			panel = GC_AdminToolsComponent.Cast(root.FindHandler(GC_AdminToolsComponent));
		if (!panel)
			return "UNAVAILABLE tools";
		return panel.GC_TestFactionForm(operation, value);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFactionReject(int requesterId, string factionKey, int destinationId, bool stale, bool teleport)
	{
		array<int> players;
		array<string> identities;
		GC_GetFactionPlayers(factionKey, players, identities);
		if (stale && !identities.IsEmpty())
			identities[0] = "stale-test-identity";
		if (teleport)
		{
			string destinationIdentity = GetPlayerVoiceIdentity(destinationId);
			if (stale)
				destinationIdentity = "stale-test-identity";
			return GC_ExecuteFactionTeleport(requesterId, factionKey, destinationId, destinationIdentity, 900);
		}
		// Faction membership is resolved at execution; stale identity checks belong to selected-player requests.
		if (stale)
			return GC_ExecuteAdminHeal(requesterId, GC_EAdminHealScope.SELECTED, players, identities, 900, 15, "");
		return GC_ExecuteAdminHeal(requesterId, GC_EAdminHealScope.FACTION, {}, {}, 900, 15, factionKey);
	}
}
#endif
