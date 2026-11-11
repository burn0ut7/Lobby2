//! Authored server actions use existing Lobby2 owners. No client request or admin authorization bypass.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	bool GC_ExecuteFrameworkAction(GC_MissionRuleEngine source, GC_RuleAction action)
	{
		if (!GC_HasFrameworkSource(source) || !action || !action.IsValid())
			return false;
		switch (action.m_eDo)
		{
			case GC_ERuleAction.CUSTOM_SCRIPT:
				bool result;
				return GC_RunFrameworkScript(source, action.m_Script, result) && result;
			case GC_ERuleAction.RESPAWN_WAVE:
				return GC_RespawnWave(action.m_aFlagNames, action.m_iAmount, action.m_bFlagAnswer, action.m_bRandomOrder) >= 0;
			case GC_ERuleAction.JIP:
				GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
				return mode && mode.SetJipMode(action.m_iOperation);
			case GC_ERuleAction.MARKER_VISIBILITY:
				return GC_ChangeFrameworkMarkerVisibility(action);
			case GC_ERuleAction.VEHICLE:
				return GC_ChangeFrameworkVehicles(action);
			case GC_ERuleAction.WEATHER:
				return GC_ChangeFrameworkWeather(action);
			case GC_ERuleAction.DELETE_ENTITIES:
				return GC_DeleteFrameworkEntities(action);
			case GC_ERuleAction.ASSIGN_WAYPOINTS:
				return GC_WaypointOrders.Apply(AIGroup.Cast(GetGame().GetWorld().FindEntityByName(action.m_sTextOrName)), action.m_aFlagNames, action.m_bFlagAnswer);
			case GC_ERuleAction.SPAWN:
				return action.m_Spawn.Execute();
			case GC_ERuleAction.MISSION_MESSAGE:
				return GC_SendMissionMessage(action);
			case GC_ERuleAction.END_MISSION:
				return GC_EndFrameworkMission(action);
			case GC_ERuleAction.RESPAWN_TICKETS:
				return GC_ChangeFrameworkTickets(action);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_HasFrameworkSource(GC_MissionRuleEngine source)
	{
		return HasMissionRuleAuthority() && m_bMissionEventsEnabled && source && source == m_MissionRuleEngine && source.IsRunning();
	}

	//------------------------------------------------------------------------------------------------
	//! Shared dispatch for script actions and sampled conditions; false means unavailable, not a false answer.
	bool GC_RunFrameworkScript(GC_MissionRuleEngine source, GC_MissionScript script, out bool result)
	{
		result = false;
		if (!GC_HasFrameworkSource(source) || !script || !script.IsValid())
			return false;
		result = script.Run(this);
		return GC_HasFrameworkSource(source);
	}

	//------------------------------------------------------------------------------------------------
	//! Called only after the framework's authority and active-engine checks.
	protected bool GC_ChangeFrameworkVehicles(GC_RuleAction action)
	{
		bool supported;
		array<IEntity> visited = {};
		foreach (string name : action.m_aFlagNames)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity || visited.Contains(entity))
				continue;
			visited.Insert(entity);
			if (action.m_bChangeFuel)
			{
				array<SCR_FuelManagerComponent> fuelManagers = {};
				SCR_FuelManagerComponent.GetAllFuelManagers(entity, fuelManagers);
				if (!fuelManagers.IsEmpty())
				{
					SCR_FuelManagerComponent.SetTotalFuelPercentageOfFuelManagers(fuelManagers, action.m_fFuelRatio);
					supported = true;
				}
			}
			if (action.m_bChangeSupplies)
			{
				SCR_ResourceComponent resources = SCR_ResourceComponent.FindResourceComponent(entity, false);
				SCR_ResourceContainer container;
				if (resources && resources.GetContainer(EResourceType.SUPPLIES, container))
				{
					container.SetResourceValue(action.m_iAmount);
					supported = true;
				}
			}
			if (action.m_bRepair)
			{
				SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
				if (damage)
				{
					damage.FullHeal();
					supported = true;
				}
			}
		}
		return supported;
	}

	//------------------------------------------------------------------------------------------------
	//! Reuse the catalog's revision, reliable update and join-in-progress snapshot paths.
	protected bool GC_ChangeFrameworkMarkerVisibility(GC_RuleAction action)
	{
		FactionManager factions = GetGame().GetFactionManager();
		foreach (GC_MarkerFactionVisibility visibility : action.m_aMarkerFactions)
		{
			if (!factions || !factions.GetFactionByKey(visibility.m_sFaction))
				return false;
		}
		array<ref GC_BriefingMarker> updates = {};
		array<string> visited = {};
		foreach (string name : action.m_aFlagNames)
		{
			GC_BriefingMarker marker = GetBriefingMarker(name);
			if (!marker || visited.Contains(name))
				continue;
			visited.Insert(name);
			GC_BriefingMarker update = new GC_BriefingMarker();
			update.CopyFrom(marker);
			update.SetFactionVisibility("", action.m_bFlagAnswer);
			foreach (GC_MarkerFactionVisibility visibility : action.m_aMarkerFactions)
				update.SetFactionVisibility(visibility.m_sFaction, visibility.m_bVisible);
			if (!ValidateBriefingMarker(update))
				return false;
			updates.Insert(update);
		}
		if (updates.IsEmpty())
			return false;
		foreach (GC_BriefingMarker update : updates)
		{
			if (!UpsertBriefingMarker(update))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_ChangeFrameworkWeather(GC_RuleAction action)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return false;
		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		if (!weather)
			return false;
		// Validate all requested changes before mutation. Time-only actions do not need a weather preset.
		if (action.m_bChangeWeather)
		{
			if (!weather.GetTransitionManager() || !GC_WeatherAction.GetPresetNames(world).Contains(action.m_sTextOrName))
				return false;
		}
		if (action.m_bChangeTime && !weather.SetHoursMinutesSeconds(action.m_iHours, action.m_iMinutes, action.m_iSeconds))
			return false;
		// Native API uses hours and owns transitions, looping and network updates.
		if (action.m_bChangeWeather)
			weather.ForceWeatherTo(!action.m_bFlagAnswer, action.m_sTextOrName, action.m_fTransitionSeconds / 3600.0);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_DeleteFrameworkEntities(GC_RuleAction action)
	{
		array<IEntity> targets = {};
		foreach (string name : action.m_aFlagNames)
		{
			IEntity target = GetGame().GetWorld().FindEntityByName(name);
			if (!target || targets.Contains(target))
				continue;
			// Reject the entire list before mutation if it would destroy this action's owner.
			IEntity ancestor = GetOwner();
			while (ancestor)
			{
				if (ancestor == target)
					return false;
				ancestor = ancestor.GetParent();
			}
			targets.Insert(target);
		}
		foreach (IEntity entity : targets)
		{
			// An earlier parent deletion may already have removed this target.
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_SendMissionMessage(GC_RuleAction action)
	{
		array<int> recipients = {};
		GC_EAdminMessageAudience audience = GC_EAdminMessageAudience.ALL_PLAYERS;
		if (action.m_iOperation == 1)
		{
			audience = GC_EAdminMessageAudience.ADMINS;
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int playerId : players)
			{
				if (SCR_Global.IsAdmin(playerId))
					recipients.Insert(playerId);
			}
		}
		else if (!action.m_aFlagNames || action.m_aFlagNames.IsEmpty())
			GetGame().GetPlayerManager().GetPlayers(recipients);
		else
		{
			audience = GC_EAdminMessageAudience.FACTION;
			foreach (string faction : action.m_aFlagNames)
			{
				array<int> players;
				array<string> identities;
				if (!GC_GetFactionPlayers(faction, players, identities))
					return false;
				foreach (int playerId : players)
				{
					if (!recipients.Contains(playerId))
						recipients.Insert(playerId);
				}
			}
		}
		m_iAdminMessageSequence++;
		foreach (int recipient : recipients)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(recipient));
			if (controller)
				controller.DeliverAdminMessage(m_iAdminMessageSequence, 0, "Mission", audience, action.m_sTextOrName, true, action.m_bFlagAnswer, true, action.m_sMessageTitle);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_EndFrameworkMission(GC_RuleAction action)
	{
		if (GetLobbyPhase() != SCR_EGameModeState.GAME)
			return false;
		int factionIndex = -1;
		EGameOverTypes reason = EGameOverTypes.EDITOR_NEUTRAL;
		if (action.m_iOperation > 0 && action.m_iOperation < 4)
		{
			FactionManager factions = GetGame().GetFactionManager();
			Faction faction = factions.GetFactionByKey(action.m_sTextOrName);
			if (!faction)
				return false;
			factionIndex = factions.GetFactionIndex(faction);
			switch (action.m_iOperation)
			{
				case 1:
					reason = EGameOverTypes.EDITOR_FACTION_VICTORY;
					break;
				case 2:
					reason = EGameOverTypes.EDITOR_FACTION_DEFEAT;
					break;
				case 3:
					reason = EGameOverTypes.EDITOR_FACTION_DRAW;
					break;
			}
		}
		if (action.m_iOperation == 4)
			reason = EGameOverTypes.LOOSE;
		array<int> results = {};
		if (factionIndex >= 0)
			results.Insert(factionIndex);
		if (action.m_aFlagNames)
		{
			foreach (string key : action.m_aFlagNames)
			{
				Faction winner = GetGame().GetFactionManager().GetFactionByKey(key);
				if (!winner)
					return false;
				int index = GetGame().GetFactionManager().GetFactionIndex(winner);
				if (!results.Contains(index))
					results.Insert(index);
			}
		}
		ref SCR_GameModeEndData data = SCR_GameModeEndData.Create(reason, null, results);
		return EnterPostGame(data, action.m_sEndMessage);
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_ChangeFrameworkTickets(GC_RuleAction action)
	{
		int tickets = GC_GetFactionRespawnTickets(action.m_sTextOrName);
		if (action.m_iOperation == 0)
			tickets = action.m_iAmount;
		else if (action.m_iOperation == 1)
		{
			if (tickets > int.MAX - action.m_iAmount)
				return false;
			tickets += action.m_iAmount;
		}
		else
			tickets = Math.Max(0, tickets - action.m_iAmount);
		return GC_SetFactionRespawnTickets(action.m_sTextOrName, tickets);
	}
}
