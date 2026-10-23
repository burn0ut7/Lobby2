//! Optional explicit MCP calls; never runs automatically or grants administration.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Arrange native death, reject teleport, then request healing without retaining the deleted body.
	string GC_TestAdminDeadTarget(int targetId, int destinationId)
	{
		if (GetManager() != this || !Replication.IsServer())
			return "UNAVAILABLE authority";
		ChimeraCharacter character = GC_GetAdminToolCharacter(targetId);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!character || !controller || !GC_GetAdminToolCharacter(destinationId))
			return "UNAVAILABLE living target and destination";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		string identity = GetPlayerVoiceIdentity(targetId);
		vector position = character.GetOrigin();
		damage.SetHealthScaled(0);
		bool deadBefore = damage.GetState() == EDamageState.DESTROYED;
		if (!deadBefore)
			return "UNAVAILABLE native death did not complete";

		array<int> players = {targetId};
		array<string> identities = {identity};
		int teleportRequest = controller.ReserveAdminPlayerToolRequest();
		controller.GC_RequestAdminTeleport(targetId, identity, destinationId,
			GetPlayerVoiceIdentity(destinationId), teleportRequest);
		int healRequest = controller.ReserveAdminPlayerToolRequest();
		controller.RequestAdminHeal(GC_EAdminHealScope.SELECTED, players, identities, healRequest);
		return string.Format("deadBefore=%1 healRequest=%2 teleportRequest=%3; inspect audit and later possession",
			deadBefore, healRequest, teleportRequest);
	}

	//------------------------------------------------------------------------------------------------
	//! Arrange a nonfatal health deficit; never used as the healing action under test.
	string GC_TestAdminDamageFixture(int playerId)
	{
		if (GetManager() != this || !Replication.IsServer())
			return "UNAVAILABLE authority";
		ChimeraCharacter character = GC_GetAdminToolCharacter(playerId);
		if (!character)
			return "UNAVAILABLE living character";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		HitZone zone = damage.GetDefaultHitZone();
		if (!zone)
			return "UNAVAILABLE default zone";
		zone.SetHealthScaled(0.6);
		return GC_TestAdminHealthSnapshot(playerId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminHeal(int scope, int selectedPlayerId, string expectedIdentity, int radiusMeters = 15)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE local controller";
		array<int> players = {};
		array<string> identities = {};
		if (selectedPlayerId > 0)
		{
			players.Insert(selectedPlayerId);
			identities.Insert(expectedIdentity);
		}
		int requestId = controller.ReserveAdminPlayerToolRequest();
		controller.RequestAdminHeal(scope, players, identities, requestId, radiusMeters);
		return string.Format("DISPATCHED request=%1; inspect outcome, zones and audit", requestId);
	}

	//------------------------------------------------------------------------------------------------
	//! Places and damages a real target, then sends the production request in the same
	//! tick so simulation cannot move an exact boundary between setup and execution.
	string GC_TestAdminHealAtDistance(int targetId, int distanceMeters, int radiusMeters)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!Replication.IsServer() || GetManager() != this || !controller)
			return "UNAVAILABLE local authority";
		ChimeraCharacter origin = GC_GetAdminToolCharacter(controller.GetPlayerId());
		ChimeraCharacter target = GC_GetAdminToolCharacter(targetId);
		if (!origin || !target || origin == target || distanceMeters < 1 || distanceMeters > 1100)
			return "UNAVAILABLE distinct living players or fixture distance";
		vector position = origin.GetOrigin();
		position[0] = position[0] + distanceMeters;
		target.SetOrigin(position);
		string before = GC_TestAdminDamageFixture(targetId);
		float distance = vector.Distance(origin.GetOrigin(), target.GetOrigin());
		string dispatch = GC_TestAdminHeal(GC_EAdminHealScope.NEARBY, 0, "", radiusMeters);
		return string.Format("distance=%1 radius=%2 before=[%3] %4 after=[%5]", distance, radiusMeters, before, dispatch, GC_TestAdminHealthSnapshot(targetId));
	}



	//------------------------------------------------------------------------------------------------
	string GC_TestAdminHealthSnapshot(int playerId)
	{
		if (GetManager() != this)
			return "UNAVAILABLE manager";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!character)
			return "UNAVAILABLE character";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "UNAVAILABLE damage manager";
		array<HitZone> zones = {};
		damage.GetAllHitZonesInHierarchy(zones);
		int damaged;
		foreach (HitZone zone : zones)
		{
			if (zone && zone.GetDamageState() != EDamageState.UNDAMAGED)
				damaged++;
		}
		return string.Format("player=%1 identity=%2 state=%3 zones=%4 damaged=%5 position=%6 spectator=%7",
			playerId, GetPlayerVoiceIdentity(playerId), damage.GetState(), zones.Count(), damaged,
			character.GetOrigin(), character.FindComponent(GC_LobbySpectatorBodyComponent) != null);
	}
}
