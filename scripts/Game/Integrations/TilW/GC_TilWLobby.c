//! Lobby2 owns this adapter; TilW remains usable without a reverse dependency.
modded class TILW_Lobby
{
	//------------------------------------------------------------------------------------------------
	static override SCR_BaseGameMode GetGameMode()
	{
		GC_LobbyGameMode lobby2 = GC_LobbyGameMode.GetGameMode();
		if (lobby2)
			return lobby2;

		return super.GetGameMode();
	}

	//------------------------------------------------------------------------------------------------
	static override ScriptInvokerInt GetOnGameStateChange()
	{
		GC_LobbyGameMode lobby2 = GC_LobbyGameMode.GetGameMode();
		if (lobby2)
			return lobby2.TILW_GetOnGameStateChange();

		return super.GetOnGameStateChange();
	}

	//------------------------------------------------------------------------------------------------
	static override FactionKey GetPlayerFactionKey(int playerId)
	{
		if (GC_LobbyGameMode.GetGameMode())
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction;
			if (factionManager)
				faction = factionManager.GetPlayerFaction(playerId);
			if (faction)
				return faction.GetFactionKey();
			return "";
		}

		return super.GetPlayerFactionKey(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static override void AddFactionChangeListener(notnull TILW_AOLimitComponent listener)
	{
		if (GC_LobbyGameMode.GetGameMode())
		{
			listener.TILW_ListenToLobby2Faction();
			return;
		}

		super.AddFactionChangeListener(listener);
	}

	//------------------------------------------------------------------------------------------------
	static override bool IsSpectator(IEntity entity)
	{
		if (GC_LobbyGameMode.GetGameMode())
			return entity && entity.FindComponent(GC_LobbySpectatorBodyComponent) != null;

		return super.IsSpectator(entity);
	}

	//------------------------------------------------------------------------------------------------
	static override bool IsUnplayedSlotCharacter(IEntity entity)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
			return entity && (IsSpectator(entity) || lobby2.GetSlotByEntity(entity) != null);

		return super.IsUnplayedSlotCharacter(entity);
	}

	//------------------------------------------------------------------------------------------------
	static override bool IsMissionAI(IEntity entity)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
		{
			if (!entity || IsSpectator(entity) || lobby2.GetSlotByEntity(entity))
				return false;
			AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (control && control.GetAIAgent())
			{
				SCR_AIGroup group = SCR_AIGroup.Cast(control.GetAIAgent().GetParentGroup());
				if (group && group.IsPlayable())
					return false;
			}
			return true;
		}

		return super.IsMissionAI(entity);
	}

	//------------------------------------------------------------------------------------------------
	static override void RemoveOpenSlots(notnull array<string> factionKeys)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
		{
			lobby2.GC_RemoveOpenMissionSlots(factionKeys);
			return;
		}

		super.RemoveOpenSlots(factionKeys);
	}

	//------------------------------------------------------------------------------------------------
	static override int GetFactionTickets(FactionKey factionKey)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
			return lobby2.GC_GetFactionRespawnTickets(factionKey);

		return super.GetFactionTickets(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	static override void SetFactionTickets(FactionKey factionKey, int tickets)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
		{
			lobby2.GC_SetFactionRespawnTickets(factionKey, tickets);
			return;
		}

		super.SetFactionTickets(factionKey, tickets);
	}

	//------------------------------------------------------------------------------------------------
	static override void RespawnDeadPlayers(FactionKey factionKey)
	{
		GC_LobbyManager lobby2 = GC_LobbyManager.GetManager();
		if (lobby2)
		{
			lobby2.GC_RespawnFactionSlots(factionKey);
			return;
		}

		super.RespawnDeadPlayers(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	static override void SetMapItemVisibility(IEntity item, bool visibleForEmpty, array<ref TILW_FactionVisibility> factionVisibility)
	{
		if (GC_LobbyGameMode.GetGameMode())
		{
			Print("TILWMF | Edit Map Item is not supported for Lobby2 catalog audiences yet; no visibility was changed.", LogLevel.ERROR);
			return;
		}

		super.SetMapItemVisibility(item, visibleForEmpty, factionVisibility);
	}
}
