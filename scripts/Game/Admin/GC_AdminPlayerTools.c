enum GC_EAdminHealScope
{
	SELECTED,
	EVERYONE,
	NEARBY,
	FACTION
}

//! Player tools use native character state; no copied health or position ownership.
modded class GC_LobbyManager
{
	static const int GC_MIN_HEAL_RADIUS = 1;
	static const int GC_MAX_HEAL_RADIUS = 1000;
	static const int GC_DEFAULT_HEAL_RADIUS = 15;

	//------------------------------------------------------------------------------------------------
	protected ChimeraCharacter GC_GetAdminToolCharacter(int playerId)
	{
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return null;
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return null;
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage || damage.GetState() == EDamageState.DESTROYED)
			return null;
		return character;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared audience validation for Heal and control Freeze; callers authenticate first.
	protected string GC_ResolveAdminToolTargets(int requesterId, GC_EAdminHealScope scope, array<int> selectedIds, array<string> selectedIdentities, int radiusMeters, string factionKey, string action, string context, out array<int> targets, out ChimeraCharacter origin)
	{
		if (scope < GC_EAdminHealScope.SELECTED || scope > GC_EAdminHealScope.FACTION)
		{
			GC_AdminActionLogger.Log(requesterId, action, context, "rejected-scope");
			return "Invalid player scope.";
		}
		if (scope == GC_EAdminHealScope.NEARBY && (radiusMeters < GC_MIN_HEAL_RADIUS || radiusMeters > GC_MAX_HEAL_RADIUS))
		{
			GC_AdminActionLogger.Log(requesterId, action, context, "rejected-radius");
			return string.Format("Choose a distance from %1 to %2 metres.", GC_MIN_HEAL_RADIUS, GC_MAX_HEAL_RADIUS);
		}

		targets = {};
		if (scope == GC_EAdminHealScope.FACTION)
		{
			array<string> identities;
			if (!GC_GetFactionPlayers(factionKey, targets, identities))
			{
				GC_AdminActionLogger.Log(requesterId, action, context, "rejected-faction");
				return "The selected faction is unavailable.";
			}
			if (targets.IsEmpty())
			{
				GC_AdminActionLogger.Log(requesterId, action, context, "rejected-empty-faction");
				return "No connected players are in that faction.";
			}
		}
		else if (scope == GC_EAdminHealScope.SELECTED)
		{
			if (!selectedIds || !selectedIdentities || selectedIds.IsEmpty() || selectedIds.Count() != selectedIdentities.Count() || selectedIds.Count() > 128)
			{
				GC_AdminActionLogger.Log(requesterId, action, context, "rejected-selection");
				return "Choose at least one player.";
			}
			// Validate the entire selection before changing any player.
			foreach (int index, int playerId : selectedIds)
			{
				if (selectedIdentities[index].IsEmpty() || GetPlayerVoiceIdentity(playerId) != selectedIdentities[index]
					|| !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				{
					GC_AdminActionLogger.Log(requesterId, action, context, "rejected-stale-selection");
					return "A selected player disconnected. Select players again.";
				}
				if (!targets.Contains(playerId))
					targets.Insert(playerId);
			}
		}
		else
			GetGame().GetPlayerManager().GetPlayers(targets);

		origin = null;
		if (scope == GC_EAdminHealScope.NEARBY)
		{
			origin = GC_GetAdminToolCharacter(requesterId);
			if (!origin)
			{
				GC_AdminActionLogger.Log(requesterId, action, context, "rejected-no-admin-character");
				return "Spawn as a player to target nearby players.";
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteAdminHeal(int requesterId, GC_EAdminHealScope scope, array<int> selectedIds, array<string> selectedIdentities, int requestId, int radiusMeters, string factionKey = "")
	{
		string context = string.Format("scope=%1 radius=%2 request=%3 faction=%4", scope, radiusMeters, requestId, factionKey);
		if (!AuthorizeAdminAction(requesterId, "heal", context))
			return "Admin login required.";
		array<int> targets;
		ChimeraCharacter origin;
		string error = GC_ResolveAdminToolTargets(requesterId, scope, selectedIds, selectedIdentities, radiusMeters, factionKey, "heal", context, targets, origin);
		if (!error.IsEmpty())
			return error;

		// Snapshot the centre before respawning or repairing any target.
		vector centre;
		if (origin)
			centre = origin.GetOrigin();
		int healed;
		int respawned;
		int skipped;
		foreach (int targetId : targets)
		{
			ChimeraCharacter character = GC_GetAdminToolCharacter(targetId);
			string targetContext = string.Format("%1 player=%2 identity=%3", context, targetId, GetPlayerVoiceIdentity(targetId));
			GC_Slot slot = GetSlotByPlayerId(targetId);
			if (!character && slot && slot.IsKIA())
			{
				IEntity corpse = slot.GetEntity();
				if (origin && !corpse)
				{
					skipped++;
					GC_AdminActionLogger.Log(requesterId, "heal-respawn", targetContext, "skipped-unknown-position");
					continue;
				}
				if (origin && vector.DistanceSq(centre, corpse.GetOrigin()) > radiusMeters * radiusMeters)
					continue;
				string respawnResult = RespawnAdminSlot(slot, corpse != null);
				if (respawnResult == "applied")
				{
					UpdateSlot(slot);
					respawned++;
				}
				else
					skipped++;
				GC_AdminActionLogger.Log(requesterId, "heal-respawn", targetContext, respawnResult);
				continue;
			}
			if (!character)
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "heal-player", targetContext, "skipped-no-living-character");
				continue;
			}
			if (origin && vector.DistanceSq(centre, character.GetOrigin()) > radiusMeters * radiusMeters)
				continue;
			bool restored = GC_HealAdminCharacter(character);
			if (restored)
			{
				healed++;
				GC_AdminActionLogger.Log(requesterId, "heal-player", targetContext, "applied");
			}
			else
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "heal-player", targetContext, "health-not-fully-restored");
			}
		}
		string outcome = string.Format("Healed %1 player(s). Respawned %2. Skipped %3 unavailable or unhealed player(s).", healed, respawned, skipped);
		if (scope == GC_EAdminHealScope.NEARBY)
		{
			GC_AdminNearbyVehicleRepair repair = new GC_AdminNearbyVehicleRepair();
			string vehicleOutcome = repair.Execute(requesterId, centre, radiusMeters, context);
			outcome = string.Format("Within %1m: %2 %3", radiusMeters, outcome, vehicleOutcome);
		}
		GC_AdminActionLogger.Log(requesterId, "heal", context, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteAdminWarn(int requesterId, int targetId, string targetIdentity, string reason)
	{
		if (!AuthorizeAdminAction(requesterId, "player-warn", targetIdentity))
			return "Admin login required.";
		reason.TrimInPlace();
		string result;
		if (targetIdentity.IsEmpty() || !GetGame().GetPlayerManager().IsPlayerConnected(targetId)
			|| GetPlayerVoiceIdentity(targetId) != targetIdentity)
			result = "Player changed. Select again.";
		else if (reason.IsEmpty() || reason.Length() > 256 || reason.Contains("\n") || reason.Contains("\r")
			|| reason.Contains("<") || reason.Contains(">"))
			result = "Enter a single-line reason (1â€“256 characters).";
		else if (!GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(targetId)))
			result = "Player is unavailable.";
		else
		{
			array<string> recipients = {targetIdentity};
			int sent;
			result = SendAdminMessage(requesterId, GC_EAdminMessageAudience.SELECTED_PLAYERS, reason, recipients, "", sent, true);
			if (sent == 1)
			{
				RecordModerationAction(targetIdentity, "Warning dispatched", reason, 0);
				result = "Warning dispatched.";
			}
		}
		GC_AdminActionLogger.Log(requesterId, "player-warn", reason, result, targetId, targetIdentity);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteAdminUnstuck(int requesterId, int targetId, string targetIdentity, int requestId)
	{
		string context = string.Format("player=%1 identity=%2 request=%3", targetId, targetIdentity, requestId);
		if (!AuthorizeAdminAction(requesterId, "unstuck", context))
			return "Admin login required.";
		ChimeraCharacter character = GC_GetAdminToolCharacter(targetId);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(targetId));
		string error;
		if (targetIdentity.IsEmpty() || GetPlayerVoiceIdentity(targetId) != targetIdentity)
			error = "Player changed. Select again.";
		else if (!character || !controller)
			error = "Player needs a living character.";
		else if (character.GetRootParent() != character)
			error = "Player must leave the vehicle first.";
		vector position;
		if (error.IsEmpty())
		{
			BaseWorld world = character.GetWorld();
			// Use the shipped character-sized clearance search, constrained to a local dry landing.
			if (!SCR_EmptyPositionHelper.TryFindNearbyFloorPositionForEntity(world, character, character.GetOrigin(), position)
				|| !SCR_Global.IsPositionWithinTerrainBounds(position)
				|| vector.DistanceSq(position, character.GetOrigin()) > 100
				|| position[1] < world.GetOceanHeight(position[0], position[2]))
				error = "No clear nearby landing.";
			else if (vector.DistanceSq(position, character.GetOrigin()) < 0.0625)
				error = "Player already has clear space.";
		}
		if (error.IsEmpty() && !controller.GC_BeginAdminTeleport(requesterId, GetPlayerVoiceIdentity(requesterId), requestId,
			character, position, context, false, "unstuck"))
			error = "A move is already pending.";
		if (!error.IsEmpty())
		{
			GC_AdminActionLogger.Log(requesterId, "unstuck", context, error, targetId, targetIdentity);
			return error;
		}
		GC_AdminActionLogger.Log(requesterId, "unstuck", context, "dispatched-awaiting-position", targetId, targetIdentity);
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteAdminTeleport(int requesterId, int fromId, string fromIdentity, int toId, string toIdentity, int requestId)
	{
		string context = string.Format("from=%1 identity=%2 to=%3 identity=%4 request=%5", fromId, fromIdentity, toId, toIdentity, requestId);
		if (!AuthorizeAdminAction(requesterId, "teleport", context))
			return "Admin login required.";
		string error;
		ChimeraCharacter source = GC_GetAdminToolCharacter(fromId);
		ChimeraCharacter destination = GC_GetAdminToolCharacter(toId);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(fromId));
		if (fromIdentity.IsEmpty() || toIdentity.IsEmpty() || GetPlayerVoiceIdentity(fromId) != fromIdentity || GetPlayerVoiceIdentity(toId) != toIdentity)
			error = "A selected player changed or disconnected. Select again.";
		else if (fromId == toId)
			error = "Choose different From and To players.";
		else if (!source || !destination || !controller)
			error = "Both players must have living spawned characters.";
		else if (source.GetRootParent() != source)
			error = "The moving player must leave their vehicle first.";
		vector position;
		if (error.IsEmpty() && (!SCR_WorldTools.FindEmptyTerrainPosition(position, destination.GetOrigin(), 5)
			|| !SCR_Global.IsPositionWithinTerrainBounds(position)
			|| vector.DistanceSq(position, destination.GetOrigin()) > 25))
			error = "No clear landing position near that player.";
		if (!error.IsEmpty())
		{
			GC_AdminActionLogger.Log(requesterId, "teleport", context, error);
			return error;
		}
		if (!controller.GC_BeginAdminTeleport(requesterId, GetPlayerVoiceIdentity(requesterId), requestId, source, position, context))
		{
			GC_AdminActionLogger.Log(requesterId, "teleport", context, "rejected-already-pending");
			return "That player already has a teleport in progress.";
		}
		GC_AdminActionLogger.Log(requesterId, "teleport", context, "dispatched-awaiting-position");
		return string.Empty;
	}


	//------------------------------------------------------------------------------------------------
	string GC_ExecuteFactionTeleport(int requesterId, string factionKey, int toId, string toIdentity, int requestId)
	{
		string context = string.Format("faction=%1 to=%2 identity=%3 request=%4", factionKey, toId, toIdentity, requestId);
		if (!AuthorizeAdminAction(requesterId, "teleport-faction", context))
			return "Admin login required.";
		string error;
		array<int> players;
		array<string> identities;
		ChimeraCharacter destination = GC_GetAdminToolCharacter(toId);
		GC_LobbyPlayerController requester = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (!GC_GetFactionPlayers(factionKey, players, identities))
			error = "The selected faction is unavailable.";
		else if (players.IsEmpty())
			error = "No connected players are in that faction.";
		else if (!destination || toIdentity.IsEmpty() || GetPlayerVoiceIdentity(toId) != toIdentity)
			error = "The destination player must still have a living spawned character.";
		else if (!requester || requester.GC_HasFactionTeleport())
			error = "A faction teleport is already in progress.";
		if (!error.IsEmpty())
		{
			GC_AdminActionLogger.Log(requesterId, "teleport-faction", context, error);
			return error;
		}

		// Plan before dispatch so remote owners cannot receive overlapping landing points.
		array<ref GC_AdminFactionLanding> landings = {};
		array<int> moving = {};
		int skipped;
		int candidateIndex;
		vector centre = destination.GetOrigin();
		foreach (int playerId : players)
		{
			if (playerId == toId)
				continue;
			ChimeraCharacter character = GC_GetAdminToolCharacter(playerId);
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!character || character.GetRootParent() != character || !controller)
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "teleport-faction-player", context + " player=" + playerId, "skipped-unavailable-or-seated");
				continue;
			}
			bool found;
			vector position;
			for (int attempt; attempt < 8 && !found; attempt++)
			{
				float angle = candidateIndex * 2.399963;
				float radius = 4 + 2 * Math.Sqrt(candidateIndex++);
				vector candidate = centre + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
				if (!SCR_WorldTools.FindEmptyTerrainPosition(position, candidate, 2)
					|| !SCR_Global.IsPositionWithinTerrainBounds(position)
					|| vector.DistanceSq(position, candidate) > 4 || vector.DistanceSq(position, centre) < 9)
					continue;
				found = true;
				foreach (GC_AdminFactionLanding planned : landings)
				{
					if (vector.DistanceSq(position, planned.m_vPosition) < 9)
						found = false;
				}
			}
			if (!found)
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "teleport-faction-player", context + " player=" + playerId, "skipped-no-safe-position");
				continue;
			}
			GC_AdminFactionLanding landing = new GC_AdminFactionLanding();
			landing.m_iPlayerId = playerId;
			landing.m_Character = character;
			landing.m_Controller = controller;
			landing.m_vPosition = position;
			landings.Insert(landing);
			moving.Insert(playerId);
		}
		if (moving.IsEmpty())
		{
			GC_AdminActionLogger.Log(requesterId, "teleport-faction", context, "no-eligible-players");
			return "No eligible players to move. The destination player stays in place; dead, seated or blocked players are skipped.";
		}
		requester.GC_StartFactionTeleport(requestId, moving, skipped, context);
		foreach (GC_AdminFactionLanding landing : landings)
		{
			string targetContext = context + " player=" + landing.m_iPlayerId;
			if (!landing.m_Controller.GC_BeginAdminTeleport(requesterId, GetPlayerVoiceIdentity(requesterId), requestId,
				landing.m_Character, landing.m_vPosition, targetContext, true))
			{
				GC_AdminActionLogger.Log(requesterId, "teleport-faction-player", targetContext, "rejected-already-pending");
				requester.GC_RecordFactionTeleport(requestId, landing.m_iPlayerId, false);
			}
		}
		GC_AdminActionLogger.Log(requesterId, "teleport-faction", context, "dispatched-awaiting-positions");
		return string.Empty;
	}

}
