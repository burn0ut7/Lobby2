//! Explicit isolated-peer setup through native session-admin roles, never a replacement policy.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected GC_LobbyPlayerController m_GC_RoleTestPeer;

	//------------------------------------------------------------------------------------------------
	//! Only changes an ordinary remote peer, and only revokes the role this fixture gave.
	//! Invoke revoke before ending the test. No automatic grant or password emulation.
	string GC_TestPeerAdminRole(int playerId, bool enabled)
	{
		PlayerController host = GetGame().GetPlayerController();
		PlayerManager players = GetGame().GetPlayerManager();
		GC_LobbyPlayerController peer = GC_LobbyPlayerController.Cast(players.GetPlayerController(playerId));
		if (!Replication.IsServer() || !host || !SCR_Global.IsAdmin(host.GetPlayerId())
			|| !peer || peer == host || !players.IsPlayerConnected(playerId))
			return "UNAVAILABLE admin authority/remote peer";

		bool changed;
		if (enabled)
		{
			if (m_GC_RoleTestPeer || SCR_Global.IsAdmin(playerId))
				return "REFUSED existing role or active role fixture";
			changed = players.GivePlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
			if (changed)
				m_GC_RoleTestPeer = peer;
		}
		else
		{
			if (m_GC_RoleTestPeer != peer)
				return "REFUSED peer not owned by this role fixture";
			changed = players.ClearPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
			if (changed)
				m_GC_RoleTestPeer = null;
		}
		string outcome = string.Format("changed=%1 queriedAdmin=%2 roles=%3", changed,
			SCR_Global.IsAdmin(playerId), players.GetPlayerRoles(playerId));
		GC_AdminActionLogger.Log(host.GetPlayerId(), "test-native-session-admin-role",
			string.Format("player=%1 enabled=%2", playerId, enabled), outcome);
		return outcome;
	}
}
#endif
