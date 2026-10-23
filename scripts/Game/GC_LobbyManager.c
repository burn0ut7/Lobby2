class GC_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_LobbyManager : SCR_BaseGameModeComponent
{
	protected const ResourceName SPECTATOR_PREFAB = "{8A578DE95771CC1D}Prefabs/Characters/Core/Character_Spectator.et";
	protected const ResourceName DEFAULT_FORMATION_SOURCE = "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et";
	protected const string DEFAULT_FORMATION_NAME = "File";
	protected const bool FORMATION_DEBUG_LOGGING = true;
	protected const int FORMATION_DEBUG_LOG_LIMIT = 64;
	protected const int JIP_REFERENCE_RETRY_MS = 1000;
	protected const int PLAYER_CATCHUP_RETRY_MS = 250;
	protected const int PLAYER_CATCHUP_MAX_RETRIES = 8;
	protected const int BRIEFING_SPAWN_BATCH_SIZE = 4;
	protected const int ROLE_PRESENTATION_RETRY_MS = 100;
	protected const int ROLE_PRESENTATION_MAX_RETRIES = 20;

    //! Game-mode setting: group headers in the voice list may be selectable
    //! without changing the player's lobby slot or base-game group.
    [Attribute("1", UIWidgets.CheckBox, "Allow group voice-channel selection")]
    protected bool m_bVoiceGroupHeaderSwitchesGroup;

	//! Game-mode setting: faction-assigned players who are still in Global when
	//! briefing begins are moved to their base-game group by default, or to HQ.
	[Attribute("0", UIWidgets.ComboBox, "Default faction voice channel during briefing", enumType: GC_EBriefingVoiceChannel)]
	protected GC_EBriefingVoiceChannel m_eBriefingDefaultVoiceChannel;

	//! Server-side grace period for reconnecting to a claimed slot.
	[Attribute("120", UIWidgets.EditBox, "Reconnect grace period in seconds (0 releases immediately)")]
	protected int m_iSlotReservationTimeoutSeconds;

	[Attribute("{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et", UIWidgets.ResourcePickerThumbnail, "AI world resource used only as a formation-data source", params: "et")]
	protected ResourceName m_sFormationSource;

	[Attribute("File", UIWidgets.EditBox, "Formation name used when a group formation is unavailable")]
	protected string m_sDefaultFormation;

	[Attribute("", UIWidgets.Object, "Lobby2 briefing and marker catalog")]
	protected ref GC_LobbyBriefingCatalog m_BriefingCatalog;

	[Attribute("", UIWidgets.Object, "Lobby2 scenario parameter definitions and runtime values")]
	protected ref GC_LobbyParameterCatalog m_ParameterCatalog;

	protected ref GC_LobbyRoster m_Roster = new GC_LobbyRoster();
	protected ref GC_LobbyVoiceState m_VoiceState = new GC_LobbyVoiceState();
    //! Narrow presentation events. Payloads identify the changed authoritative
    //! record; consumers re-query base-game state and update only affected widgets.
    ref ScriptInvoker<int> m_OnRegistryReset = new ScriptInvoker<int>();
    ref ScriptInvoker<int> m_OnPlayerConnected = new ScriptInvoker<int>();
    ref ScriptInvoker<int> m_OnPlayerDisconnected = new ScriptInvoker<int>();
    ref ScriptInvoker<int> m_OnPlayerAffiliationChanged = new ScriptInvoker<int>();
    ref ScriptInvoker<Faction> m_OnFactionTopologyChanged = new ScriptInvoker<Faction>();
    ref ScriptInvoker<Faction> m_OnFactionPresentationChanged = new ScriptInvoker<Faction>();
    ref ScriptInvoker<GC_Slot> m_OnSlotPresentationChanged = new ScriptInvoker<GC_Slot>();
    ref ScriptInvoker<int> m_OnVoiceAssignmentChanged = new ScriptInvoker<int>();
    ref ScriptInvoker<int> m_OnVoiceTopologyChanged = new ScriptInvoker<int>();
    ref ScriptInvoker<int> m_OnPlayerRoleChanged = new ScriptInvoker<int>();
    //! Signals that the complete local briefing view changed. Consumers re-query
    //! this manager and current authoritative faction state.
    ref ScriptInvoker<int> m_OnBriefingsChanged = new ScriptInvoker<int>();
	ref ScriptInvoker<int> m_OnBriefingMarkersChanged = new ScriptInvoker<int>();
	ref ScriptInvoker<SCR_EGameModeState> m_OnLobbyPhaseChanged = new ScriptInvoker<SCR_EGameModeState>();
	ref ScriptInvoker<int> m_OnScenarioParametersChanged = new ScriptInvoker<int>();
	protected ref GC_PlayerFormationCatalog m_FormationCatalog = new GC_PlayerFormationCatalog();
	protected ref array<ref GC_PlayerGroupSpawnTransform> m_aGroupSpawnTransforms = {};
	protected bool m_bShutdown;
	protected bool m_bFormationDefaultLogged;
	protected bool m_bFormationFallbackLogged;
	protected int m_iFormationDebugLogCount;
	protected ref map<int, IEntity> m_mLobbySpectators = new map<int, IEntity>();
	protected ref map<UUID, IEntity> m_mReservedSpectators = new map<UUID, IEntity>();
	protected ref array<GC_Slot> m_aPendingBriefingSpawns = {};
	protected ref array<int> m_aPendingPlayerCatchUps = {};
	protected ref map<int, int> m_mPlayerCatchUpAttempts = new map<int, int>();
	//! Transport-convergence hints only. Never consulted for authorization or UI state.
	protected ref map<int, bool> m_mPendingRoleAdmin = new map<int, bool>();
	protected ref map<int, int> m_mPendingRoleAttempts = new map<int, int>();
	protected ref set<int> m_sPendingAffiliationPlayers = new set<int>();
	protected ref set<Faction> m_sPendingFactionPresentation = new set<Faction>();
	protected bool m_bBriefingSpawnQueued;
	protected bool m_bCatchUpQueued;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (Replication.IsServer() && !SCR_Global.IsEditMode())
			EnsureFormationCatalog();
	}

	protected bool EnsureParameterCatalog()
	{
		if (!m_ParameterCatalog)
			m_ParameterCatalog = new GC_LobbyParameterCatalog();

		return m_ParameterCatalog.Initialize();
	}

	protected bool EnsureFormationCatalog()
	{
		if (m_sFormationSource == string.Empty)
			m_sFormationSource = DEFAULT_FORMATION_SOURCE;
		if (m_sDefaultFormation == string.Empty)
			m_sDefaultFormation = DEFAULT_FORMATION_NAME;

		if (!m_FormationCatalog)
			m_FormationCatalog = new GC_PlayerFormationCatalog();

		return m_FormationCatalog.Load(m_sFormationSource);
	}

	protected string ResolveGroupFormationName(SCR_AIGroup group)
	{
		string formationName = m_sDefaultFormation;
		AIFormationComponent formationComponent = null;
		AIFormationDefinition formationDefinition = null;
		BaseContainer formationSource;
		string authoredFormationName;
		if (group)
			formationComponent = AIFormationComponent.Cast(group.FindComponent(AIFormationComponent));
		if (formationComponent)
			formationDefinition = formationComponent.GetFormation();
		if (formationDefinition && formationDefinition.GetName() != string.Empty)
			return formationDefinition.GetName();

		// A playable group can author DefaultFormation on its world instance while
		// no live AI world exists to resolve a native AIFormationDefinition. The
		// component source preserves that effective config value without loading
		// the group prefab or AI-world resource during member spawning.
		if (formationComponent && group)
			formationSource = formationComponent.GetComponentSource(group);
		if (formationSource && formationSource.Get("DefaultFormation", authoredFormationName) && authoredFormationName != string.Empty)
			return authoredFormationName;

		if (!m_bFormationDefaultLogged)
		{
			m_bFormationDefaultLogged = true;
			PrintFormat("GC | Formation | using fallback formation: %1", m_sDefaultFormation);
		}

		return formationName;
	}

	//! Returns one local-space offset without loading a resource per group/member.
	vector ResolveGroupMemberOffset(SCR_AIGroup group, int slotIndex)
	{
		EnsureFormationCatalog();
		string formationName = ResolveGroupFormationName(group);

		vector offset;
		if (m_FormationCatalog && m_FormationCatalog.TryGetOffset(formationName, slotIndex, offset))
			return offset;

		if (formationName != m_sDefaultFormation && m_FormationCatalog && m_FormationCatalog.TryGetOffset(m_sDefaultFormation, slotIndex, offset))
			return offset;

		if (!m_bFormationFallbackLogged)
		{
			m_bFormationFallbackLogged = true;
			PrintFormat("GC | Formation | using linear fallback: formation=%1 default=%2 slot=%3", formationName, m_sDefaultFormation, slotIndex, level: LogLevel.WARNING);
		}

		return Vector(slotIndex * 2, 0, 0);
	}

	//! Re-evaluate briefing presentation on every machine when the authoritative
	//! base-game phase changes. The phase itself remains owned and replicated by
	//! SCR_BaseGameMode.
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		string role = "proxy";
		if (Replication.IsServer())
			role = "authority";

		PrintFormat("GC | Phase | base state event: state=%1 role=%2", GetPhaseName(state), role);

		if (Replication.IsServer() && state == SCR_EGameModeState.BRIEFING)
		{
			ReconcileBriefingVoiceAssignments();
			QueueCatchUpConnectedPlayers();
		}
		else if (Replication.IsServer() && state == SCR_EGameModeState.POSTGAME)
		{
			array<int> connectedPlayers = {};
			GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
			foreach (int playerId : connectedPlayers)
			{
				GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				if (controller && controller.GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR)
					controller.SetLobbySpectatorState(false);
			}
		}

		if (state == SCR_EGameModeState.GAME && Replication.IsServer())
			QueueCatchUpConnectedPlayers();

		m_OnLobbyPhaseChanged.Invoke(state);
		m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
		m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
	}

	SCR_EGameModeState GetLobbyPhase()
	{
		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		if (!gameMode)
			return SCR_EGameModeState.PREGAME;

		return gameMode.GetState();
	}

	static string GetPhaseName(SCR_EGameModeState phase)
	{
		return GC_LobbyPhase.GetName(phase);
	}

	static string GetBaseStateName(SCR_EGameModeState state)
	{
		switch (state)
		{
			case SCR_EGameModeState.PREGAME: return "PREGAME";
			case SCR_EGameModeState.SLOTTING: return "SLOTTING";
			case SCR_EGameModeState.BRIEFING: return "BRIEFING";
			case SCR_EGameModeState.GAME: return "GAME";
			case SCR_EGameModeState.POSTGAME: return "POSTGAME";
		}

		return "UNKNOWN";
	}

	int GetSlotReservationTimeoutSeconds()
	{
		return Math.Max(0, m_iSlotReservationTimeoutSeconds);
	}

	//! Validates and applies an admin-owned request on the server authority.
	bool TryAdvanceLobbyPhase(int playerId, SCR_EGameModeState requestedPhase)
	{
		string requestedPhaseName = GetPhaseName(requestedPhase);
		if (m_bShutdown)
		{
			Print("GC | Phase | request rejected: Lobby2 manager is shutting down", LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-manager-shutdown");
			return false;
		}

		if (!Replication.IsServer())
		{
			Print("GC | Phase | request rejected: handler is not running on authority", LogLevel.ERROR);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-not-authority");
			return false;
		}

		PrintFormat("GC | Phase | request received: player=%1 current=%2 requested=%3 requestedValue=%4", playerId, GetPhaseName(GetLobbyPhase()), requestedPhaseName, requestedPhase);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerId <= 0 || !playerManager || !playerManager.IsPlayerConnected(playerId))
		{
			PrintFormat("GC | Phase | request rejected: player=%1 is not connected", playerId, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-not-logged-in");
			return false;
		}

		if (!GC_LobbyPhase.IsValid(requestedPhase))
		{
			PrintFormat("GC | Phase | request rejected: invalid target value=%1", requestedPhase, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-invalid-target");
			return false;
		}

		if (!SCR_Global.IsAdmin(playerId))
		{
			if (GC_UIEventDiagnostics.ENABLED)
				PrintFormat("GC | Admin Trace | server action rejected: action=advance-phase player=%1 queriedAdmin=false", playerId, level: LogLevel.WARNING);
			PrintFormat("GC | Phase | request rejected: player=%1 is not an admin", playerId, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-not-admin");
			return false;
		}

		SCR_EGameModeState nextPhase = GC_LobbyPhase.GetNext(GetLobbyPhase());
		if (requestedPhase != nextPhase)
		{
			PrintFormat("GC | Phase | request rejected: player=%1 current=%2 requested=%3 expected=%4", playerId, GetPhaseName(GetLobbyPhase()), GetPhaseName(requestedPhase), GetPhaseName(nextPhase), level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-not-next-phase");
			return false;
		}

		if (requestedPhase == SCR_EGameModeState.POSTGAME)
		{
			bool enteredPostGame = EnterPostGame(null);
			string outcome = "rejected-transition-failed";
			if (enteredPostGame)
				outcome = "succeeded";
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, outcome);
			return enteredPostGame;
		}

		SCR_BaseGameMode gameMode = GetGameMode();
		if (!gameMode)
		{
			Print("GC | Phase | request rejected: SCR_BaseGameMode is unavailable", LogLevel.ERROR);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-game-mode-unavailable");
			return false;
		}

		if (requestedPhase == SCR_EGameModeState.GAME)
		{
			if (gameMode.GetState() != SCR_EGameModeState.BRIEFING)
			{
				PrintFormat("GC | Phase | GAME transition rejected: expected state=BRIEFING actual=%1", GetBaseStateName(gameMode.GetState()), level: LogLevel.ERROR);
				GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-invalid-base-state");
				return false;
			}

			// Give every claimed slot one final authority-side spawn attempt before
			// the base game begins. A failed prefab does not block the mission; the
			// affected player remains a spectator and the error is logged.
			ForceSpawnClaimedSlots();

			PrintFormat("GC | Phase | invoking native StartGameMode: lobby=%1 base=%2", GetPhaseName(GetLobbyPhase()), GetBaseStateName(gameMode.GetState()));
			gameMode.StartGameMode();
			if (gameMode.GetState() != SCR_EGameModeState.GAME)
			{
				PrintFormat("GC | Phase | native StartGameMode failed: base=%1", GetBaseStateName(gameMode.GetState()), level: LogLevel.ERROR);
				GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-transition-failed");
				return false;
			}

			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "succeeded");
			return true;
		}

		if (!gameMode.GC_SetLobbyState(requestedPhase))
		{
			PrintFormat("GC | Phase | custom pre-match transition failed: requested=%1", GetPhaseName(requestedPhase), level: LogLevel.ERROR);
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-transition-failed");
			return false;
		}

		if (requestedPhase == SCR_EGameModeState.BRIEFING)
			QueueBriefingSlotSpawns();
		GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "succeeded");
		return true;
	}

	//! Shared authority entry point for admin overrides and normal mission logic.
	bool EnterPostGame(SCR_GameModeEndData endData)
	{
		if (m_bShutdown)
		{
			Print("GC | Phase | POSTGAME entry rejected: Lobby2 manager is shutting down", LogLevel.WARNING);
			return false;
		}

		if (!Replication.IsServer())
		{
			Print("GC | Phase | POSTGAME entry rejected: caller is not authority", LogLevel.ERROR);
			return false;
		}

		PrintFormat("GC | Phase | POSTGAME entry requested: lobby=%1 endDataProvided=%2", GetPhaseName(GetLobbyPhase()), endData != null);

		if (GetLobbyPhase() != SCR_EGameModeState.GAME)
		{
			PrintFormat("GC | Phase | POSTGAME entry rejected: expected lobby=GAME actual=%1", GetPhaseName(GetLobbyPhase()), level: LogLevel.WARNING);
			return false;
		}

		SCR_BaseGameMode gameMode = GetGameMode();
		if (!gameMode || gameMode.GetState() != SCR_EGameModeState.GAME)
		{
			string baseState = "UNAVAILABLE";
			if (gameMode)
				baseState = GetBaseStateName(gameMode.GetState());

			PrintFormat("GC | Phase | POSTGAME entry rejected: gameMode=%1 base=%2", gameMode != null, baseState, level: LogLevel.ERROR);
			return false;
		}

		if (!endData)
			endData = SCR_GameModeEndData.CreateSimple();

		PrintFormat("GC | Phase | invoking native EndGameMode: lobby=%1 base=%2", GetPhaseName(GetLobbyPhase()), GetBaseStateName(gameMode.GetState()));
		gameMode.EndGameMode(endData);
		if (gameMode.GetState() != SCR_EGameModeState.POSTGAME)
		{
			PrintFormat("GC | Phase | native EndGameMode failed: base=%1", GetBaseStateName(gameMode.GetState()), level: LogLevel.ERROR);
			return false;
		}

		return true;
	}

    override void OnPlayerAuditSuccess(int playerId)
    {
        PrintFormat("GC | OnPlayerAuditSuccess: %1, connected!", playerId);

		if (Replication.IsServer())
			QueuePlayerCatchUp(playerId);

    }

    override void OnPlayerConnected(int playerId)
    {
        super.OnPlayerConnected(playerId);
    }

    override void OnPlayerRegistered(int playerId)
    {
		m_OnPlayerConnected.Invoke(playerId);
        super.OnPlayerRegistered(playerId);
    }

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
		const KickCauseGroup2 causeGroup = KickCauseCodeAPI.GetGroup(cause);
		bool isShutdown = causeGroup == RplKickCauseGroup.REPLICATION && KickCauseCodeAPI.GetReason(cause) == RplError.SHUTDOWN;
		if (Replication.IsServer() && !isShutdown)
			HandlePlayerDisconnected(playerId);

		if (!isShutdown)
		{
			RemovePlayerVoiceAssignment(playerId);
			m_OnPlayerDisconnected.Invoke(playerId);
		}
        super.OnPlayerDisconnected(playerId, cause, timeout);
    }
    
    //! Make the player enter spectator mode from the server
    void SetSpectator(int playerId, bool state)
    {
        if (!Replication.IsServer())
            return;

		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
		{
			PrintFormat("GC | Lifecycle | spectator controller unavailable: player=%1", playerId, level: LogLevel.WARNING);
			return;
		}

		if (!state)
		{
			playerController.SetLobbyPlayingState();
			return;
		}

        if (state)
        {
			PrintFormat("GC | Lifecycle | spectator requested: player=%1", playerId);
			SCR_EGameModeState phase = GetLobbyPhase();
			bool activateCamera = phase == SCR_EGameModeState.GAME
				|| (phase == SCR_EGameModeState.BRIEFING && !m_Roster.GetSlotByPlayerId(playerId));

			IEntity existingSpectator = m_mLobbySpectators.Get(playerId);
			if (!existingSpectator)
			{
				UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
				existingSpectator = m_mReservedSpectators.Get(identity);
				if (existingSpectator)
					m_mReservedSpectators.Remove(identity);
			}
			if (existingSpectator)
			{
				if (playerController.SetLobbySpectator(existingSpectator, activateCamera))
				{
					m_mLobbySpectators.Set(playerId, existingSpectator);
					return;
				}

				SCR_EntityHelper.DeleteEntityAndChildren(existingSpectator);
				m_mLobbySpectators.Remove(playerId);
			}

            EntitySpawnParams params = new EntitySpawnParams();
            params.Transform[3] = GetOwner().GetOrigin();
            Resource spectatorPrefab = Resource.Load(SPECTATOR_PREFAB);
			PrintFormat("GC | Lifecycle | spectator prefab loaded: valid=%1", spectatorPrefab != null);
			if (!spectatorPrefab)
			{
				PrintFormat("GC | Lifecycle | spectator prefab unavailable: %1", SPECTATOR_PREFAB, level: LogLevel.ERROR);
				return;
			}
			IEntity spectator = GetGame().SpawnEntityPrefab(spectatorPrefab, null, params);
			PrintFormat("GC | Lifecycle | spectator spawned: valid=%1", spectator != null);

			if (!spectator || !playerController.SetLobbySpectator(spectator, activateCamera))
			{
				PrintFormat("GC | Lifecycle | spectator assignment failed: player=%1", playerId, level: LogLevel.WARNING);
				return;
			}

			m_mLobbySpectators.Set(playerId, spectator);
			UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			if (!identity.IsEmpty())
				m_mReservedSpectators.Remove(identity);
        }

    }

	//------------------------------------------------------------------------------------------------
	protected void HandlePlayerDisconnected(int playerId)
	{
		UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		GC_Slot slot = m_Roster.GetSlotByPlayerId(playerId);
		IEntity spectator = m_mLobbySpectators.Get(playerId);
		if (spectator && !identity.IsEmpty())
			m_mReservedSpectators.Set(identity, spectator);
		m_mLobbySpectators.Remove(playerId);

		if (!slot)
		{
			if (GetSlotReservationTimeoutSeconds() == 0)
				OnLobbyReservationExpired(identity, spectator);
			return;
		}
		if (slot.GetUUID().IsEmpty())
		{
			ReleaseSlotClaim(slot);
			UpdateSlot(slot);
			PrintFormat("GC | Lifecycle | released identity-less disconnected claim: player=%1 slot=%2", playerId, slot.GetID(), level: LogLevel.WARNING);
			return;
		}

		slot.MarkDisconnected();
		UpdateSlot(slot);
		if (GetSlotReservationTimeoutSeconds() == 0)
			OnLobbyReservationExpired(slot.GetUUID(), spectator);
	}

	//------------------------------------------------------------------------------------------------
	void OnLobbyReservationExpired(UUID identity, IEntity reservedEntity)
	{
		if (!Replication.IsServer() || GetManager() != this || identity.IsEmpty())
			return;

		GC_Slot slot = m_Roster.GetSlotByUUID(identity);
		// A stale expiry must never release a currently connected assignment.
		if (slot && slot.GetPlayerId() > 0)
			return;

		IEntity spectator = m_mReservedSpectators.Get(identity);
		if (spectator && spectator == reservedEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spectator);
			m_mReservedSpectators.Remove(identity);
		}

		if (!slot)
			return;

		ReleaseSlotClaim(slot);
		UpdateSlot(slot);
		PrintFormat("GC | Lifecycle | slot reservation expired: slot=%1", slot.GetID());
	}

	protected void CatchUpPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		SCR_EGameModeState phase = GetLobbyPhase();
		RegisterPlayerPersonalVoiceChannel(playerId, identity);
		PrintFormat("GC | Lifecycle | catch-up begin: player=%1 uuid=%2 phase=%3", playerId, identity, GetPhaseName(GetLobbyPhase()));
		SCR_ReconnectComponent reconnect = SCR_ReconnectComponent.GetInstance();
		if (reconnect)
			reconnect.HandlePlayerReconnect(playerId);

		// A connected player is authoritatively identified by the current session
		// player ID. UUID lookup is reserved for reconnecting players whose former
		// live player ID was cleared when they disconnected.
		GC_Slot slot = m_Roster.GetSlotByPlayerId(playerId);
		if (!slot)
			slot = m_Roster.GetSlotByUUID(identity);
		if (!slot)
		{
			PrintFormat("GC | Lifecycle | catch-up unassigned spectator: player=%1", playerId);
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
			SetSpectator(playerId, true);
			return;
		}

		slot.ReconnectPlayer(playerId);
		SCR_AIGroup group = slot.GetGroup();
		if (group && !group.GetPlayerIDs().Contains(playerId))
			group.AddPlayer(playerId);

		if (group && group.GetFaction() && phase == SCR_EGameModeState.BRIEFING)
			AssignPlayerToBriefingVoiceChannel(playerId);
		else if (group && group.GetFaction())
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, group.GetFaction().GetFactionKey(), Replication.FindItemId(group));
		else
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);

		if (slot.IsKIA())
		{
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
			SetSpectator(playerId, true);
			return;
		}

		if (phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING)
		{
			if (phase == SCR_EGameModeState.BRIEFING)
				SpawnSlotEntity(slot, false);
			SetSpectator(playerId, true);
			return;
		}

		if (phase == SCR_EGameModeState.GAME)
		{
			if (!slot.GetEntity())
				SpawnSlotEntity(slot, true);

			if (slot.GetEntity() && AssignSlotEntity(playerId, slot))
				return;

			PrintFormat("GC | Lifecycle | GAME catch-up fell back to spectator: player=%1 slot=%2", playerId, slot.GetID(), level: LogLevel.WARNING);
		}

		SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
		SetSpectator(playerId, true);
	}

	protected void QueuePlayerCatchUp(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0 || m_bShutdown)
			return;

		if (!m_aPendingPlayerCatchUps.Contains(playerId))
			m_aPendingPlayerCatchUps.Insert(playerId);

		if (m_bCatchUpQueued)
			return;

		m_bCatchUpQueued = true;
		GetGame().GetCallqueue().CallLater(ProcessPlayerCatchUps, 0);
	}

	protected void ProcessPlayerCatchUps()
	{
		m_bCatchUpQueued = false;
		if (m_bShutdown || !Replication.IsServer())
		{
			m_aPendingPlayerCatchUps.Clear();
			m_mPlayerCatchUpAttempts.Clear();
			return;
		}

		array<int> retry = {};
		PrintFormat("GC | Lifecycle | processing catch-up queue: count=%1", m_aPendingPlayerCatchUps.Count());
		foreach (int playerId : m_aPendingPlayerCatchUps)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller)
			{
				int attempts = m_mPlayerCatchUpAttempts.Get(playerId) + 1;
				if (attempts < PLAYER_CATCHUP_MAX_RETRIES)
				{
					m_mPlayerCatchUpAttempts.Set(playerId, attempts);
					retry.Insert(playerId);
				}
				else
				{
					m_mPlayerCatchUpAttempts.Remove(playerId);
					PrintFormat("GC | Lifecycle | player catch-up abandoned after retries: player=%1", playerId, level: LogLevel.WARNING);
				}
				continue;
			}

			m_mPlayerCatchUpAttempts.Remove(playerId);
			CatchUpPlayer(playerId);
		}

		m_aPendingPlayerCatchUps = retry;
		if (!m_aPendingPlayerCatchUps.IsEmpty())
		{
			m_bCatchUpQueued = true;
			GetGame().GetCallqueue().CallLater(ProcessPlayerCatchUps, PLAYER_CATCHUP_RETRY_MS);
		}
	}

	protected bool AssignSlotEntity(int playerId, GC_Slot slot)
	{
		if (!slot || !slot.GetEntity())
			return false;

		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller || !controller.SetLobbyControlledEntity(slot.GetEntity(), false))
			return false;

		IEntity spectator = m_mLobbySpectators.Get(playerId);
		if (!spectator)
		{
			// A briefing disconnect can reconnect after GAME has begun. Its old
			// spectator body was reserved by identity, not by the new session ID.
			UUID identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			spectator = m_mReservedSpectators.Get(identity);
			m_mReservedSpectators.Remove(identity);
		}
		if (spectator && spectator != slot.GetEntity())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spectator);
			m_mLobbySpectators.Remove(playerId);
		}

		slot.SetState(GC_SlotState.LOCKED);
		UpdateSlot(slot);
		return true;
	}

	protected bool SpawnSlotEntity(GC_Slot slot, bool force, SCR_AIGroup expectedGroup = null)
	{
		if (!Replication.IsServer() || !slot || slot.IsKIA())
			return false;

		if (slot.GetEntity())
			return true;

		SCR_AIGroup group = expectedGroup;
		if (!group)
			group = ResolveSlotGroup(slot);
		Resource prefab = Resource.Load(slot.GetResolvedPrefab());
		if (!group || !prefab)
		{
			PrintFormat("GC | Lifecycle | slot spawn failed: slot=%1 group=%2 prefab=%3 force=%4", slot.GetID(), group != null, slot.GetResolvedPrefab(), force, level: LogLevel.ERROR);
			return false;
		}

		GC_PlayerGroupSpawnTransform groupSpawnTransform = FindGroupSpawnTransform(group);
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector groupOriginDirect;
		vector groupAngles;
		if (groupSpawnTransform)
		{
			groupSpawnTransform.ApplyRotation(params);
			groupOriginDirect = groupSpawnTransform.GetOrigin();
			groupAngles = groupSpawnTransform.GetAngles();
		}
		else
		{
			group.GetWorldTransform(params.Transform);
			groupOriginDirect = group.GetOrigin();
			groupAngles = group.GetAngles();
			PrintFormat("GC | Formation | authored group transform unavailable; using current transform: slot=%1 group=%2", slot.GetID(), Replication.FindItemId(group), level: LogLevel.WARNING);
		}
		EnsureFormationCatalog();
		string formationName = ResolveGroupFormationName(group);
		vector offset = ResolveGroupMemberOffset(group, slot.GetGroupSlotIndex());
		// Formation offsets are local to the group. CoordToParent applies the
		// group's authored rotation before the world-space spawn position is set.
		bool isExactGroupOrigin = offset[0] == 0 && offset[1] == 0 && offset[2] == 0;
		vector formationWorldPosition;
		if (isExactGroupOrigin)
			formationWorldPosition = groupOriginDirect;
		else if (groupSpawnTransform)
			formationWorldPosition = groupSpawnTransform.TransformOffset(offset);
		else
			formationWorldPosition = group.CoordToParent(offset);
		params.Transform[3] = formationWorldPosition;
		vector terrainPosition = formationWorldPosition;
		bool snapToTerrain = group.GC_ShouldSnapMembersToTerrain();
		if (snapToTerrain)
			terrainPosition[1] = GetGame().GetWorld().GetSurfaceY(terrainPosition[0], terrainPosition[2]);
		params.Transform[3] = terrainPosition;

		bool debugThisSpawn = FORMATION_DEBUG_LOGGING && m_iFormationDebugLogCount < FORMATION_DEBUG_LOG_LIMIT;
		if (debugThisSpawn)
		{
			m_iFormationDebugLogCount++;
			PrintFormat("GC | Formation | debug anchor slot=%1 authoredOrigin=%2 currentGroupOrigin=%3 groupAngles=%4", slot.GetID(), groupOriginDirect, group.GetOrigin(), groupAngles);
			PrintFormat("GC | Formation | debug spawn slot=%1 index=%2 formation=%3 originMode=%4 snapToTerrain=%5 localOffset=%6 transformed=%7 final=%8", slot.GetID(), slot.GetGroupSlotIndex(), formationName, isExactGroupOrigin, snapToTerrain, offset, formationWorldPosition, terrainPosition);
		}

		IEntity entity = GetGame().SpawnEntityPrefab(prefab, null, params);
		if (!entity)
		{
			PrintFormat("GC | Lifecycle | slot spawn returned null: slot=%1 force=%2", slot.GetID(), force, level: LogLevel.ERROR);
			return false;
		}

		if (debugThisSpawn)
			PrintFormat("GC | Formation | debug spawned slot=%1 entityOrigin=%2 entityAngles=%3", slot.GetID(), entity.GetOrigin(), entity.GetAngles());

		Faction faction = group.GetFaction();
		if (faction)
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (affiliation)
				affiliation.SetAffiliatedFactionByKey(faction.GetFactionKey());
		}

		group.AddAIEntityToGroup(entity);
		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (aiControl)
		{
			aiControl.DeactivateAI();
			if (aiControl.IsAIActivated())
			{
				PrintFormat("GC | Lifecycle | playable slot spawn rejected because AI remained active: slot=%1 entity=%2", slot.GetID(), entity, level: LogLevel.ERROR);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				return false;
			}
		}

		slot.SetEntity(entity);
		if (slot.HasClaim())
			slot.SetState(GC_SlotState.LOCKED);
		else
			slot.SetState(GC_SlotState.FREE);
		UpdateSlot(slot);
		return true;
	}

	//! Releases lobby ownership and removes any staged character while the match
	//! has not started. GAME and POSTGAME entities are match records and must not
	//! be deleted by lobby slot changes or reservation expiry.
	protected void ReleaseSlotClaim(GC_Slot slot)
	{
		if (!slot)
			return;

		SCR_EGameModeState phase = GetLobbyPhase();
		bool deleteStagedEntity = phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING;
		IEntity stagedEntity = slot.GetEntity();
		if (deleteStagedEntity && stagedEntity)
			slot.ClearEntityReference(false);

		slot.ReleaseClaim();

		if (deleteStagedEntity && stagedEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(stagedEntity);
			PrintFormat("GC | Lifecycle | removed released pre-game slot entity: slot=%1 phase=%2", slot.GetID(), GetPhaseName(phase));
		}
	}

	//! Resolves the authored group for a slot without trusting a transient live
	//! pointer. The replicated RplId remains stable across the briefing spawn
	//! queue and is resolved through the roster's authoritative group index.
	protected SCR_AIGroup ResolveSlotGroup(GC_Slot slot)
	{
		if (!slot)
			return null;

		SCR_AIGroup group = slot.GetGroup();
		if (group && m_Roster.GetGroups().Contains(group))
			return group;

		RplId groupRplId = slot.GetGroupRplId();
		if (groupRplId != RplId.Invalid())
		{
			group = m_Roster.GetGroupByRplId(groupRplId);
			if (group)
			{
				if (FORMATION_DEBUG_LOGGING && slot.GetGroup() != group)
					PrintFormat("GC | Formation | repaired slot group slot=%1 rpl=%2 origin=%3 angles=%4", slot.GetID(), groupRplId, group.GetOrigin(), group.GetAngles());
				return group;
			}
		}

		return null;
	}

	protected GC_PlayerGroupSpawnTransform FindGroupSpawnTransform(SCR_AIGroup group)
	{
		foreach (GC_PlayerGroupSpawnTransform spawnTransform : m_aGroupSpawnTransforms)
		{
			if (spawnTransform && spawnTransform.IsFor(group))
				return spawnTransform;
		}

		return null;
	}

	protected void CacheGroupSpawnTransform(SCR_AIGroup group)
	{
		if (!group || FindGroupSpawnTransform(group))
			return;

		m_aGroupSpawnTransforms.Insert(new GC_PlayerGroupSpawnTransform(group));
	}

	protected void RemoveGroupSpawnTransform(SCR_AIGroup group)
	{
		for (int index = m_aGroupSpawnTransforms.Count() - 1; index >= 0; index--)
		{
			GC_PlayerGroupSpawnTransform spawnTransform = m_aGroupSpawnTransforms[index];
			if (spawnTransform && spawnTransform.IsFor(group))
				m_aGroupSpawnTransforms.Remove(index);
		}
	}

	protected void QueueBriefingSlotSpawns()
	{
		if (!Replication.IsServer())
			return;

		m_aPendingBriefingSpawns.Clear();
		foreach (GC_Slot slot : m_Roster.GetSlots())
		{
			if (slot && slot.HasClaim() && !slot.IsKIA() && !slot.GetEntity())
				m_aPendingBriefingSpawns.Insert(slot);
		}

		GetGame().GetCallqueue().Remove(ProcessBriefingSlotSpawns);
		if (!m_aPendingBriefingSpawns.IsEmpty())
		{
			m_bBriefingSpawnQueued = true;
			GetGame().GetCallqueue().CallLater(ProcessBriefingSlotSpawns, 0);
		}
	}

	protected void ProcessBriefingSlotSpawns()
	{
		if (m_bShutdown || GetLobbyPhase() != SCR_EGameModeState.BRIEFING)
		{
			m_aPendingBriefingSpawns.Clear();
			m_bBriefingSpawnQueued = false;
			return;
		}

		int count;
		while (count < BRIEFING_SPAWN_BATCH_SIZE && !m_aPendingBriefingSpawns.IsEmpty())
		{
			GC_Slot slot = m_aPendingBriefingSpawns[0];
			m_aPendingBriefingSpawns.Remove(0);
			if (slot && slot.HasClaim() && !slot.IsKIA() && !slot.GetEntity())
				SpawnSlotEntity(slot, false);
			count++;
		}

		if (!m_aPendingBriefingSpawns.IsEmpty())
			GetGame().GetCallqueue().CallLater(ProcessBriefingSlotSpawns, 0);
		else
			m_bBriefingSpawnQueued = false;
	}

	protected void ForceSpawnClaimedSlots()
	{
		foreach (GC_Slot slot : m_Roster.GetSlots())
		{
			if (slot && slot.HasClaim() && !slot.IsKIA() && !slot.GetEntity())
				SpawnSlotEntity(slot, true);
		}
	}

	protected void QueueCatchUpConnectedPlayers()
	{
		array<int> connectedPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
		foreach (int playerId : connectedPlayers)
			QueuePlayerCatchUp(playerId);
	}

	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);

		if (!Replication.IsServer())
			return;

		IEntity victim = instigatorContextData.GetVictimEntity();
		GC_Slot slot = m_Roster.GetSlotByEntity(victim);
		if (!slot)
			return;
		PrintFormat("GC | Killed From | event=authority-destroyed victim=%1 killer=%2 slot=%3 phase=%4", victim, instigatorContextData.GetKillerEntity(), slot.GetID(), GetLobbyPhase());

		if (GetLobbyPhase() == SCR_EGameModeState.GAME)
		{
			slot.MarkKIA();
			int playerId = slot.GetPlayerId();
			if (playerId > 0)
			{
				GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				if (playerController)
				{
					GC_KilledFromSnapshot killedFromSnapshot = new GC_KilledFromSnapshot();
					killedFromSnapshot.CaptureDeathFallback(victim, instigatorContextData.GetKillerEntity());
					PrintFormat("GC | Killed From | event=authority-fallback reason=controllable-destroyed player=%1 snapshot={%2}", playerId, killedFromSnapshot.GetDebugReport());
					playerController.SendKilledFromSnapshot(killedFromSnapshot);
				}
				SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
				SetSpectator(playerId, true);
			}
		}
		else
		{
			slot.ClearEntityReference(false);
			if (slot.HasClaim())
			{
				slot.SetState(GC_SlotState.RESERVED);
				if (GetLobbyPhase() == SCR_EGameModeState.BRIEFING)
					SpawnSlotEntity(slot, false);
			}
		}

		UpdateSlot(slot);
	}

	override void OnControllableDeleted(IEntity entity)
	{
		super.OnControllableDeleted(entity);

		if (!Replication.IsServer())
			return;

		GC_Slot slot = m_Roster.GetSlotByEntity(entity);
		if (!slot)
			return;
		PrintFormat("GC | Killed From | event=authority-deleted victim=%1 slot=%2 phase=%3", entity, slot.GetID(), GetLobbyPhase());

		if (GetLobbyPhase() == SCR_EGameModeState.GAME)
		{
			bool wasKIA = slot.IsKIA();
			int playerId = slot.GetPlayerId();
			if (!wasKIA && playerId > 0)
			{
				IEntity killer;
				SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
				if (damageManager)
				{
					Instigator instigator = damageManager.GetInstigator();
					if (instigator)
						killer = instigator.GetInstigatorEntity();
				}

				GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				if (playerController)
				{
					GC_KilledFromSnapshot killedFromSnapshot = new GC_KilledFromSnapshot();
					killedFromSnapshot.CaptureDeathFallback(entity, killer);
					PrintFormat("GC | Killed From | event=authority-fallback reason=controllable-deleted player=%1 killer=%2 snapshot={%3}", playerId, killer, killedFromSnapshot.GetDebugReport());
					playerController.SendKilledFromSnapshot(killedFromSnapshot);
				}
			}
			slot.ClearEntityReference(true);
			slot.MarkKIA();
		}
		else
		{
			slot.ClearEntityReference(false);
			if (slot.HasClaim())
			{
				slot.SetState(GC_SlotState.RESERVED);
				if (GetLobbyPhase() == SCR_EGameModeState.BRIEFING)
					SpawnSlotEntity(slot, false);
			}
		}

		UpdateSlot(slot);
	}

    void RegisterGroup(SCR_AIGroup group)
    {
        PrintFormat("GC | RegisterGroup: %1", group);
        if (!group)
            return;

		if (FORMATION_DEBUG_LOGGING)
			PrintFormat("GC | Formation | registered group rpl=%1 origin=%2 angles=%3 formation=%4 snapToTerrain=%5", Replication.FindItemId(group), group.GetOrigin(), group.GetAngles(), ResolveGroupFormationName(group), group.GC_ShouldSnapMembersToTerrain());
        
        bool wasEmpty;
        if (!m_Roster.AddGroup(group, wasEmpty))
            return;

		if (Replication.IsServer())
			CacheGroupSpawnTransform(group);

        SubscribeToGroupEvents(group);

        if (wasEmpty)
            SubscribeToGroupPlayerEvents();

        Faction faction = group.GetFaction();
        bool factionWasRegistered = faction && m_Roster.GetFactions().Contains(faction);
        if (faction)
            RegisterFaction(faction.GetFactionKey());

        if (Replication.IsServer())
        {
            RplId voiceGroupId = Replication.FindItemId(group);
            if (voiceGroupId != RplId.Invalid())
                m_VoiceState.GetFrequency(string.Format("group:%1", voiceGroupId));
            EnsureFormationCatalog();
            RegisterGroupMembers(group);
        }

        // RegisterFaction already notifies when this is a new faction. Notify
        // again only for an existing faction so dynamic faction/group views
        // can discover an additional group without duplicating the new-faction
        // callback.
        if (faction && factionWasRegistered)
            m_OnFactionTopologyChanged.Invoke(faction);
    }

    //! Removes a group from the manager's index and its lobby-only slot records.
    bool UnregisterGroup(SCR_AIGroup group)
    {
        if (!group)
            return false;

        if (!m_Roster.ContainsGroup(group))
            return false;

        Faction faction = group.GetFaction();
        UnsubscribeFromGroupEvents(group);
		RemoveGroupSpawnTransform(group);
        m_Roster.RemoveGroup(group);

        for (int i = m_Roster.GetSlots().Count() - 1; i >= 0; i--)
        {
            GC_Slot slot = m_Roster.GetSlots()[i];
            if (slot && slot.GetGroup() == group)
            {
                UnsubscribeFromSlotEvents(slot);
                m_Roster.RemoveSlot(slot);
            }
        }

        if (!m_Roster.HasGroups())
            UnsubscribeFromGroupPlayerEvents();

        if (faction)
            m_OnFactionTopologyChanged.Invoke(faction);

        return true;
    }

    protected void SubscribeToGroupEvents(SCR_AIGroup group)
    {
        if (!group)
            return;

        group.GetOnFactionChanged().Insert(OnGroupFactionChanged);
    }

    protected void SubscribeToGroupPlayerEvents()
    {
        SCR_AIGroup.GetOnPlayerAdded().Insert(OnGroupPlayerAdded);
        SCR_AIGroup.GetOnPlayerRemoved().Insert(OnGroupPlayerRemoved);
    }

    protected void UnsubscribeFromGroupEvents(SCR_AIGroup group)
    {
        if (!group)
            return;

        group.GetOnFactionChanged().Remove(OnGroupFactionChanged);
    }

    protected void UnsubscribeFromGroupPlayerEvents()
    {
        SCR_AIGroup.GetOnPlayerAdded().Remove(OnGroupPlayerAdded);
        SCR_AIGroup.GetOnPlayerRemoved().Remove(OnGroupPlayerRemoved);
    }

    protected void UnsubscribeFromAllGroupEvents()
    {
        foreach (SCR_AIGroup group : m_Roster.GetGroups())
            UnsubscribeFromGroupEvents(group);

        UnsubscribeFromGroupPlayerEvents();
    }

    protected void OnGroupFactionChanged(Faction faction)
    {
        if (faction)
        {
            m_Roster.AddFaction(faction);
            if (Replication.IsServer())
                m_VoiceState.GetFrequency("faction:" + faction.GetFactionKey());
        }

		// The base event does not identify the group or previous faction. One null
		// topology event asks consumers to reconcile from authoritative group state.
		m_OnFactionTopologyChanged.Invoke(null);
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			if (Replication.IsServer())
			{
				Faction currentFaction = GetPlayerFaction(playerId);
				GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
				if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.FACTION)
				{
					if (currentFaction)
						SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.FACTION, currentFaction.GetFactionKey());
					else
						SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
				}
				else if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.GROUP)
				{
					SCR_AIGroup selectedGroup = GetRegisteredGroupByRplId(assignment.GetGroupRplId());
					if (selectedGroup && currentFaction && selectedGroup.GetFaction() == currentFaction)
						SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, currentFaction.GetFactionKey(), assignment.GetGroupRplId());
					else
						SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
				}
				else if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.PERSONAL && !CanAccessPersonalVoiceChannel(playerId, assignment.GetPersonalOwnerIdentity()))
					SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
			}
			QueueAffiliationPresentation(playerId, GetPlayerGroup(playerId));
		}
    }

    protected void OnGroupPlayerAdded(SCR_AIGroup group, int playerId)
    {
		if (Replication.IsServer() && group && group.GetFaction())
		{
			if (GetLobbyPhase() == SCR_EGameModeState.BRIEFING)
				AssignPlayerToBriefingVoiceChannel(playerId);
			else
				SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, group.GetFaction().GetFactionKey(), Replication.FindItemId(group));
		}

		QueueAffiliationPresentation(playerId, group);
    }

    protected void OnGroupPlayerRemoved(SCR_AIGroup group, int playerId)
    {
        if (Replication.IsServer() && group)
        {
			GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
			if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.GROUP && assignment.GetGroupRplId() == Replication.FindItemId(group))
			{
				if (GetLobbyPhase() != SCR_EGameModeState.BRIEFING || !AssignPlayerToBriefingVoiceChannel(playerId))
					SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
			}
        }

		QueueAffiliationPresentation(playerId, group);
    }

    protected void NotifyGroupFactionUpdated(SCR_AIGroup group)
    {
        if (!group || !m_Roster.GetGroups().Contains(group))
            return;

        Faction faction = group.GetFaction();
        if (faction)
            m_OnFactionTopologyChanged.Invoke(faction);
    }

	protected void QueueAffiliationPresentation(int playerId, SCR_AIGroup group)
	{
		if (playerId > 0)
			m_sPendingAffiliationPlayers.Insert(playerId);
		if (group && group.GetFaction())
			m_sPendingFactionPresentation.Insert(group.GetFaction());

		GetGame().GetCallqueue().Remove(FlushAffiliationPresentation);
		GetGame().GetCallqueue().CallLater(FlushAffiliationPresentation, 0);
	}

	protected void FlushAffiliationPresentation()
	{
		foreach (int playerId : m_sPendingAffiliationPlayers)
		{
			RefreshPlayerRadio(playerId);
			m_OnPlayerAffiliationChanged.Invoke(playerId);
		}
		foreach (Faction faction : m_sPendingFactionPresentation)
			m_OnFactionPresentationChanged.Invoke(faction);

		m_sPendingAffiliationPlayers.Clear();
		m_sPendingFactionPresentation.Clear();
	}

    protected void SubscribeToSlotEvents(GC_Slot slot)
    {
        if (!slot)
            return;

        slot.m_OnChanged.Insert(OnSlotChanged);
    }

    protected void UnsubscribeFromSlotEvents(GC_Slot slot)
    {
        if (!slot)
            return;

        slot.m_OnChanged.Remove(OnSlotChanged);
    }

    protected void UnsubscribeFromAllSlotEvents()
    {
        foreach (GC_Slot slot : m_Roster.GetSlots())
            UnsubscribeFromSlotEvents(slot);
    }

    protected void OnSlotChanged(GC_Slot slot)
    {
        if (!slot || !m_Roster.GetSlots().Contains(slot))
            return;

        if (Replication.IsServer())
            UpdateSlot(slot);

        m_OnSlotPresentationChanged.Invoke(slot);
        SCR_AIGroup group = slot.GetGroup();
		QueueAffiliationPresentation(slot.GetPlayerId(), group);
    }

    protected void RegisterGroupMembers(SCR_AIGroup group)
    {
        if (!Replication.IsServer())
            return;

        if (!group || !group.m_aUnitPrefabSlots)
            return;
        
        int unitSlotCount = group.m_aUnitPrefabSlots.Count();
        array<GC_SlotVariantPolicy> variantPolicies = {};
        variantPolicies.Resize(unitSlotCount);
        for (int policyIndex = 0; policyIndex < unitSlotCount; policyIndex++)
            variantPolicies[policyIndex] = GC_SlotVariantPolicy.KEEP_VARIANT;

        GC_GroupSlotVariantConfigComponent variantConfig = GC_GroupSlotVariantConfigComponent.Cast(group.FindComponent(GC_GroupSlotVariantConfigComponent));
        if (variantConfig)
            variantConfig.ResolvePolicies(unitSlotCount, variantPolicies);

        for (int unitIndex = 0; unitIndex < unitSlotCount; unitIndex++)
        {
            ResourceName prefab = group.m_aUnitPrefabSlots[unitIndex];
            ResourceName resolvedPrefab = SCR_EditableEntityComponentClass.GetRandomVariant(prefab);
            if (resolvedPrefab == string.Empty)
                resolvedPrefab = prefab;

            GC_Slot slot = new GC_Slot();
            slot.SetID(m_Roster.AllocateSlotId());
            slot.SetGroupSlotIndex(unitIndex);
            slot.SetAuthoredPrefab(prefab);
            slot.SetResolvedPrefab(resolvedPrefab);
            slot.SetVariantPolicy(variantPolicies[unitIndex]);
            slot.SetGroup(group);
            
            m_Roster.AddSlot(slot);
            SubscribeToSlotEvents(slot);

            UpdateSlot(slot);
        }

        NotifyGroupFactionUpdated(group);
    }

    //! Serializes Lobby2's replicated snapshot. Save and load order must match.
    override protected bool RplSave(ScriptBitWriter writer)
    {
		// Faction-dependent authoring validation is intentionally deferred until
		// replication. Dedicated-server EOnInit can run before the faction manager
		// is ready, which previously cleared all parameter defaults permanently.
		if (!EnsureParameterCatalog())
		{
			Print("GC | JIP save failed: domain=parameters initialization", level: LogLevel.ERROR);
			return false;
		}

		array<SCR_AIGroup> groups = m_Roster.GetGroups();
        writer.WriteInt(groups.Count());
		PrintFormat("GC | JIP save begin: groups=%1 slots=%2", groups.Count(), m_Roster.GetSlots().Count());

        foreach (SCR_AIGroup group : groups)
        {
            RplId groupRplId = RplId.Invalid();
            if (group)
                groupRplId = Replication.FindItemId(group);

            writer.WriteRplId(groupRplId);
        }

		array<ref GC_Slot> slots = m_Roster.GetSlots();
        writer.WriteInt(slots.Count());
        foreach (GC_Slot slot : slots)
        {
            if (!slot || !slot.RplSave(writer))
                return false;
        }

		if (!m_VoiceState.Save(writer) || !m_BriefingCatalog.Save(writer) || !m_ParameterCatalog.Save(writer))
			return false;

		PrintFormat("GC | JIP save complete: groups=%1 slots=%2", groups.Count(), slots.Count());
        return true;
    }

    //! Deserializes Lobby2's snapshot from the authority.
    override protected bool RplLoad(ScriptBitReader reader)
    {
		if (!EnsureParameterCatalog())
		{
			Print("GC | JIP load failed: domain=parameters initialization", level: LogLevel.ERROR);
			return false;
		}

		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		m_Roster.ResetJipAttempts();

		int groupCount;
		if (!reader.ReadInt(groupCount) || groupCount < 0)
			return false;

		PrintFormat("GC | JIP load begin: serializedGroups=%1", groupCount);
		UnsubscribeFromAllGroupEvents();
		UnsubscribeFromAllSlotEvents();
		m_Roster.Clear();

		for (int groupIndex; groupIndex < groupCount; groupIndex++)
		{
			RplId groupRplId;
			if (!reader.ReadRplId(groupRplId))
				return false;

			if (groupRplId == RplId.Invalid())
			{
				PrintFormat("GC | JIP load group has invalid RplId: index=%1", groupIndex, level: LogLevel.WARNING);
				continue;
			}

			// Preserve the authority's serialized order independently of proxy
			// streaming timing. Ordered resolution below owns roster insertion.
			m_Roster.GetPendingGroupIds().Insert(groupRplId);
		}

		int slotCount;
		if (!reader.ReadInt(slotCount) || slotCount < 0)
			return false;

		PrintFormat("GC | JIP load slots: serializedSlots=%1", slotCount);
		for (int slotIndex; slotIndex < slotCount; slotIndex++)
		{
			GC_Slot slot = new GC_Slot();
			if (!slot.RplLoad(reader))
			{
				PrintFormat("GC | JIP load failed: domain=slots index=%1 count=%2", slotIndex, slotCount, level: LogLevel.ERROR);
				return false;
			}

			m_Roster.AddSlot(slot);
			SubscribeToSlotEvents(slot);
		}

		if (!m_VoiceState.Load(reader))
		{
			Print("GC | JIP load failed: domain=voice", level: LogLevel.ERROR);
			return false;
		}
		if (!m_BriefingCatalog.Load(reader))
		{
			Print("GC | JIP load failed: domain=briefing", level: LogLevel.ERROR);
			return false;
		}
		if (!m_ParameterCatalog.Load(reader))
		{
			Print("GC | JIP load failed: domain=parameters", level: LogLevel.ERROR);
			return false;
		}

		m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
		m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
		m_OnScenarioParametersChanged.Invoke(m_ParameterCatalog.GetRevision());
		ResolvePendingJipReferences();
		// The RplProp callback can run before the rest of the snapshot is available,
		// and PREGAME is a no-op on a fresh proxy. Publish one idempotent refresh
		// after every snapshot-owned domain has loaded so presentation can re-read
		// the complete replicated state.
		m_OnLobbyPhaseChanged.Invoke(GetLobbyPhase());
		return true;
	}

	protected bool ResolveJipGroup(RplId groupRplId)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(Replication.FindItem(groupRplId));
		if (!group)
			return false;

		bool wasEmpty;
		if (m_Roster.AddGroup(group, wasEmpty))
		{
			SubscribeToGroupEvents(group);
			if (wasEmpty)
				SubscribeToGroupPlayerEvents();
		}

		Faction faction = group.GetFaction();
		if (!faction)
			return false;

		RegisterFaction(faction.GetFactionKey());
		return true;
	}

	protected void ResolvePendingJipReferences()
	{
		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		int attempt = m_Roster.IncrementJipAttempt();
		array<RplId> pendingGroups = m_Roster.GetPendingGroupIds();

		// Resolve only the available prefix. Allowing later proxies to register
		// ahead of an earlier unresolved proxy would make client faction/group
		// presentation order depend on streaming timing instead of server order.
		while (!pendingGroups.IsEmpty())
		{
			if (!ResolveJipGroup(pendingGroups[0]))
				break;

			pendingGroups.RemoveOrdered(0);
		}

		int pendingSlotGroups;
		int pendingSlotEntities;
		foreach (GC_Slot slot : m_Roster.GetSlots())
		{
			if (!slot)
				continue;

			if (!slot.GetGroup())
			{
				RplId slotGroupRplId = slot.GetGroupRplId();
				if (slotGroupRplId == RplId.Invalid())
				{
					PrintFormat("GC | JIP slot has invalid group RplId: slot=%1", slot.GetID(), level: LogLevel.WARNING);
				}
				else if (!slot.ResolveReplicatedGroup())
				{
					pendingSlotGroups++;
				}
			}

			if (slot.GetEntityRplId() != RplId.Invalid() && !slot.ResolveReplicatedEntity())
				pendingSlotEntities++;
		}

		if (!pendingGroups.IsEmpty() || pendingSlotGroups > 0 || pendingSlotEntities > 0)
		{
			GetGame().GetCallqueue().CallLater(ResolvePendingJipReferences, JIP_REFERENCE_RETRY_MS);
			return;
		}

		PrintFormat("GC | JIP topology ready: attempts=%1 groups=%2 factions=%3 slots=%4", attempt,
			m_Roster.GetGroups().Count(), m_Roster.GetFactions().Count(), m_Roster.GetSlots().Count());
		m_OnRegistryReset.Invoke(attempt);
	}

    bool RegisterFaction(FactionKey factionKey)
    {
		if (factionKey.IsEmpty())
			return false;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return false;

		Faction faction = factionManager.GetFactionByKey(factionKey);
		if (!m_Roster.AddFaction(faction))
			return false;

		if (Replication.IsServer())
			m_VoiceState.GetFrequency("faction:" + factionKey);
		m_OnFactionTopologyChanged.Invoke(faction);
		return true;
    }

    bool UnregisterFaction(FactionKey factionKey)
    {
		Faction faction = m_Roster.GetFaction(factionKey);
		if (!m_Roster.RemoveFaction(faction))
			return false;

		m_OnFactionTopologyChanged.Invoke(faction);
		return true;
    }

    Faction GetRegisteredFaction(FactionKey factionKey) { return m_Roster.GetFaction(factionKey); }

    void NotifyFactionUpdated(Faction faction)
    {
		if (m_Roster.ContainsFaction(faction))
			m_OnFactionPresentationChanged.Invoke(faction);
    }

    array<Faction> GetRegisteredFactions() { return m_Roster.GetFactions(); }
    array<SCR_AIGroup> GetRegisteredGroups() { return m_Roster.GetGroups(); }
    array<ref GC_Slot> GetSlots() { return m_Roster.GetSlots(); }
    array<SCR_AIGroup> GetGroupsForFaction(FactionKey factionKey) { return m_Roster.GetGroupsForFaction(factionKey); }
    int GetFactionPlayerCount(FactionKey factionKey) { return m_Roster.GetFactionPlayerCount(factionKey); }
    int GetGroupPlayerCount(SCR_AIGroup group) { return m_Roster.GetGroupPlayerCount(group); }
    array<ref GC_Slot> GetSlotsForGroup(SCR_AIGroup group) { return m_Roster.GetSlotsForGroup(group); }
    int GetFactionSlotCount(FactionKey factionKey) { return m_Roster.GetFactionSlotCount(factionKey); }
    GC_Slot GetSlot(int id) { return m_Roster.GetSlot(id); }
    GC_Slot GetSlotByPlayerId(int playerId) { return m_Roster.GetSlotByPlayerId(playerId); }
	GC_Slot GetSlotByUUID(UUID identity) { return m_Roster.GetSlotByUUID(identity); }
	GC_Slot GetSlotByEntity(IEntity entity) { return m_Roster.GetSlotByEntity(entity); }
    SCR_AIGroup GetRegisteredGroupByRplId(RplId groupRplId) { return m_Roster.GetGroupByRplId(groupRplId); }
    SCR_AIGroup GetPlayerGroup(int playerId) { return m_Roster.GetPlayerGroup(playerId); }
	Faction GetPlayerFaction(int playerId) { return m_Roster.GetPlayerFaction(playerId); }

	//! Publishes a role-change prompt from server authority. The transported flag
	//! is used only to wait for each client's PlayerManager view to converge;
	//! consumers always query SCR_Global.IsAdmin when applying presentation.
	void PublishPlayerRoleChanged(int playerId, EPlayerRole roleFlags)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		bool expectedAdmin = SCR_Global.IsAdminRole(roleFlags);
		if (!expectedAdmin)
		{
			GC_LobbyPlayerController voiceController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (voiceController)
				voiceController.RevokeLobbyAdminVoice();
		}
		if (GC_UIEventDiagnostics.ENABLED)
			PrintFormat("GC | Admin Trace | role publish: player=%1 expectedAdmin=%2", playerId, expectedAdmin);

		QueuePlayerRolePresentation(playerId, expectedAdmin);
		Rpc(RpcDo_PlayerRoleChanged, playerId, expectedAdmin);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayerRoleChanged(int playerId, bool expectedAdmin)
	{
		if (Replication.IsServer())
			return;

		if (GC_UIEventDiagnostics.ENABLED)
			PrintFormat("GC | Admin Trace | role broadcast received: player=%1 expectedAdmin=%2", playerId, expectedAdmin);
		QueuePlayerRolePresentation(playerId, expectedAdmin);
	}

	protected void QueuePlayerRolePresentation(int playerId, bool expectedAdmin)
	{
		m_mPendingRoleAdmin.Set(playerId, expectedAdmin);
		m_mPendingRoleAttempts.Set(playerId, 0);
		GetGame().GetCallqueue().Remove(ReconcilePendingPlayerRoles);
		GetGame().GetCallqueue().CallLater(ReconcilePendingPlayerRoles, 0);
	}

	protected void ReconcilePendingPlayerRoles()
	{
		GetGame().GetCallqueue().Remove(ReconcilePendingPlayerRoles);
		array<int> completed = {};
		bool retry;
		foreach (int playerId, bool expectedAdmin : m_mPendingRoleAdmin)
		{
			int attempts;
			m_mPendingRoleAttempts.Find(playerId, attempts);
			bool queriedAdmin = SCR_Global.IsAdmin(playerId);
			if (queriedAdmin == expectedAdmin || attempts >= ROLE_PRESENTATION_MAX_RETRIES)
			{
				if (GC_UIEventDiagnostics.ENABLED && attempts >= ROLE_PRESENTATION_MAX_RETRIES && queriedAdmin != expectedAdmin)
					PrintFormat("GC | Admin Trace | role convergence timed out: player=%1 expectedAdmin=%2 queriedAdmin=%3", playerId, expectedAdmin, queriedAdmin, level: LogLevel.WARNING);
				else if (GC_UIEventDiagnostics.ENABLED)
					PrintFormat("GC | Admin Trace | role converged: player=%1 admin=%2 attempts=%3", playerId, queriedAdmin, attempts);

				m_OnPlayerRoleChanged.Invoke(playerId);
				completed.Insert(playerId);
				continue;
			}

			m_mPendingRoleAttempts.Set(playerId, attempts + 1);
			retry = true;
		}

		foreach (int completedPlayerId : completed)
		{
			m_mPendingRoleAdmin.Remove(completedPlayerId);
			m_mPendingRoleAttempts.Remove(completedPlayerId);
		}

		if (retry)
			GetGame().GetCallqueue().CallLater(ReconcilePendingPlayerRoles, ROLE_PRESENTATION_RETRY_MS);
	}

	GC_VoiceChannelAssignment GetPlayerVoiceAssignment(int playerId) { return m_VoiceState.Get(playerId); }

	//! Resolve transport from accepted assignments and live base-game affiliation.
	void RefreshPlayerRadio(int playerId)
	{
		if (!Replication.IsServer() || m_bShutdown)
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return;
		int chatFrequency = GC_LobbyRadio.GLOBAL_FREQUENCY;
		int factionFrequency = GC_LobbyRadio.INACTIVE_FREQUENCY;
		Faction faction = GetPlayerFaction(playerId);
		if (faction)
			factionFrequency = m_VoiceState.GetFrequency("faction:" + faction.GetFactionKey());
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (assignment)
		{
			switch (assignment.GetChannelType())
			{
				case GC_VoiceChannelType.FACTION:
					chatFrequency = factionFrequency;
					break;
				case GC_VoiceChannelType.GROUP:
					chatFrequency = m_VoiceState.GetFrequency(string.Format("group:%1", assignment.GetGroupRplId()));
					break;
				case GC_VoiceChannelType.PERSONAL:
					chatFrequency = m_VoiceState.GetFrequency("personal:" + assignment.GetPersonalOwnerIdentity());
					break;
			}
		}
		controller.SetLobbyRadioChannels(chatFrequency, factionFrequency, IsPlayerVoiceDeafened(playerId));
	}
	UUID GetPlayerVoiceIdentity(int playerId) { return m_VoiceState.GetPlayerIdentity(playerId); }
	bool IsPlayerVoiceDeafened(int playerId) { return m_VoiceState.IsDeafened(playerId); }

	bool TrySetPlayerVoiceDeafened(int playerId, bool deafened)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment assignment;
		if (!m_VoiceState.SetDeafened(playerId, deafened, assignment))
			return assignment != null;

		PrintFormat("GC | Voice deafen state changed: player=%1 deafened=%2", playerId, deafened);
		RefreshPlayerRadio(playerId);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnVoiceAssignmentChanged.Invoke(playerId);
		return true;
	}

	bool TrySetPlayerVoiceChannel(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		switch (channelType)
		{
			case GC_VoiceChannelType.GLOBAL:
				if (GetLobbyPhase() == SCR_EGameModeState.BRIEFING && GetPlayerFaction(playerId))
					return false;
				return SetPlayerVoiceChannel(playerId, channelType);
			case GC_VoiceChannelType.FACTION:
			{
				Faction requestedFaction = GetRegisteredFaction(factionKey);
				if (!requestedFaction || GetPlayerFaction(playerId) != requestedFaction)
					return false;
				return SetPlayerVoiceChannel(playerId, channelType, factionKey);
			}
			case GC_VoiceChannelType.GROUP:
				return TryJoinGroupFromVoiceHeader(playerId, groupRplId);
			case GC_VoiceChannelType.PERSONAL:
				if (!CanAccessPersonalVoiceChannel(playerId, personalOwnerIdentity))
					return false;
				return SetPlayerVoiceChannel(playerId, channelType, "", RplId.Invalid(), personalOwnerIdentity);
		}

		return false;
	}

	//! Applies the configured briefing policy using authoritative base-game
	//! faction/group membership. HQ is the safe fallback if a group is not yet
	//! replicated, so a faction-assigned player never remains in Global.
	protected bool AssignPlayerToBriefingVoiceChannel(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		Faction faction = GetPlayerFaction(playerId);
		if (!faction)
			return false;

		if (m_eBriefingDefaultVoiceChannel == GC_EBriefingVoiceChannel.GROUP)
		{
			SCR_AIGroup group = GetPlayerGroup(playerId);
			if (group && group.GetFaction() == faction)
			{
				RplId groupRplId = Replication.FindItemId(group);
				if (groupRplId != RplId.Invalid())
					return SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, faction.GetFactionKey(), groupRplId);
			}

			PrintFormat("GC | Voice briefing group unavailable; using faction HQ: player=%1 faction=%2", playerId, faction.GetFactionKey(), level: LogLevel.WARNING);
		}

		return SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.FACTION, faction.GetFactionKey());
	}

	//! Reconciles only implicit/explicit Global assignments. Same-faction
	//! personal channels remain valid and deliberate player selections.
	protected void ReconcileBriefingVoiceAssignments()
	{
		array<int> connectedPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
		foreach (int playerId : connectedPlayers)
		{
			GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
			if (assignment && assignment.GetChannelType() != GC_VoiceChannelType.GLOBAL)
				continue;

			AssignPlayerToBriefingVoiceChannel(playerId);
		}
	}

	protected bool SetPlayerVoiceChannel(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment previousAssignment = m_VoiceState.Get(playerId);
		UUID playerIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		bool wasOpenPersonalOwner = previousAssignment && previousAssignment.GetChannelType() == GC_VoiceChannelType.PERSONAL && previousAssignment.GetPersonalOwnerIdentity() == playerIdentity;

		GC_VoiceChannelAssignment assignment;
		if (!m_VoiceState.SetChannel(playerId, channelType, factionKey, groupRplId, personalOwnerIdentity, assignment))
			return assignment != null;

		PrintFormat("GC | Voice assignment changed: player=%1 type=%2 faction=%3 group=%4 personalOwner=%5", playerId, channelType, factionKey, groupRplId, personalOwnerIdentity);
		RefreshPlayerRadio(playerId);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnVoiceAssignmentChanged.Invoke(playerId);

		if (wasOpenPersonalOwner && (channelType != GC_VoiceChannelType.PERSONAL || personalOwnerIdentity != playerIdentity))
			ClosePersonalVoiceChannel(playerIdentity);
		return true;
	}

	//! The owner always sees their channel. Other players may discover and join
	//! it only while the owner is inside it and both players share the same
	//! authoritative faction scope (or both are unassigned spectators).
	bool CanAccessPersonalVoiceChannel(int playerId, UUID ownerIdentity)
	{
		if (playerId <= 0 || ownerIdentity.IsEmpty() || !m_VoiceState.HasPersonalChannel(ownerIdentity))
			return false;

		int ownerPlayerId = GetConnectedPlayerIdByIdentity(ownerIdentity);
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId) || ownerPlayerId <= 0)
			return false;
		if (m_VoiceState.GetPlayerIdentity(playerId) == ownerIdentity)
			return true;

		GC_VoiceChannelAssignment ownerAssignment = m_VoiceState.Get(ownerPlayerId);
		if (!ownerAssignment || ownerAssignment.GetChannelType() != GC_VoiceChannelType.PERSONAL || ownerAssignment.GetPersonalOwnerIdentity() != ownerIdentity)
			return false;

		Faction playerFaction = GetPlayerFaction(playerId);
		Faction ownerFaction = GetPersonalVoiceChannelFaction(ownerIdentity);
		if (!playerFaction || !ownerFaction)
			return !playerFaction && !ownerFaction;

		return playerFaction == ownerFaction;
	}

	//! A personal channel follows its owner's authoritative faction. Connected
	//! base-group membership wins; a UUID-reserved lobby slot preserves the same
	//! faction while the player is reconnecting.
	Faction GetPersonalVoiceChannelFaction(UUID ownerIdentity)
	{
		int ownerPlayerId = GetConnectedPlayerIdByIdentity(ownerIdentity);
		if (ownerPlayerId > 0)
		{
			Faction playerFaction = GetPlayerFaction(ownerPlayerId);
			if (playerFaction)
				return playerFaction;
		}

		GC_Slot ownerSlot = GetSlotByUUID(ownerIdentity);
		if (ownerSlot && ownerSlot.GetGroup())
			return ownerSlot.GetGroup().GetFaction();

		return null;
	}

	int GetConnectedPlayerIdByIdentity(UUID identity)
	{
		if (identity.IsEmpty())
			return -1;

		int playerId = m_VoiceState.GetPlayerIdByIdentity(identity);
		if (playerId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return playerId;

		return -1;
	}

	protected void RegisterPlayerPersonalVoiceChannel(int playerId, UUID identity)
	{
		if (!Replication.IsServer() || playerId <= 0 || identity.IsEmpty())
			return;

		bool changed;
		GC_VoiceChannelAssignment identityAssignment;
		if (m_VoiceState.SetPlayerIdentity(playerId, identity, identityAssignment))
		{
			Rpc(RpcDo_UpdateVoiceAssignment, identityAssignment);
			changed = true;
		}

		if (m_VoiceState.RegisterPersonalChannel(identity))
		{
			PrintFormat("GC | Voice personal channel registered: player=%1 identity=%2", playerId, identity);
			Rpc(RpcDo_RegisterPersonalVoiceChannel, identity);
			changed = true;
		}

		if (changed)
			m_OnVoiceTopologyChanged.Invoke(playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RegisterPersonalVoiceChannel(string ownerIdentity)
	{
		if (Replication.IsServer() || !m_VoiceState.RegisterPersonalChannel(ownerIdentity))
			return;

		m_OnVoiceTopologyChanged.Invoke(GetConnectedPlayerIdByIdentity(ownerIdentity));
	}

	protected void ClosePersonalVoiceChannel(UUID ownerIdentity)
	{
		array<int> members = m_VoiceState.GetPersonalChannelMembers(ownerIdentity);
		foreach (int memberPlayerId : members)
		{
			if (m_VoiceState.GetPlayerIdentity(memberPlayerId) != ownerIdentity)
				SetPlayerVoiceChannel(memberPlayerId, GC_VoiceChannelType.GLOBAL);
		}
	}

	protected void RemovePlayerVoiceAssignment(int playerId)
	{
		if (!Replication.IsServer())
			return;

		ClosePersonalVoiceChannel(m_VoiceState.GetPlayerIdentity(playerId));
		if (!m_VoiceState.Remove(playerId))
			return;

		Rpc(RpcDo_RemoveVoiceAssignment, playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateVoiceAssignment(GC_VoiceChannelAssignment updateAssignment)
	{
		if (Replication.IsServer() || !m_VoiceState.Apply(updateAssignment))
			return;

		m_OnVoiceAssignmentChanged.Invoke(updateAssignment.GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RemoveVoiceAssignment(int playerId)
	{
		if (Replication.IsServer())
			return;

		// Player disconnection is the sole presentation event for this removal.
		// This RPC only converges the client-owned voice model.
		m_VoiceState.Remove(playerId);
	}

    //! Returns one briefing by its one-based authored order/index.
	GC_Briefing GetBriefing(int briefingOrder)
	{
		return m_BriefingCatalog.GetBriefing(briefingOrder);
	}

	GC_LobbyParameterCatalog GetScenarioParameterCatalog()
	{
		EnsureParameterCatalog();
		return m_ParameterCatalog;
	}

	array<ref GC_LobbyParameterDefinition> GetScenarioParameterDefinitions()
	{
		if (!GetScenarioParameterCatalog())
			return {};

		return m_ParameterCatalog.GetDefinitions();
	}

	array<ref GC_LobbyParameterValue> GetScenarioParameterValues()
	{
		if (!GetScenarioParameterCatalog())
			return {};

		return m_ParameterCatalog.GetValues();
	}

	int GetScenarioParameterRevision()
	{
		if (!GetScenarioParameterCatalog())
			return 0;

		return m_ParameterCatalog.GetRevision();
	}

	bool HasScenarioParameters()
	{
		return GetScenarioParameterCatalog() && m_ParameterCatalog.HasDefinitions();
	}

	bool IsScenarioParameterCatalogValid()
	{
		return GetScenarioParameterCatalog() && m_ParameterCatalog.IsValid();
	}

	//! Returns whether this admin may change one definition. Visibility is a
	//! separate presentation concern and is intentionally not checked here.
	bool CanModifyScenarioParameter(int playerId, int definitionIndex)
	{
		if (!IsScenarioParameterCatalogValid() || !SCR_Global.IsAdmin(playerId))
			return false;

		GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(definitionIndex);
		if (!definition)
			return false;

		string ownerFactionKey = definition.GetOwnerFactionKey();
		Faction playerFaction = GetPlayerFaction(playerId);
		if (!playerFaction)
			return true;

		return playerFaction.GetFactionKey() == ownerFactionKey;
	}

	//! Applies one complete client draft atomically on the server. The player id
	//! is derived by GC_LobbyPlayerController from the owning RPC endpoint.
	bool TryApplyScenarioParameters(int playerId, int baseRevision, array<ref GC_LobbyParameterValue> values)
	{
		if (m_bShutdown || !Replication.IsServer())
		{
			Print("GC | Parameters | request rejected: manager inactive or caller is not authority", LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-inactive-or-not-authority");
			return false;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerId <= 0 || !playerManager || !playerManager.IsPlayerConnected(playerId))
		{
			PrintFormat("GC | Parameters | request rejected: invalid player=%1", playerId, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-not-logged-in");
			return false;
		}

		if (!SCR_Global.IsAdmin(playerId))
		{
			if (GC_UIEventDiagnostics.ENABLED)
				PrintFormat("GC | Admin Trace | server action rejected: action=apply-parameters player=%1 queriedAdmin=false", playerId, level: LogLevel.WARNING);
			PrintFormat("GC | Parameters | request rejected: player=%1 is not an admin", playerId, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-not-admin");
			return false;
		}

		SCR_EGameModeState phase = GetLobbyPhase();
		if (phase != SCR_EGameModeState.PREGAME && phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
		{
			PrintFormat("GC | Parameters | request rejected: parameters locked in lobby=%1", GetPhaseName(phase), level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-phase-locked");
			return false;
		}

		if (!IsScenarioParameterCatalogValid() || baseRevision != GetScenarioParameterRevision())
		{
			PrintFormat("GC | Parameters | request rejected: stale or invalid catalog player=%1 baseRevision=%2 currentRevision=%3", playerId, baseRevision, GetScenarioParameterRevision(), level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-stale-catalog");
			return false;
		}

		if (!m_ParameterCatalog.ValidateValues(values))
		{
			PrintFormat("GC | Parameters | request rejected: invalid complete batch player=%1", playerId, level: LogLevel.WARNING);
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-invalid-values");
			return false;
		}

		foreach (GC_LobbyParameterValue value : values)
		{
			GC_LobbyParameterValue currentValue = m_ParameterCatalog.FindValue(value.GetDefinitionIndex());
			if (!currentValue || GC_LobbyParameterCatalog.AreValuesEqual(currentValue, value))
				continue;

			if (!CanModifyScenarioParameter(playerId, value.GetDefinitionIndex()))
			{
				GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(value.GetDefinitionIndex());
				string escapedLabel = definition.GetLabel();
				escapedLabel.Replace("\\", "\\\\");
				escapedLabel.Replace("\"", "\\\"");
				escapedLabel.Replace("\n", "\\n");
				escapedLabel.Replace("\r", "\\r");
				PrintFormat("GC | Parameters | request rejected: owner mismatch player=%1 index=%2 label=\"%3\"", playerId, value.GetDefinitionIndex(), escapedLabel, level: LogLevel.WARNING);
				GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-owner-mismatch");
				return false;
			}
		}

		if (!m_ParameterCatalog.ApplyValues(values))
		{
			GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "rejected-apply-failed");
			return false;
		}

		int revision = m_ParameterCatalog.GetRevision();
		m_OnScenarioParametersChanged.Invoke(revision);
		PrintScenarioParameterValues(playerId);
		Rpc(RpcDo_SetScenarioParameters, revision, m_ParameterCatalog.GetValues());
		GC_AdminActionLogger.Log(playerId, "apply-parameters", "scenario-parameters", "succeeded");
		return true;
	}

	protected void PrintScenarioParameterValues(int playerId)
	{
		foreach (GC_LobbyParameterValue value : m_ParameterCatalog.GetValues())
		{
			if (!value)
				continue;

			string normalized = value.GetNormalizedValue();
			normalized.Replace("\\", "\\\\");
			normalized.Replace("\"", "\\\"");
			normalized.Replace("\n", "\\n");
			normalized.Replace("\r", "\\r");
			GC_LobbyParameterDefinition definition = m_ParameterCatalog.FindDefinition(value.GetDefinitionIndex());
			string escapedLabel = definition.GetLabel();
			escapedLabel.Replace("\\", "\\\\");
			escapedLabel.Replace("\"", "\\\"");
			escapedLabel.Replace("\n", "\\n");
			escapedLabel.Replace("\r", "\\r");
			PrintFormat("GC | Parameters | player=%1 revision=%2 index=%3 label=\"%4\" value=\"%5\"", playerId, m_ParameterCatalog.GetRevision(), value.GetDefinitionIndex(), escapedLabel, normalized);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetScenarioParameters(int revision, array<ref GC_LobbyParameterValue> values)
	{
		if (Replication.IsServer() || !m_ParameterCatalog)
			return;

		if (m_ParameterCatalog.ApplySnapshot(revision, values))
			m_OnScenarioParametersChanged.Invoke(revision);
	}

	array<ref GC_Briefing> GetBriefings()
    {
        return m_BriefingCatalog.GetBriefings();
    }

    array<ref GC_Briefing> GetBriefingsForPlayer(int playerId)
    {
        return m_BriefingCatalog.GetBriefingsFor(GetPlayerFaction(playerId), GetLobbyPhase());
    }

    bool UpsertBriefing(GC_Briefing update)
    {
        if (!Replication.IsServer() || !ValidateBriefing(update))
            return false;

        GC_Briefing briefing;
        if (!m_BriefingCatalog.UpsertBriefing(update, briefing))
            return false;

        m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
        Rpc(RpcDo_UpsertBriefing, briefing);
        return true;
    }

    bool SetBriefing(int order, string title, string content, array<string> factionKeys = null, SCR_EGameModeState showAtGameModeState = SCR_EGameModeState.GAME, bool invertGameModeState = false, bool visible = true)
    {
        GC_Briefing update = new GC_Briefing();
        update.Set(order, title, content, factionKeys, showAtGameModeState, invertGameModeState, visible);
        return UpsertBriefing(update);
    }

    bool SetBriefingVisible(int briefingOrder, bool visible)
    {
        if (!Replication.IsServer())
            return false;

        GC_Briefing briefing = GetBriefing(briefingOrder);
        if (!briefing || briefing.IsVisible() == visible)
            return briefing != null;

        return SetBriefing(briefing.GetOrder(), briefing.GetTitle(), briefing.GetContent(),
            briefing.GetFactionKeys(), briefing.GetShowAtGameModeState(),
            briefing.IsGameModeStateInverted(), visible);
    }

    bool RemoveBriefing(int briefingOrder)
    {
        if (!Replication.IsServer() || briefingOrder <= 0)
            return false;

        int revision;
        if (!m_BriefingCatalog.RemoveBriefing(briefingOrder, revision))
            return false;

        m_OnBriefingsChanged.Invoke(revision);
        Rpc(RpcDo_RemoveBriefing, briefingOrder, revision);
        return true;
    }

    protected bool ValidateBriefing(GC_Briefing briefing)
    {
        if (!briefing || briefing.GetOrder() <= 0 || briefing.GetOrder() > GC_LobbyBriefingCatalog.MAX_BRIEFINGS)
            return false;

        if (briefing.GetTitle().Length() > GC_LobbyBriefingCatalog.MAX_BRIEFING_TITLE_LENGTH
            || briefing.GetContent().Length() > GC_LobbyBriefingCatalog.MAX_BRIEFING_CONTENT_LENGTH
            || !GC_LobbyPhase.IsValid(briefing.GetShowAtGameModeState()))
            return false;

        return ValidateFactionKeys(briefing.GetFactionKeys(), GC_Briefing.MAX_FACTION_KEYS);
    }

    protected bool ValidateFactionKeys(array<string> factionKeys, int maximumCount)
    {
        if (!factionKeys || factionKeys.Count() > maximumCount)
            return false;

        if (factionKeys.IsEmpty())
            return true;

        FactionManager factionManager = GetGame().GetFactionManager();
        if (!factionManager)
            return false;

        array<string> validatedFactionKeys = {};
        foreach (string factionKey : factionKeys)
        {
            if (factionKey.IsEmpty() || validatedFactionKeys.Contains(factionKey)
                || !factionManager.GetFactionByKey(factionKey))
                return false;

            validatedFactionKeys.Insert(factionKey);
        }

        return true;
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_UpsertBriefing(GC_Briefing update)
    {
        if (Replication.IsServer() || !m_BriefingCatalog.ApplyBriefing(update))
            return;

        m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_RemoveBriefing(int briefingOrder, int revision)
    {
        if (Replication.IsServer() || !m_BriefingCatalog.ApplyBriefingRemoval(briefingOrder, revision))
            return;

        m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
    }

    bool RegisterBriefingMarker(GC_BriefingMarkerComponent component)
    {
        if (!Replication.IsServer() || !component)
            return false;

        GC_BriefingMarker marker = component.CreateMarkerSnapshot();
        if (UpsertBriefingMarker(marker))
            return true;

        string markerName;
        if (marker)
            markerName = marker.GetName();

        PrintFormat("GC | Briefing marker component registration rejected: marker=%1", markerName, level: LogLevel.WARNING);
        return false;
    }

    GC_BriefingMarker GetBriefingMarker(string markerName)
    {
        return m_BriefingCatalog.GetMarker(markerName);
    }

    array<ref GC_BriefingMarker> GetBriefingMarkers()
    {
        return m_BriefingCatalog.GetMarkers();
    }

    array<ref GC_BriefingMarker> GetBriefingMarkersForPlayer(int playerId)
    {
        return m_BriefingCatalog.GetMarkersFor(GetPlayerFaction(playerId), GetLobbyPhase());
    }

    bool UpsertBriefingMarker(GC_BriefingMarker update)
    {
        if (!Replication.IsServer() || !ValidateBriefingMarker(update))
            return false;

        GC_BriefingMarker marker;
        if (!m_BriefingCatalog.UpsertMarker(update, marker))
            return false;

        m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
        Rpc(RpcDo_UpsertBriefingMarker, marker);
        return true;
    }

    bool SetBriefingMarker(string markerName, ResourceName image, ResourceName highlightImage, string quadName, vector worldPosition,
        float rotation, bool worldScale, float size, float minSize, string description, array<string> factionKeys = null,
        SCR_EGameModeState showAtGameModeState = SCR_EGameModeState.GAME,
        bool invertGameModeState = false, bool visible = true, int color = Color.WHITE)
    {
        GC_BriefingMarker update = new GC_BriefingMarker();
        update.Set(markerName, image, highlightImage, quadName, worldPosition, rotation, worldScale, size, minSize,
            description, factionKeys, showAtGameModeState, invertGameModeState, visible, color);
        return UpsertBriefingMarker(update);
    }

    bool SetBriefingMarkerVisible(string markerName, bool visible)
    {
        if (!Replication.IsServer())
            return false;

        GC_BriefingMarker marker = GetBriefingMarker(markerName);
        if (!marker || marker.IsVisible() == visible)
            return marker != null;

        return SetBriefingMarker(marker.GetName(), marker.GetImage(), marker.GetHighlightImage(), marker.GetQuadName(),
            marker.GetWorldPosition(), marker.GetRotation(), marker.IsWorldScale(), marker.GetSize(),
            marker.GetMinSize(), marker.GetDescription(), marker.GetFactionKeys(), marker.GetShowAtGameModeState(),
            marker.IsGameModeStateInverted(), visible, marker.GetColor());
    }

    bool RemoveBriefingMarker(string markerName)
    {
        if (!Replication.IsServer() || markerName.IsEmpty())
            return false;

        int revision;
        if (!m_BriefingCatalog.RemoveMarker(markerName, revision))
            return false;

        m_OnBriefingMarkersChanged.Invoke(revision);
        Rpc(RpcDo_RemoveBriefingMarker, markerName, revision);
        return true;
    }

    protected bool ValidateBriefingMarker(GC_BriefingMarker marker)
    {
        if (!marker || marker.GetName().IsEmpty()
            || marker.GetName().Length() > GC_LobbyBriefingCatalog.MAX_MARKER_NAME_LENGTH
            || marker.GetImage().IsEmpty()
            || marker.GetDescription().Length() > GC_LobbyBriefingCatalog.MAX_MARKER_DESCRIPTION_LENGTH
            || marker.GetSize() <= 0
			|| marker.GetMinSize() < 0
            || !GC_LobbyPhase.IsValid(marker.GetShowAtGameModeState()))
            return false;

        if ((marker.GetImage().EndsWith(".imageset") || !marker.GetHighlightImage().IsEmpty())
            && marker.GetQuadName().IsEmpty())
            return false;
        if (!marker.GetHighlightImage().IsEmpty() && !marker.GetHighlightImage().EndsWith(".imageset"))
            return false;

        return ValidateFactionKeys(marker.GetFactionKeys(), GC_BriefingMarker.MAX_FACTION_KEYS);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_UpsertBriefingMarker(GC_BriefingMarker update)
    {
        if (Replication.IsServer() || !m_BriefingCatalog.ApplyMarker(update))
            return;

        m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_RemoveBriefingMarker(string markerName, int revision)
    {
        if (Replication.IsServer() || !m_BriefingCatalog.ApplyMarkerRemoval(markerName, revision))
            return;

        m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
    }

    bool TryJoinGroupFromVoiceHeader(int playerId, RplId groupRplId)
    {
        if (!Replication.IsServer())
        {
            Print("GC | Voice group join rejected: request reached a non-authority instance", level: LogLevel.ERROR);
            return false;
        }

        if (playerId <= 0)
        {
            PrintFormat("GC | Voice group join rejected: invalid player id=%1", playerId, level: LogLevel.ERROR);
            return false;
        }

        if (!m_bVoiceGroupHeaderSwitchesGroup)
        {
            Print("GC | Voice group join rejected: group voice-channel selection is disabled in the Lobby2 component", level: LogLevel.WARNING);
            return false;
        }

        SCR_AIGroup targetGroup = GetRegisteredGroupByRplId(groupRplId);
        if (!targetGroup)
        {
            PrintFormat("GC | Voice group join rejected: unknown group rplId=%1 player=%2", groupRplId, playerId, level: LogLevel.WARNING);
            return false;
        }

        Faction playerFaction = GetPlayerFaction(playerId);
        if (!playerFaction || playerFaction != targetGroup.GetFaction())
        {
            PrintFormat("GC | Voice group join rejected: player=%1 faction=%2 targetFaction=%3", playerId, playerFaction, targetGroup.GetFaction(), level: LogLevel.WARNING);
            return false;
        }

        // Voice-channel selection is presentation/routing state only. Do not
        // move the player between authoritative base-game groups or clear the
        // Lobby2 slot that supplies their role and slot metadata.
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, targetGroup.GetFaction().GetFactionKey(), groupRplId);
        return true;
    }

    //! Validates and applies a player-owned slot request on the server authority.
    bool TryAssignPlayerToSlot(int playerId, int slotId)
    {
        if (!Replication.IsServer() || playerId <= 0)
            return false;

		SCR_EGameModeState phase = GetLobbyPhase();
        GC_Slot targetSlot = GetSlot(slotId);
        if (!CanPlayerRequestSlot(playerId, targetSlot))
        {
			PrintFormat("GC | Slot request rejected: player=%1 slot=%2 phase=%3 faction=%4", playerId, slotId, GetPhaseName(phase), GetPlayerFaction(playerId), level: LogLevel.WARNING);
            return false;
		}

        int occupyingPlayerId = targetSlot.GetPlayerId();
		SCR_AIGroup targetGroup = ResolveSlotGroup(targetSlot);

		if (FORMATION_DEBUG_LOGGING)
		{
			vector slotOrigin = Vector(0, 0, 0);
			if (targetSlot.GetGroup())
				slotOrigin = targetSlot.GetGroup().GetOrigin();
			PrintFormat("GC | Formation | slot assignment slot=%1 targetOrigin=%2 targetAngles=%3 slotOrigin=%4 sameGroup=%5", slotId, targetGroup.GetOrigin(), targetGroup.GetAngles(), slotOrigin, targetSlot.GetGroup() == targetGroup);
		}

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
        if (!groupsManager)
            return false;

        GC_Slot currentSlot = GetSlotByPlayerId(playerId);
        SCR_AIGroup currentGroup = GetPlayerGroup(playerId);

		// Toggling out remains SLOTTING-only. From BRIEFING onward the current
		// authoritative group is also the player's faction lock.
		if (occupyingPlayerId == playerId)
		{
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);

			ReleaseSlotClaim(targetSlot);
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
            PrintFormat("GC | Slot request accepted (unslotted): player=%1 slot=%2", playerId, slotId);
            return true;
        }

        if (currentGroup != targetGroup)
        {
            if (currentGroup)
                currentGroup.RemovePlayer(playerId);

            targetGroup.AddPlayer(playerId);
            if (!targetGroup.GetPlayerIDs().Contains(playerId))
            {
                if (currentGroup)
                    currentGroup.AddPlayer(playerId);

                PrintFormat("GC | Slot request rejected after group assignment failed: player=%1 slot=%2", playerId, slotId, level: LogLevel.WARNING);
                return false;
            }
        }

		if (currentSlot && currentSlot != targetSlot)
			ReleaseSlotClaim(currentSlot);

		targetSlot.SetPlayer(playerId);
		if (phase != SCR_EGameModeState.SLOTTING && targetSlot.GetEntity())
			targetSlot.SetState(GC_SlotState.LOCKED);
		else
			targetSlot.SetState(GC_SlotState.RESERVED);
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, targetGroup.GetFaction().GetFactionKey(), Replication.FindItemId(targetGroup));
		if (phase == SCR_EGameModeState.BRIEFING && !targetSlot.GetEntity())
			SpawnSlotEntity(targetSlot, false);
		else if (phase == SCR_EGameModeState.GAME)
			QueuePlayerCatchUp(playerId);

        PrintFormat("GC | Slot request accepted: player=%1 slot=%2", playerId, slotId);
        return true;
    }

	//! Shared presentation and authority policy for player-owned slot requests.
	//! Base-game group membership supplies the faction lock; Lobby2 stores no
	//! parallel faction ownership value.
	bool CanPlayerRequestSlot(int playerId, GC_Slot targetSlot)
	{
		if (playerId <= 0 || !targetSlot)
			return false;

		SCR_EGameModeState phase = GetLobbyPhase();
		if (phase != SCR_EGameModeState.SLOTTING
			&& phase != SCR_EGameModeState.BRIEFING
			&& phase != SCR_EGameModeState.GAME)
			return false;

		int occupyingPlayerId = targetSlot.GetPlayerId();
		if (targetSlot.GetState() != GC_SlotState.FREE && occupyingPlayerId != playerId)
			return false;
		if (occupyingPlayerId >= 0 && occupyingPlayerId != playerId)
			return false;
		if (occupyingPlayerId == playerId)
			return phase == SCR_EGameModeState.SLOTTING;

		SCR_AIGroup targetGroup = ResolveSlotGroup(targetSlot);
		if (!targetGroup || !targetGroup.GetFaction())
			return false;

		if (phase == SCR_EGameModeState.SLOTTING)
			return true;

		GC_Slot currentSlot = GetSlotByPlayerId(playerId);
		if (currentSlot && currentSlot.IsKIA())
			return false;

		Faction lockedFaction = GetPlayerFaction(playerId);
		return !lockedFaction || lockedFaction == targetGroup.GetFaction();
	}

    //! Broadcasts a complete server-authoritative slot state.
    bool UpdateSlot(GC_Slot slot)
    {
        if (!Replication.IsServer() || !slot || !m_Roster.GetSlots().Contains(slot))
            return false;

        Rpc(RpcDo_UpdateSlot, slot);
        return true;
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_UpdateSlot(GC_Slot updateSlot)
    {
        if (Replication.IsServer() || !updateSlot)
            return;

        int slotId = updateSlot.GetID();
        GC_Slot slot = GetSlot(slotId);
        if (!slot)
        {
            slot = new GC_Slot();
            slot.SetID(slotId);
            m_Roster.AddSlot(slot);
            SubscribeToSlotEvents(slot);
        }

        slot.CopyFrom(updateSlot);
		if (!slot.GetGroup() && slot.GetGroupRplId() != RplId.Invalid())
			ResolvePendingJipReferences();
    }

    override void OnGameModeEnd(SCR_GameModeEndData data)
    {
		Shutdown();
		super.OnGameModeEnd(data);
	}

	override void OnGameEnd()
	{
		Shutdown();
		super.OnGameEnd();
	}

	protected void Shutdown()
	{
		if (m_bShutdown)
			return;

		m_bShutdown = true;
		GetGame().GetCallqueue().Remove(ReconcilePendingPlayerRoles);
		GetGame().GetCallqueue().Remove(FlushAffiliationPresentation);
		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		GetGame().GetCallqueue().Remove(ProcessBriefingSlotSpawns);
		GetGame().GetCallqueue().Remove(ProcessPlayerCatchUps);
		m_aPendingBriefingSpawns.Clear();
		m_aPendingPlayerCatchUps.Clear();
		m_mPlayerCatchUpAttempts.Clear();
		m_mPendingRoleAdmin.Clear();
		m_mPendingRoleAttempts.Clear();
		m_sPendingAffiliationPlayers.Clear();
		m_sPendingFactionPresentation.Clear();
		m_mLobbySpectators.Clear();
		m_mReservedSpectators.Clear();
		if (m_FormationCatalog)
			m_FormationCatalog.Clear();
		m_aGroupSpawnTransforms.Clear();
        UnsubscribeFromAllGroupEvents();
        UnsubscribeFromAllSlotEvents();
		m_Roster.Clear();
		m_VoiceState.Clear();
		m_BriefingCatalog.Clear();
		if (m_ParameterCatalog)
			m_ParameterCatalog.Clear();
		Print("GC | Lifecycle | Lobby2 manager shutdown complete");
    }

	static GC_LobbyManager GetManager()
	{
		GC_LobbyGameMode lobbyGameMode = GC_LobbyGameMode.GetGameMode();
		if (!lobbyGameMode)
			return null;

		return GC_LobbyManager.Cast(lobbyGameMode.FindComponent(GC_LobbyManager));
	}
}
