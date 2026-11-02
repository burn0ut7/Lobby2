modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		if (Replication.IsServer())
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int playerId : players)
			{
				GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				if (controller)
					controller.GC_ReleaseClothingPermission(this);
			}
		}
		super.Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteForcedClothing(int requesterId, GC_EAdminHealScope scope, array<int> selectedIds, array<string> selectedIdentities, int requestId, int radiusMeters, string factionKey, bool allowed)
	{
		string action = "clothing-lock";
		if (allowed)
			action = "clothing-allow";
		string context = string.Format("scope=%1 radius=%2 request=%3 faction=%4", scope, radiusMeters, requestId, factionKey);
		if (!AuthorizeAdminAction(requesterId, action, context))
			return "Admin login required.";
		array<int> targets;
		ChimeraCharacter origin;
		string error = GC_LobbyManager.Cast(this).GC_ResolveAdminToolTargets(requesterId, scope, selectedIds, selectedIdentities, radiusMeters, factionKey, action, context, targets, origin);
		if (!error.IsEmpty())
			return error;

		int applied;
		int skipped;
		foreach (int targetId : targets)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(targetId));
			IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId);
			if (origin && character && vector.DistanceSq(origin.GetOrigin(), character.GetOrigin()) > radiusMeters * radiusMeters)
				continue;
			if (!controller || (origin && (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))))
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, action, context, "skipped-unavailable", targetId, GetPlayerVoiceIdentity(targetId));
				continue;
			}
			if (!controller.GC_SetClothingChangesAllowed(requesterId, allowed))
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, action, context, "rejected-admin-session", targetId, GetPlayerVoiceIdentity(targetId));
				continue;
			}
			applied++;
			GC_AdminActionLogger.Log(requesterId, action, context, "applied", targetId, GetPlayerVoiceIdentity(targetId));
		}
		string state = "locked";
		if (allowed)
			state = "allowed";
		string result = string.Format("Clothing changes %1 for %2 player(s). Skipped %3.", state, applied, skipped);
		GC_AdminActionLogger.Log(requesterId, action, context, result);
		return result;
	}
}
