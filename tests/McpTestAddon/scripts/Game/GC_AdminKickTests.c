#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestKickRequest(int targetId, string reason)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!Replication.IsServer() || !controller || targetId == controller.GetPlayerId())
			return "UNAVAILABLE authority, local controller or remote target";
		UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(targetId);
		controller.RequestAdminPlayerAction("player-kick", targetId, identity, reason);
		return "DISPATCHED production request; inspect connection and audit separately";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestKickSnapshot(int targetId)
	{
		if (!Replication.IsServer())
			return "UNAVAILABLE authority";
		PlayerManager players = GetGame().GetPlayerManager();
		return string.Format("player=%1 connected=%2 controller=%3 slot=%4 voice=%5", targetId,
			players.IsPlayerConnected(targetId), players.GetPlayerController(targetId) != null,
			GetSlotByPlayerId(targetId) != null, GetPlayerVoiceAssignment(targetId) != null);
	}
}
#endif
