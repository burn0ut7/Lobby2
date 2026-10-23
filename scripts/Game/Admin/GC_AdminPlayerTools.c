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
	string GC_ExecuteAdminHeal(int requesterId, GC_EAdminHealScope scope, array<int> selectedIds, array<string> selectedIdentities, int requestId, int radiusMeters, string factionKey = "")
	{
		string context = string.Format("scope=%1 radius=%2 request=%3 faction=%4", scope, radiusMeters, requestId, factionKey);
		if (!AuthorizeAdminAction(requesterId, "heal", context))
			return "Admin login required.";
		if (scope < GC_EAdminHealScope.SELECTED || scope > GC_EAdminHealScope.FACTION)
		{
			GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-scope");
			return "Invalid healing scope.";
		}
		if (scope == GC_EAdminHealScope.NEARBY && (radiusMeters < GC_MIN_HEAL_RADIUS || radiusMeters > GC_MAX_HEAL_RADIUS))
		{
			GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-radius");
			return string.Format("Choose a distance from %1 to %2 metres.", GC_MIN_HEAL_RADIUS, GC_MAX_HEAL_RADIUS);
		}

		if (scope == GC_EAdminHealScope.FACTION && !GC_MatchesFactionSelection(factionKey, selectedIds, selectedIdentities))
		{
			GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-stale-faction-selection");
			return "Faction membership changed or is empty. Review the faction and try again.";
		}
		array<int> targets = {};
		if (scope == GC_EAdminHealScope.SELECTED || scope == GC_EAdminHealScope.FACTION)
		{
			if (!selectedIds || !selectedIdentities || selectedIds.IsEmpty() || selectedIds.Count() != selectedIdentities.Count() || selectedIds.Count() > 128)
			{
				GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-selection");
				return "Choose at least one player.";
			}
			// Validate the entire selection before changing anyone's health.
			foreach (int index, int playerId : selectedIds)
			{
				if (selectedIdentities[index].IsEmpty() || GetPlayerVoiceIdentity(playerId) != selectedIdentities[index]
					|| !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				{
					GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-stale-selection");
					return "A selected player disconnected. Select players again.";
				}
				if (!targets.Contains(playerId))
					targets.Insert(playerId);
			}
		}
		else
			GetGame().GetPlayerManager().GetPlayers(targets);

		ChimeraCharacter origin;
		if (scope == GC_EAdminHealScope.NEARBY)
		{
			origin = GC_GetAdminToolCharacter(requesterId);
			if (!origin)
			{
				GC_AdminActionLogger.Log(requesterId, "heal", context, "rejected-no-admin-character");
				return "Spawn as a player to heal nearby players.";
			}
		}

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
	//! The same snapshot feeds the preview and authority validation, including KIA claims.
	void GC_GetFactionPlayers(string factionKey, out array<int> players, out array<string> identities)
	{
		players = {};
		identities = {};
		if (factionKey.IsEmpty())
			return;
		foreach (GC_Slot slot : GetSlots())
		{
			SCR_AIGroup group = slot.GetGroup();
			if (!group || !group.GetFaction() || group.GetFaction().GetFactionKey() != factionKey)
				continue;
			int playerId = slot.GetPlayerId();
			if (playerId <= 0 || players.Contains(playerId) || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			players.Insert(playerId);
			identities.Insert(GetPlayerVoiceIdentity(playerId));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_MatchesFactionSelection(string factionKey, array<int> players, array<string> identities)
	{
		if (!players || !identities || players.IsEmpty() || players.Count() > 128 || players.Count() != identities.Count())
			return false;
		array<int> current;
		array<string> currentIdentities;
		GC_GetFactionPlayers(factionKey, current, currentIdentities);
		if (current.Count() != players.Count())
			return false;
		foreach (int index, int playerId : current)
		{
			int selected = players.Find(playerId);
			if (selected < 0 || identities[selected].IsEmpty() || identities[selected] != currentIdentities[index])
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteFactionTeleport(int requesterId, string factionKey, array<int> players, array<string> identities, int toId, string toIdentity, int requestId)
	{
		string context = string.Format("faction=%1 to=%2 identity=%3 request=%4", factionKey, toId, toIdentity, requestId);
		if (!AuthorizeAdminAction(requesterId, "teleport-faction", context))
			return "Admin login required.";
		string error;
		ChimeraCharacter destination = GC_GetAdminToolCharacter(toId);
		GC_LobbyPlayerController requester = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (!GC_MatchesFactionSelection(factionKey, players, identities))
			error = "Faction membership changed or is empty. Review the faction and confirm again.";
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
