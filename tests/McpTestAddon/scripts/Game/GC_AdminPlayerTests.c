#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminFollowReject()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !Replication.IsServer())
			return "UNAVAILABLE authority/controller";
		int playerId = controller.GetPlayerId();
		UUID identity = GetPlayerVoiceIdentity(playerId);
		RplId entityId;
		string denied = ValidateAdminFollowPlayer(-1, playerId, identity, entityId);
		if (denied != "rejected-not-logged-in" || entityId.IsValid())
			return "FAIL invalid requester";
		string stale = ValidateAdminFollowPlayer(playerId, playerId, "00000000-0000-0000-0000-000000000001", entityId);
		if (stale != "rejected-stale-player" || entityId.IsValid())
			return "FAIL stale identity";
		string self = ValidateAdminFollowPlayer(playerId, playerId, identity, entityId);
		if (self != "unavailable-follow-camera" || entityId.IsValid())
			return "FAIL self target";
		return "PASS unknown requester, stale identity and self follow rejected without camera grant";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminPlayerReject()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller)
			return "UNAVAILABLE authority/controller";
		int playerId = controller.GetPlayerId();
		UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string denied = ExecuteAdminPlayerAction(-1, "player-kick", playerId, identity, "fixture", 0, 0);
		string stale = ExecuteAdminPlayerAction(playerId, "player-kick", playerId, "00000000-0000-0000-0000-000000000001", "fixture", 0, 0);
		string reason = ExecuteAdminPlayerAction(playerId, "player-kick", playerId, identity, "", 0, 0);
		string duration = ExecuteAdminPlayerAction(playerId, "player-ban", playerId, identity, "fixture", -1, 0);
		string page = ExecuteAdminPlayerAction(playerId, "ban-list", -1, "", "", 0, -1);
		string invalidIdentity = ExecuteAdminPlayerAction(playerId, "player-unban", -1, "bad", "", 0, 0);
		if (denied != "rejected-not-logged-in" || stale != "rejected-stale-player" || reason != "rejected-admin-reason" || duration != "rejected-ban-duration" || page != "rejected-ban-page" || invalidIdentity != "rejected-account-identity")
			return string.Format("FAIL denied=%1 stale=%2 reason=%3 duration=%4 page=%5 identity=%6", denied, stale, reason, duration, page, invalidIdentity);
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return "FAIL rejection disconnected player";
		return "PASS invalid requester, stale account, missing reason, negative duration, negative page and invalid account rejected; player remains connected";
	}
}
#endif
