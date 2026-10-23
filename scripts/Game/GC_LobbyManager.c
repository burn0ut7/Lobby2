class GC_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_LobbyManager : SCR_BaseGameModeComponent
{
	protected const ResourceName SPECTATOR_PREFAB = "{8A578DE95771CC1D}Prefabs/Characters/Core/Character_Spectator.et";
	protected const ResourceName DEFAULT_FORMATION_SOURCE = "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et";
	protected const string DEFAULT_FORMATION_NAME = "File";
	protected const bool FORMATION_DEBUG_LOGGING = false;
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
	protected ref map<int, float> m_mPriorityVoiceExpiry = new map<int, float>();
	protected ref map<int, float> m_mAdminMessageTimes = new map<int, float>();
	protected int m_iAdminMessageSequence;
	protected ref GC_AdminBanService m_AdminBanService;
	//! 0 follows the phase, 1 locks faction switching, 2 permits switching.
	protected int m_iFactionLockOverride;
	protected bool m_bEnforceFactionBalance;
	protected int m_iFactionRulesRevision;
	ref ScriptInvoker<int> m_OnFactionRulesChanged = new ScriptInvoker<int>();
	protected ref array<string> m_aAdminSessionHistory = {};
	protected ref array<ref GC_AdminSessionPlayer> m_aModerationHistory = {};
	protected ref map<int, string> m_mAdminSessionLabels = new map<int, string>();
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

		GetAdminSelectedPlayer();
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
		return GC_LobbyPhase.GetName(state);
	}

	int GetSlotReservationTimeoutSeconds()
	{
		return Math.Max(0, m_iSlotReservationTimeoutSeconds);
	}

	//! Validates and applies an admin-owned request on the server authority.
	bool TryAdvanceLobbyPhase(int playerId, SCR_EGameModeState requestedPhase)
	{
		string requestedPhaseName = GetPhaseName(requestedPhase);
		if (!AuthorizeAdminAction(playerId, "advance-phase", requestedPhaseName))
			return false;
		if (!GC_LobbyPhase.IsValid(requestedPhase))
		{
			GC_AdminActionLogger.Log(playerId, "advance-phase", requestedPhaseName, "rejected-invalid-target");
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

		// Parameters are weak references. Retain a newly created result until
		// EndGameMode takes ownership, including throughout its callbacks.
		ref SCR_GameModeEndData resolvedEndData = endData;
		if (!resolvedEndData)
			resolvedEndData = SCR_GameModeEndData.CreateSimple();

		PrintFormat("GC | Phase | invoking native EndGameMode: lobby=%1 base=%2", GetPhaseName(GetLobbyPhase()), GetBaseStateName(gameMode.GetState()));
		gameMode.EndGameMode(resolvedEndData);
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
		{
			RecordAdminSessionEvent(playerId, true, 0);
			QueuePlayerCatchUp(playerId);
		}

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
		{
			RecordAdminSessionEvent(playerId, false, KickCauseCodeAPI.GetReason(cause));
			HandlePlayerDisconnected(playerId);
		}

		if (!isShutdown)
		{
			RemovePlayerVoiceAssignment(playerId);
			if (m_iAdminSelectedPlayer == playerId)
				SelectAdminPlayer(-1);
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

		// A timer from an older claim cannot expire a new initial reservation.
		if (slot.IsAwaitingInitialJoin())
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

		// Initial pre-assignment uses the same native group/slot transition as
		// ordinary assignment; reconnects keep their existing lifecycle.
		if (slot.IsAwaitingInitialJoin())
		{
			if (!ApplyPlayerSlotAssignment(playerId, slot, false, false))
			{
				PrintFormat("GC | Lifecycle | initial reservation assignment failed: player=%1 slot=%2", playerId, slot.GetID(), level: LogLevel.WARNING);
				SetSpectator(playerId, true);
				return;
			}
		}
		else
			slot.ReconnectPlayer(playerId);
		SCR_AIGroup group = slot.GetGroup();
		if (group && !group.GetPlayerIDs().Contains(playerId))
			group.AddPlayer(playerId);
		SynchronizePlayerNativeMembership(playerId);

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

	protected bool SpawnSlotEntity(GC_Slot slot, bool force, SCR_AIGroup expectedGroup = null, EntitySpawnParams spawnOverride = null)
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

		GC_SlotSpawnPoint adminSpawnPoint = slot.GetAdminSpawnPoint();
		if (adminSpawnPoint)
			params = adminSpawnPoint.CreateParams();
		if (spawnOverride)
			params = spawnOverride;
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
		{
			// Authored lobby groups need native IDs before controller membership
			// can be published to native chat and other group consumers.
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (groupsManager)
				groupsManager.GC_RegisterLobbyGroup(group);
			CacheGroupSpawnTransform(group);
		}

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
		if (GetManager() != this)
			return;

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
		if (GetManager() != this)
			return;

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
		if (GetManager() != this)
			return;

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
		if (GetManager() != this)
			return;

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

		writer.WriteInt(m_iFactionLockOverride);
		writer.WriteBool(m_bEnforceFactionBalance);
		writer.WriteInt(m_iFactionRulesRevision);

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

			m_Roster.QueueJipGroup(groupRplId);
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

		if (!reader.ReadInt(m_iFactionLockOverride) || !reader.ReadBool(m_bEnforceFactionBalance)
			|| !reader.ReadInt(m_iFactionRulesRevision) || m_iFactionLockOverride < 0 || m_iFactionLockOverride > 2)
			return false;
		m_OnFactionRulesChanged.Invoke(m_iFactionRulesRevision);

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
		if (GetManager() != this)
			return;

		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		int attempt = m_Roster.IncrementJipAttempt();
		array<RplId> pendingGroups = m_Roster.GetPendingGroupIds();

		// Resolve every available group. The roster preserves snapshot order
		// independently, so one delayed proxy cannot hide all subsequent groups.
		bool topologyChanged;
		for (int groupIndex = pendingGroups.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			if (!ResolveJipGroup(pendingGroups[groupIndex]))
				continue;

			pendingGroups.RemoveOrdered(groupIndex);
			topologyChanged = true;
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
			if (topologyChanged)
				m_OnRegistryReset.Invoke(attempt);
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
		if (GetManager() != this)
			return;

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

				GetAdminSelectedPlayer();
				m_OnAdminSelectionChanged.Invoke();
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

	//------------------------------------------------------------------------------------------------
	//! Resolve announcement recipients on authority. Admin chat uses native channels.
	string SendAdminMessage(int requesterId, GC_EAdminMessageAudience audience, string message, array<string> selectedIdentities, string factionKey = "")
	{
		message.TrimInPlace();
		string target = string.Format("audience=%1 faction=%2 length=%3", audience, factionKey, message.Length());
		if (!AuthorizeAdminAction(requesterId, "message-send", target))
			return "rejected-not-logged-in";
		string outcome = "rejected-message";
		if (message.IsEmpty() || message.Length() > 512 || message.Contains("<") || message.Contains(">"))
		{
			GC_AdminActionLogger.Log(requesterId, "message-send", target, outcome);
			return outcome;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		float previous;
		if (m_mAdminMessageTimes.Find(requesterId, previous) && now - previous < 500)
		{
			GC_AdminActionLogger.Log(requesterId, "message-send", target, "rejected-rate-limit");
			return "rejected-rate-limit";
		}

		array<int> recipients = {};
		if (audience == GC_EAdminMessageAudience.SELECTED_PLAYERS)
		{
			if (!selectedIdentities || selectedIdentities.IsEmpty() || selectedIdentities.Count() > 64)
			{
				GC_AdminActionLogger.Log(requesterId, "message-send", target, "rejected-recipients");
				return "rejected-recipients";
			}
			foreach (string identity : selectedIdentities)
			{
				int playerId = GetConnectedPlayerIdByIdentity(identity);
				if (playerId <= 0)
				{
					GC_AdminActionLogger.Log(requesterId, "message-send", target, "rejected-stale-recipient");
					return "rejected-stale-recipient";
				}
				if (!recipients.Contains(playerId))
					recipients.Insert(playerId);
			}
		}
		else if (audience == GC_EAdminMessageAudience.ALL_PLAYERS)
		{
			GetGame().GetPlayerManager().GetPlayers(recipients);
		}
		else if (audience == GC_EAdminMessageAudience.FACTION)
		{
			foreach (GC_Slot slot : GetSlots())
			{
				SCR_AIGroup group = slot.GetGroup();
				if (!group || !group.GetFaction() || group.GetFaction().GetFactionKey() != factionKey)
					continue;
				int memberId = slot.GetPlayerId();
				if (memberId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(memberId) && !recipients.Contains(memberId))
					recipients.Insert(memberId);
			}
		}
		else
		{
			GC_AdminActionLogger.Log(requesterId, "message-send", target, "rejected-audience");
			return "rejected-audience";
		}

		if (recipients.IsEmpty())
		{
			GC_AdminActionLogger.Log(requesterId, "message-send", target, "rejected-empty-audience");
			return "rejected-empty-audience";
		}
		target += " players=";
		foreach (int targetId : recipients)
			target += string.Format("%1,", targetId);
		m_mAdminMessageTimes.Set(requesterId, now);
		m_iAdminMessageSequence++;
		int sent;
		string senderName = GetGame().GetPlayerManager().GetPlayerName(requesterId);
		foreach (int recipientId : recipients)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(recipientId));
			if (!controller)
				continue;
			controller.DeliverAdminMessage(m_iAdminMessageSequence, requesterId, senderName, audience, message);
			sent++;
		}
		outcome = string.Format("dispatched message=%1 recipients=%2", m_iAdminMessageSequence, sent);
		GC_AdminActionLogger.Log(requesterId, "message-send", target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	//! One server entry for manual reservations and versioned roster transfer.
	string ExecuteSlotReservation(int requesterId, string action, int slotId, UUID expectedClaim, string identityText, string playerName, string input, out string output)
	{
		output = "";
		string target = string.Format("slot=%1 operation=%2", slotId, action);
		if (!AuthorizeAdminAction(requesterId, "slot-reservation", target))
			return "Sign in as an administrator to manage reservations.";

		string outcome;
		if (action == "export")
			outcome = ExportSlotReservations(output);
		else if (GC_LobbyPhase.IsAtOrAfter(GetLobbyPhase(), SCR_EGameModeState.GAME))
			outcome = "Reservations can be changed during slotting or briefing.";
		else if (action == "import")
			outcome = ImportSlotReservations(input);
		else
		{
			GC_Slot slot = GetSlot(slotId);
			outcome = "The slot changed. Select it again and retry.";
			if (slot && slot.GetUUID() == expectedClaim)
			{
				if (action == "clear" && slot.IsAwaitingInitialJoin() && slot.GetPlayerId() <= 0)
				{
					ReleaseSlotClaim(slot);
					outcome = "Reservation cleared.";
				}
				else if (action == "reserve")
				{
					identityText.TrimInPlace();
					identityText.ToLower();
					UUID identity = identityText;
					playerName.TrimInPlace();
					target += " identity=" + identity;
					outcome = ValidateSlotReservation(slot, identity);
					if (playerName.Length() > 128 || playerName.Contains("\n") || playerName.Contains("\r"))
						outcome = "Use a player name of 128 characters or fewer on one line.";
					if (outcome.IsEmpty())
					{
						int playerId = GetConnectedPlayerIdByIdentity(identity);
						if (slot.HasClaim())
							outcome = "This player already holds the slot. Its existing claim was kept.";
						else if (playerId > 0)
						{
							// Reuse the authoritative assignment path for connected players.
							string assigned = ExecuteAdminAssignSlot(requesterId, playerId, identity, slotId, expectedClaim);
							outcome = GC_AdminActionResultText.Get(assigned);
						}
						else
						{
							slot.ReserveIdentity(identity, playerName);
							outcome = "Slot reserved. The player will be assigned when they join.";
						}
					}
				}
				else
					outcome = "Only a reservation awaiting its first join can be cleared here.";
			}
		}
		GC_AdminActionLogger.Log(requesterId, "slot-reservation", target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string ValidateSlotReservation(GC_Slot slot, UUID identity)
	{
		if (!UUID.IsUUID(identity) || identity.IsNull())
			return "Enter a valid Arma UID (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx).";
		if (!slot || slot.IsKIA() || slot.IsAdminLocked())
			return "This slot is unavailable or locked.";
		SCR_AIGroup group = ResolveSlotGroup(slot);
		if (!group || !group.GetFaction())
			return "The slot's group is unavailable.";
		if (slot.HasClaim() && slot.GetUUID() != identity)
			return "This slot is already claimed. Clear its reservation or remove its player first.";
		GC_Slot current = m_Roster.GetSlotByUUID(identity);
		if (current && current != slot && current.GetPlayerId() <= 0)
			return "This UID already holds another slot. Clear that claim first.";
		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Stable keys derive from authored group geometry, never runtime callsigns or IDs.
	protected string GetReservationGroupKey(GC_Slot slot)
	{
		SCR_AIGroup group = ResolveSlotGroup(slot);
		if (!group || !group.GetFaction() || !group.GetPrefabData() || slot.GetGroupSlotIndex() < 0 || slot.GetAdminSpawnPoint())
			return "";
		GC_PlayerGroupSpawnTransform transform = FindGroupSpawnTransform(group);
		if (!transform)
			return "";
		string source = string.Format("%1|%2|%3|%4|%5", GetGame().GetWorldFile(),
			group.GetPrefabData().GetPrefabName(), transform.GetOrigin(), transform.GetAngles(), group.GetFaction().GetFactionKey());
		return UUID.GenV8(UUID.NAMESPACE_URL, source);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetReservationSlotKey(GC_Slot slot)
	{
		string groupKey = GetReservationGroupKey(slot);
		if (groupKey.IsEmpty())
			return "";
		return UUID.GenV8(UUID.NAMESPACE_URL, string.Format("%1|%2|%3", groupKey, slot.GetGroupSlotIndex(), slot.GetAuthoredPrefab()));
	}

	//------------------------------------------------------------------------------------------------
	protected string ExportSlotReservations(out string output)
	{
		array<ref map<string, string>> rows = {};
		array<string> keys = {};
		int excluded;
		foreach (GC_Slot slot : GetSlots())
		{
			string key = GetReservationSlotKey(slot);
			if (key.IsEmpty())
			{
				excluded++;
				continue;
			}
			if (keys.Contains(key))
				return "Export rejected: authored slots have ambiguous locations and roles.";
			keys.Insert(key);
			SCR_AIGroup group = ResolveSlotGroup(slot);
			GC_PlayerGroupSpawnTransform transform = FindGroupSpawnTransform(group);
			map<string, string> row = new map<string, string>();
			row.Set("slotKey", key);
			row.Set("slotNumber", slot.GetID().ToString());
			row.Set("teamKey", group.GetFaction().GetFactionKey());
			row.Set("teamName", WidgetManager.Translate(group.GetFaction().GetFactionName()));
			row.Set("groupKey", GetReservationGroupKey(slot));
			row.Set("groupName", GC_GroupCallsignHelper.GetDisplayName(group));
			row.Set("groupPrefab", group.GetPrefabData().GetPrefabName());
			row.Set("groupPosition", transform.GetOrigin().ToString());
			row.Set("unitIndex", slot.GetGroupSlotIndex().ToString());
			row.Set("rolePrefab", slot.GetAuthoredPrefab());
			SCR_UIInfo role = GC_Slot.GetPrefabRoleInfo(slot.GetAuthoredPrefab());
			if (role)
			{
				row.Set("roleNameKey", role.GetName());
				row.Set("roleName", WidgetManager.Translate(role.GetName()));
			}
			else
				row.Set("roleName", "Unit " + (slot.GetGroupSlotIndex() + 1).ToString());
			row.Set("playerName", slot.GetCachedPlayerName());
			row.Set("armaUid", slot.GetUUID());
			rows.Insert(row);
		}
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("version", 1) || !context.WriteValue("world", GetGame().GetWorldFile()) || !context.WriteValue("slots", rows))
			return "The roster could not be serialized.";
		output = context.SaveToString();
		if (output.Length() > 32768 || rows.Count() > 128)
		{
			output = "";
			return "This roster exceeds the transfer limit of 128 slots / 32 KiB.";
		}
		return string.Format("Exported %1 authored slots. %2 runtime-only slots excluded. Copy the text to save it.", rows.Count(), excluded);
	}

	//------------------------------------------------------------------------------------------------
	//! Descriptive metadata is an export snapshot; only stable keys select live slots.
	//! Validate all claims first. Blank UIDs are no-ops, never a bulk removal request.
	protected string ImportSlotReservations(string input)
	{
		if (input.IsEmpty() || input.Length() > 32768)
			return "Paste a reservation export of 32 KiB or less.";
		JsonLoadContext context = new JsonLoadContext(false);
		int version;
		string world;
		array<ref map<string, string>> rows;
		if (!context.LoadFromString(input) || !context.ReadValue("version", version) || version != 1
			|| !context.ReadValue("world", world) || !context.ReadValue("slots", rows) || !rows)
			return "Invalid reservation document. Use a version 1 Lobby2 export.";
		if (world != GetGame().GetWorldFile())
			return "This export belongs to a different scenario.";
		if (rows.Count() > 128)
			return "Import supports at most 128 slots.";

		map<string, GC_Slot> currentSlots = new map<string, GC_Slot>();
		foreach (GC_Slot existing : GetSlots())
		{
			string currentKey = GetReservationSlotKey(existing);
			if (currentKey.IsEmpty())
				continue;
			if (currentSlots.Contains(currentKey))
				return "Import rejected: authored slot locations are ambiguous.";
			currentSlots.Set(currentKey, existing);
		}
		array<GC_Slot> pending = {};
		array<string> identities = {};
		array<string> names = {};
		array<string> seenKeys = {};
		array<string> seenIdentities = {};
		foreach (map<string, string> row : rows)
		{
			string key;
			string value;
			if (!row || !row.Find("slotKey", key) || !row.Find("armaUid", value))
				return "Each slot needs slotKey and armaUid fields. No reservations were changed.";
			GC_Slot slot = currentSlots.Get(key);
			if (!slot || seenKeys.Contains(key))
				return "A slot is missing, changed, or repeated. Export the current scenario again.";
			seenKeys.Insert(key);
			value.TrimInPlace();
			value.ToLower();
			if (value.IsEmpty())
				continue;
			if (seenIdentities.Contains(value))
				return "The same Arma UID appears more than once. No reservations were changed.";
			seenIdentities.Insert(value);
			string error = ValidateSlotReservation(slot, value);
			if (!error.IsEmpty())
				return string.Format("Slot %1: %2 No reservations were changed.", slot.GetID(), error);
			string playerName = row.Get("playerName");
			playerName.TrimInPlace();
			if (playerName.Length() > 128 || playerName.Contains("\n") || playerName.Contains("\r"))
				return "Player names must be 128 characters or fewer on one line. No reservations were changed.";
			GC_Slot existingClaim = m_Roster.GetSlotByUUID(value);
			if (existingClaim && existingClaim != slot)
				return "An imported player already occupies another slot. Use manual assignment to move them.";
			if (existingClaim == slot)
				continue;
			pending.Insert(slot);
			identities.Insert(value);
			names.Insert(playerName);
		}
		foreach (int index, GC_Slot reserved : pending)
			reserved.ReserveIdentity(identities[index], names[index]);
		foreach (string identity : identities)
		{
			int playerId = GetConnectedPlayerIdByIdentity(identity);
			if (playerId > 0)
				QueuePlayerCatchUp(playerId);
		}
		return string.Format("Reserved %1 slots. Connected players will join their slots. Matching claims and blank UIDs made no changes.", pending.Count());
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminAssignSlot(int requesterId, int playerId, UUID expectedIdentity, int slotId, UUID expectedClaim)
	{
		string target = string.Format("player=%1 identity=%2 slot=%3 claim=%4", playerId, expectedIdentity, slotId, expectedClaim);
		if (!AuthorizeAdminAction(requesterId, "slot-assign", target))
			return "rejected-not-logged-in";
		string outcome = "rejected-stale-target";
		GC_Slot slot = GetSlot(slotId);
		if (slot && slot.GetUUID() == expectedClaim && !expectedIdentity.IsEmpty() && GetConnectedPlayerIdByIdentity(expectedIdentity) == playerId)
		{
			outcome = "rejected-slot-unavailable";
			SCR_EGameModeState phase = GetLobbyPhase();
			SCR_AIGroup group = ResolveSlotGroup(slot);
			if (!slot.IsKIA() && (!slot.HasClaim() || slot.GetPlayerId() == playerId) && group && group.GetFaction() && phase != SCR_EGameModeState.POSTGAME)
			{
				bool spawned = true;
				if (phase == SCR_EGameModeState.BRIEFING || phase == SCR_EGameModeState.GAME)
					spawned = SpawnSlotEntity(slot, true, group);
				outcome = "failed-character-spawn";
				if (spawned)
				{
					outcome = "failed-group-assignment";
					if (ApplyPlayerSlotAssignment(playerId, slot, false, true))
					{
						outcome = "applied";
						PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
						if (phase == SCR_EGameModeState.GAME && (!controller || controller.GetControlledEntity() != slot.GetEntity()))
							outcome = "assigned-character-entry-failed";
					}
				}
			}
		}
		GC_AdminActionLogger.Log(requesterId, "slot-assign", target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	int GetFactionLockOverride() { return m_iFactionLockOverride; }
	bool IsFactionBalanceEnforced() { return m_bEnforceFactionBalance; }
	int GetFactionRulesRevision() { return m_iFactionRulesRevision; }

	//------------------------------------------------------------------------------------------------
	bool IsFactionSwitchLocked()
	{
		if (m_iFactionLockOverride == 1)
			return true;
		if (m_iFactionLockOverride == 2)
			return false;
		return GC_LobbyPhase.IsAtOrAfter(GetLobbyPhase(), SCR_EGameModeState.BRIEFING);
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminFactionRules(int requesterId, int expectedRevision, int lockOverride, bool enforceBalance)
	{
		string target = string.Format("revision=%1 lockMode=%2 balance=%3", expectedRevision, lockOverride, enforceBalance);
		if (!AuthorizeAdminAction(requesterId, "faction-rules", target))
			return "rejected-not-logged-in";
		string outcome = "applied";
		if (expectedRevision != m_iFactionRulesRevision)
			outcome = "rejected-stale-rules";
		else if (lockOverride < 0 || lockOverride > 2 || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			outcome = "rejected-faction-rules";
		else
		{
			m_iFactionLockOverride = lockOverride;
			m_bEnforceFactionBalance = enforceBalance;
			m_iFactionRulesRevision++;
			Rpc(RpcDo_AdminFactionRules, m_iFactionRulesRevision, lockOverride, enforceBalance);
			m_OnFactionRulesChanged.Invoke(m_iFactionRulesRevision);
		}
		GC_AdminActionLogger.Log(requesterId, "faction-rules", target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_AdminFactionRules(int revision, int lockOverride, bool enforceBalance)
	{
		if (Replication.IsServer() || GC_LobbyManager.GetManager() != this || m_bShutdown || revision <= m_iFactionRulesRevision)
			return;
		m_iFactionRulesRevision = revision;
		m_iFactionLockOverride = lockOverride;
		m_bEnforceFactionBalance = enforceBalance;
		m_OnFactionRulesChanged.Invoke(revision);
	}

	//------------------------------------------------------------------------------------------------
	//! Ordinary choices obey proportional quotas. Explicit administrator assignment
	//! uses its separately authorized route and can override these lobby rules.
	bool CanPlayerJoinBalancedFaction(int playerId, Faction targetFaction)
	{
		if (!targetFaction)
			return false;
		if (!m_bEnforceFactionBalance || GetPlayerFaction(playerId) == targetFaction)
			return true;
		array<string> keys = {};
		foreach (Faction faction : GetRegisteredFactions())
		{
			if (faction && GetFactionSlotCount(faction.GetFactionKey()) > 0)
				keys.Insert(faction.GetFactionKey());
		}
		if (keys.Count() < 2)
			return true;
		keys.Sort();
		array<int> capacities = {};
		foreach (string key : keys)
			capacities.Insert(GetFactionSlotCount(key));
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<int> quotas;
		GC_FactionBalancePolicy.Calculate(capacities, players.Count(), quotas);
		int index = keys.Find(targetFaction.GetFactionKey());
		if (index < 0)
			return false;
		int occupants;
		foreach (int connectedPlayerId : players)
		{
			if (GetPlayerFaction(connectedPlayerId) == targetFaction)
				occupants++;
		}
		return occupants < quotas[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Session history is presentation evidence, never player or permission authority.
	protected void RecordAdminSessionEvent(int playerId, bool connected, int cause)
	{
		if (connected)
		{
			string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			GC_AdminSessionPlayer record;
			foreach (GC_AdminSessionPlayer prior : m_aModerationHistory)
			{
				if (prior.m_sIdentity == identity && !identity.IsEmpty())
					record = prior;
			}
			if (!record)
			{
				record = new GC_AdminSessionPlayer();
				m_aModerationHistory.Insert(record);
			}
			record.m_iPlayerId = playerId;
			record.m_sName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			record.m_sIdentity = identity;
			// Installed native API; obsolete but no replacement platform-ID accessor is exposed.
			if (GetGame().GetPlayerManager().GetPlatformKind(playerId) == PlatformKind.STEAM)
				record.m_sSteamId = GetGame().GetBackendApi().GetPlayerPlatformId(playerId);
		}
		string label;
		if (connected)
		{
			label = string.Format("%1 / %2 / session %3", GetGame().GetPlayerManager().GetPlayerName(playerId), SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId), playerId);
			label.Replace("\n", " ");
			label.Replace("\r", " ");
			m_mAdminSessionLabels.Set(playerId, label);
		}
		else
		{
			if (!m_mAdminSessionLabels.Find(playerId, label))
				label = string.Format("session %1", playerId);
			m_mAdminSessionLabels.Remove(playerId);
		}
		string eventName = "left";
		if (connected)
			eventName = "joined";
		m_aAdminSessionHistory.Insert(string.Format("%1 / %2 / %3 / cause %4", System.GetUnixTime(), eventName, label, cause));
		if (m_aAdminSessionHistory.Count() > 256)
			m_aAdminSessionHistory.RemoveOrdered(0);
		NotifyModerationChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates a player target against the current session and character, not a cached menu.
	string ValidateAdminFollowPlayer(int requesterId, int targetId, UUID expectedIdentity, out RplId entityId)
	{
		entityId = RplId.Invalid();
		string target = string.Format("player=%1 identity=%2", targetId, expectedIdentity);
		if (!AuthorizeAdminAction(requesterId, "spectator-follow", target))
			return "rejected-not-logged-in";
		string outcome = "rejected-stale-player";
		PlayerManager players = GetGame().GetPlayerManager();
		GC_LobbyPlayerController requester = GC_LobbyPlayerController.Cast(players.GetPlayerController(requesterId));
		if (!expectedIdentity.IsEmpty() && players.IsPlayerConnected(targetId) && GetPlayerVoiceIdentity(targetId) == expectedIdentity)
		{
			outcome = "unavailable-follow-camera";
			IEntity entity = players.GetPlayerControlledEntity(targetId);
			if (requester && requester.GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR && requesterId != targetId && entity && !entity.FindComponent(GC_LobbySpectatorBodyComponent))
			{
				CharacterControllerComponent character = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
				RplComponent replication = RplComponent.Cast(entity.FindComponent(RplComponent));
				if (character && !character.IsDead() && !character.IsUnconscious() && replication)
				{
					entityId = replication.Id();
					if (entityId.IsValid())
						outcome = "authorized";
				}
			}
		}
		// Authorization is not proof that the owner's local camera attached successfully.
		GC_AdminActionLogger.Log(requesterId, "spectator-follow", target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	string GetAdminSessionReport(int requesterId, string search)
	{
		if (!AuthorizeAdminAction(requesterId, "admin-session-report", search))
			return "A current admin login is required.";
		if (search.Length() > 64)
		{
			GC_AdminActionLogger.Log(requesterId, "admin-session-report", "search-length=" + search.Length().ToString(), "rejected-search-length");
			return "Search is limited to 64 characters.";
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		float uptimeSeconds = System.GetTickCount() * 0.001;
		string report = string.Format("Server FPS (last 10 frames): %1\nServer uptime: %2 seconds\nPlayers online: %3 / Slots: %4\nSession history (last 256 events; newest 64 matches):\n", System.GetFPS(), uptimeSeconds, players.Count(), GetSlots().Count());
		search.ToLower();
		int count;
		for (int index = m_aAdminSessionHistory.Count() - 1; index >= 0 && count < 64; index--)
		{
			string entry = m_aAdminSessionHistory[index];
			string lower = entry;
			lower.ToLower();
			if (!search.IsEmpty() && !lower.Contains(search))
				continue;
			report += entry + "\n";
			count++;
		}
		if (count == 0)
			report += "No matching session events.";
		GC_AdminActionLogger.Log(requesterId, "admin-session-report", search, string.Format("returned matches=%1", count));
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminPlayerAction(int requesterId, string action, int targetId, UUID expectedIdentity, string reason, int duration, int page, int requestId = 0, int expiresAt = 0)
	{
		string target = string.Format("player=%1 identity=%2 duration=%3 page=%4 reason=%5", targetId, expectedIdentity, duration, page, reason);
		target += string.Format(" name=%1 expiresAt=%2 requestId=%3", GetGame().GetPlayerManager().GetPlayerName(targetId), expiresAt, requestId);
		if (!AuthorizeAdminAction(requesterId, action, target))
			return "rejected-not-logged-in";
		if (expiresAt != 0)
		{
			int now = System.GetUnixTime();
			if (action != "player-ban" || duration != 0 || expiresAt <= now || expiresAt - now > 315360000)
			{
				GC_AdminActionLogger.Log(requesterId, action, target, "rejected-ban-expiry");
				return "rejected-ban-expiry";
			}
			duration = expiresAt - now;
		}
		string outcome = ValidateAdminPlayerAction(action, targetId, expectedIdentity, reason, duration, page);
		if (outcome.IsEmpty())
		{
			if (action == "player-kick")
			{
				GetGame().GetPlayerManager().KickPlayer(targetId, PlayerManagerKickReason.KICK);
				RecordModerationAction(expectedIdentity, "Kick requested", reason, 0);
				outcome = "dispatched-player-kick";
			}
			else
			{
				if (!m_AdminBanService)
					m_AdminBanService = new GC_AdminBanService(this);
				outcome = m_AdminBanService.Request(requesterId, action, targetId, expectedIdentity, reason, duration, page, requestId);
			}
		}
		GC_AdminActionLogger.Log(requesterId, action, target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string ValidateAdminPlayerAction(string action, int targetId, UUID expectedIdentity, string reason, int duration, int page)
	{
		if (action != "player-kick" && action != "player-ban" && action != "player-unban" && action != "ban-list")
			return "rejected-unknown-action";
		if (action == "ban-list")
		{
			if (page < 0 || page > 100000)
				return "rejected-ban-page";
			return "";
		}
		if (!IsAdminAccountIdentity(expectedIdentity))
			return "rejected-account-identity";
		if (action == "player-unban")
			return "";
		if ((targetId > 0 || action == "player-kick") && (!GetGame().GetPlayerManager().IsPlayerConnected(targetId)
			|| SCR_PlayerIdentityUtils.GetPlayerIdentityId(targetId) != expectedIdentity))
			return "rejected-stale-player";
		reason.TrimInPlace();
		if (reason.IsEmpty() || reason.Length() > 256 || reason.Contains("\n") || reason.Contains("\r"))
			return "rejected-admin-reason";
		PlayerController localController = GetGame().GetPlayerController();
		if (action == "player-kick" && localController && localController.GetPlayerId() == targetId)
			return "unsupported-local-host-kick";
		if (action == "player-ban" && (duration < 0 || duration > 315360000))
			return "rejected-ban-duration";
		return "";
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsAdminAccountIdentity(string identity)
	{
		if (identity.Length() != 36)
			return false;
		string lower = identity;
		lower.ToLower();
		for (int index; index < 36; index++)
		{
			string character = lower.Substring(index, 1);
			if (index == 8 || index == 13 || index == 18 || index == 23)
			{
				if (character != "-")
					return false;
			}
			else if (!"0123456789abcdef".Contains(character))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void RecordModerationAction(string identity, string action, string reason, int expiresAt)
	{
		foreach (GC_AdminSessionPlayer record : m_aModerationHistory)
		{
			if (record.m_sIdentity != identity)
				continue;
			record.m_sAction = action;
			record.m_sReason = reason;
			record.m_iExpiresAt = expiresAt;
		}
		NotifyModerationChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Export only a server-observed session identity; never accept client-supplied names or platform IDs.
	string CopyAdminPlayerDetails(int requesterId, UUID identity)
	{
		if (!AuthorizeAdminAction(requesterId, "copy-player-details", identity))
			return "rejected-not-logged-in";
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		foreach (GC_AdminSessionPlayer record : m_aModerationHistory)
		{
			if (record.m_sIdentity != identity || !controller)
				continue;
			string name = record.m_sName;
			PlayerManager players = GetGame().GetPlayerManager();
			if (players.IsPlayerConnected(record.m_iPlayerId)
				&& SCR_PlayerIdentityUtils.GetPlayerIdentityId(record.m_iPlayerId) == identity)
				name = players.GetPlayerName(record.m_iPlayerId);
			// Keep the three labels unambiguous even if a displayed name contains line breaks.
			name.Replace("\r", " ");
			name.Replace("\n", " ");
			string steamId = record.m_sSteamId;
			if (steamId.IsEmpty())
				steamId = "Unavailable";
			controller.SendPlayerDetailsToClipboard("Name: " + name + "\nSteam ID: " + steamId + "\nArma UID: " + identity);
			GC_AdminActionLogger.Log(requesterId, "copy-player-details", identity, "dispatched-owner-clipboard");
			return "dispatched-owner-clipboard";
		}
		GC_AdminActionLogger.Log(requesterId, "copy-player-details", identity, "rejected-unknown-identity");
		return "rejected-unknown-identity";
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyModerationChanged()
	{
		if (!Replication.IsServer() || GetManager() != this)
			return;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
				controller.SendModerationChanged();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Admin-only presentation snapshot. Live membership is always re-queried.
	void SendModerationSession(int requesterId, int requestId)
	{
		if (!AuthorizeAdminAction(requesterId, "moderation-session", ""))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (!controller)
			return;
		array<string> rows = {};
		PlayerManager players = GetGame().GetPlayerManager();
		foreach (GC_AdminSessionPlayer record : m_aModerationHistory)
		{
			bool connected = players.IsPlayerConnected(record.m_iPlayerId)
				&& SCR_PlayerIdentityUtils.GetPlayerIdentityId(record.m_iPlayerId) == record.m_sIdentity;
			string status = "Disconnected";
			int playerId;
			if (connected)
			{
				playerId = record.m_iPlayerId;
				record.m_sName = players.GetPlayerName(playerId);
				status = "Connected";
			}
			if (record.m_sAction == "Banned" && (record.m_iExpiresAt == 0 || record.m_iExpiresAt > System.GetUnixTime()))
				status = "Banned (session record)";
			rows.Insert(playerId.ToString());
			rows.Insert(record.m_sName);
			rows.Insert(record.m_sIdentity);
			rows.Insert(record.m_sSteamId);
			rows.Insert(status);
			rows.Insert(record.m_sReason);
			string expiry;
			if (record.m_sAction == "Banned")
				expiry = record.m_iExpiresAt.ToString();
			rows.Insert(expiry);
		}
		controller.SendModerationRows(rows, requestId);
		GC_AdminActionLogger.Log(requesterId, "moderation-session", "", "returned");
	}

	//------------------------------------------------------------------------------------------------
	//! Shared presentation preflight. The server repeats it at execution time.
	SCR_AIGroup GetPlayableCharacterGroup(IEntity entity)
	{
		if (!entity || GetLobbyPhase() == SCR_EGameModeState.POSTGAME || m_Roster.GetSlotByEntity(entity))
			return null;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character || !character.GetCharacterController())
			return null;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (controller.IsDead() || controller.IsUnconscious() || controller.IsPlayerControlled())
			return null;
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity) > 0)
			return null;
		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!aiControl || !aiControl.GetAIAgent())
			return null;
		SCR_AIGroup group = SCR_AIGroup.Cast(aiControl.GetAIAgent().GetParentGroup());
		if (!group || !m_Roster.ContainsGroup(group) || !group.GetFaction())
			return null;
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!affiliation || affiliation.GetAffiliatedFaction() != group.GetFaction())
			return null;
		EntityPrefabData prefabData = entity.GetPrefabData();
		RplComponent replication = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!prefabData || prefabData.GetPrefabName().IsEmpty() || !replication || !replication.Id().IsValid())
			return null;
		return group;
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminMakePlayable(int requesterId, RplId entityId, RplId expectedGroup)
	{
		string target = string.Format("character=%1 group=%2", entityId, expectedGroup);
		if (!AuthorizeAdminAction(requesterId, "slot-make-playable", target))
			return "rejected-not-logged-in";
		int createdSlotId = -1;
		string outcome = MakeAdminCharacterPlayable(entityId, expectedGroup, createdSlotId);
		GC_AdminActionLogger.Log(requesterId, "slot-make-playable", target, string.Format("%1 slot=%2", outcome, createdSlotId));
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string MakeAdminCharacterPlayable(RplId entityId, RplId expectedGroup, out int createdSlotId)
	{
		createdSlotId = -1;
		RplComponent replication = RplComponent.Cast(Replication.FindItem(entityId));
		if (!replication || replication.IsProxy())
			return "rejected-stale-character";
		IEntity entity = replication.GetEntity();
		SCR_AIGroup group = GetPlayableCharacterGroup(entity);
		if (!group || Replication.FindItemId(group) != expectedGroup)
			return "rejected-incompatible-character";
		if (m_Roster.GetSlots().Count() >= 512)
			return "rejected-slot-limit";

		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		aiControl.DeactivateAI();
		if (aiControl.IsAIActivated())
			return "failed-ai-deactivation";
		int nextIndex;
		foreach (GC_Slot existing : m_Roster.GetSlotsForGroup(group))
		{
			if (existing.GetGroupSlotIndex() >= nextIndex)
				nextIndex = existing.GetGroupSlotIndex() + 1;
		}
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		GC_Slot slot = new GC_Slot();
		slot.SetID(m_Roster.AllocateSlotId());
		slot.SetGroup(group);
		slot.SetGroupSlotIndex(nextIndex);
		slot.SetAuthoredPrefab(prefab);
		slot.SetResolvedPrefab(prefab);
		slot.SetAdminSpawnPoint(new GC_SlotSpawnPoint(entity.GetOrigin(), entity.GetAngles()));
		slot.SetEntity(entity);
		m_Roster.AddSlot(slot);
		SubscribeToSlotEvents(slot);
		UpdateSlot(slot);
		NotifyGroupFactionUpdated(group);
		createdSlotId = slot.GetID();
		return "applied";
	}

	//------------------------------------------------------------------------------------------------
	//! The existing registered slot supplies the approved prefab and group.
	//! No client-supplied resource name is loaded or spawned.
	string ExecuteAdminCreateSlot(int requesterId, int templateSlotId, RplId expectedGroup, vector position)
	{
		string target = string.Format("template=%1 group=%2 position=%3", templateSlotId, expectedGroup, position);
		if (!AuthorizeAdminAction(requesterId, "slot-create", target))
			return "rejected-not-logged-in";
		int createdSlotId = -1;
		string outcome = CreateAdminSlot(templateSlotId, expectedGroup, position, createdSlotId);
		GC_AdminActionLogger.Log(requesterId, "slot-create", target, string.Format("%1 slot=%2", outcome, createdSlotId));
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string CreateAdminSlot(int templateSlotId, RplId expectedGroup, vector position, out int createdSlotId)
	{
		createdSlotId = -1;
		if (GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "rejected-create-state";
		GC_Slot source = GetSlot(templateSlotId);
		if (!source || source.GetGroupRplId() != expectedGroup)
			return "rejected-stale-template";
		SCR_AIGroup group = ResolveSlotGroup(source);
		if (!group || !m_Roster.ContainsGroup(group) || source.GetResolvedPrefab().IsEmpty())
			return "rejected-stale-template";

		vector minimum;
		vector maximum;
		GetGame().GetWorld().GetBoundBox(minimum, maximum);
		// Positive bounds checks reject NaN and infinity as well as outside-world positions.
		for (int axis; axis < 3; axis++)
		{
			if (!(position[axis] >= minimum[axis] && position[axis] <= maximum[axis]))
				return "rejected-spawn-position";
		}
		if (m_Roster.GetSlots().Count() >= 512)
			return "rejected-slot-limit";

		int nextIndex;
		foreach (GC_Slot existing : m_Roster.GetSlotsForGroup(group))
		{
			if (existing.GetGroupSlotIndex() >= nextIndex)
				nextIndex = existing.GetGroupSlotIndex() + 1;
		}
		GC_Slot slot = new GC_Slot();
		slot.SetID(m_Roster.AllocateSlotId());
		slot.SetGroup(group);
		slot.SetGroupSlotIndex(nextIndex);
		slot.SetAuthoredPrefab(source.GetAuthoredPrefab());
		slot.SetResolvedPrefab(source.GetResolvedPrefab());
		slot.SetVariantPolicy(source.GetVariantPolicy());
		slot.SetAdminSpawnPoint(new GC_SlotSpawnPoint(position, group.GetAngles()));
		// Stage successfully before publishing the slot. Failure leaves the roster unchanged.
		if (!SpawnSlotEntity(slot, true, group))
			return "failed-character-spawn";
		createdSlotId = slot.GetID();
		m_Roster.AddSlot(slot);
		SubscribeToSlotEvents(slot);
		UpdateSlot(slot);
		NotifyGroupFactionUpdated(group);
		return "applied";
	}

	//------------------------------------------------------------------------------------------------
	//! Occupied slots stay occupied when locked. Free/remove explicitly release
	//! possession first; removal never deletes the represented character.
	string ExecuteAdminSlotAction(int requesterId, int slotId, UUID expectedClaim, GC_EAdminSlotAction action)
	{
		string actionName = GetAdminSlotActionName(action);
		string target = string.Format("slot=%1 claim=%2", slotId, expectedClaim);
		if (!AuthorizeAdminAction(requesterId, actionName, target))
			return "rejected-not-logged-in";
		GC_Slot slot = GetSlot(slotId);
		string outcome = "rejected-stale-slot";
		if (slot && slot.GetUUID() == expectedClaim)
			outcome = ApplyAdminSlotAction(slot, action);
		GC_AdminActionLogger.Log(requesterId, actionName, target, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetAdminSlotActionName(GC_EAdminSlotAction action)
	{
		switch (action)
		{
			case GC_EAdminSlotAction.LOCK:
				return "slot-lock";
			case GC_EAdminSlotAction.UNLOCK:
				return "slot-unlock";
			case GC_EAdminSlotAction.PIN:
				return "slot-pin";
			case GC_EAdminSlotAction.UNPIN:
				return "slot-unpin";
			case GC_EAdminSlotAction.FREE:
				return "slot-free";
			case GC_EAdminSlotAction.REMOVE:
				return "slot-remove";
			case GC_EAdminSlotAction.RESPAWN_ORIGINAL:
				return "slot-respawn-original";
			case GC_EAdminSlotAction.RESPAWN_CURRENT:
				return "slot-respawn-current";
			case GC_EAdminSlotAction.FORCE_ENTRY:
				return "slot-force-entry";
			case GC_EAdminSlotAction.HEAL_OR_RESPAWN:
				return "slot-heal-respawn";
		}
		return "slot-unknown";
	}

	//------------------------------------------------------------------------------------------------
	//! Shared native healing for player tools and slot-targeted recovery.
	protected bool GC_HealAdminCharacter(ChimeraCharacter character)
	{
		if (!Replication.IsServer() || !character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return false;
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage || damage.GetState() == EDamageState.DESTROYED)
			return false;
		damage.FullHeal();
		array<HitZone> zones = {};
		damage.GetAllHitZonesInHierarchy(zones);
		if (zones.IsEmpty())
			return false;
		foreach (HitZone zone : zones)
		{
			if (zone && zone.GetDamageState() != EDamageState.UNDAMAGED)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected string ApplyAdminSlotAction(GC_Slot slot, GC_EAdminSlotAction action)
	{
		switch (action)
		{
			case GC_EAdminSlotAction.LOCK:
				slot.SetAdminLocked(true);
				return "applied";
			case GC_EAdminSlotAction.UNLOCK:
				slot.SetAdminLocked(false);
				return "applied";
			case GC_EAdminSlotAction.PIN:
				if (!slot.HasClaim())
					return "rejected-empty-slot";
				slot.SetAssignmentPinned(true);
				return "applied";
			case GC_EAdminSlotAction.UNPIN:
				slot.SetAssignmentPinned(false);
				return "applied";
			case GC_EAdminSlotAction.FREE:
				if (!ReleaseAdminSlotPlayer(slot))
					return "failed-spectator-transfer";
				slot.SetAdminLocked(false);
				return "applied";
			case GC_EAdminSlotAction.REMOVE:
				if (!ReleaseAdminSlotPlayer(slot))
					return "failed-spectator-transfer";
				UnsubscribeFromSlotEvents(slot);
				m_Roster.RemoveSlot(slot);
				Rpc(RpcDo_RemoveAdminSlot, slot.GetID());
				NotifyGroupFactionUpdated(slot.GetGroup());
				return "applied";
			case GC_EAdminSlotAction.RESPAWN_ORIGINAL:
				return RespawnAdminSlot(slot, false);
			case GC_EAdminSlotAction.RESPAWN_CURRENT:
				return RespawnAdminSlot(slot, true);
			case GC_EAdminSlotAction.HEAL_OR_RESPAWN:
				if (GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
					return "rejected-respawn-state";
				if (slot.IsKIA())
					return RespawnAdminSlot(slot, slot.GetEntity() != null);
				if (GC_HealAdminCharacter(ChimeraCharacter.Cast(slot.GetEntity())))
					return "applied";
				return "failed-character-heal";
			case GC_EAdminSlotAction.FORCE_ENTRY:
				if (GetLobbyPhase() != SCR_EGameModeState.GAME || slot.IsKIA() || slot.GetPlayerId() <= 0)
					return "rejected-entry-state";
				if (!SpawnSlotEntity(slot, true) || !AssignSlotEntity(slot.GetPlayerId(), slot))
					return "failed-character-entry";
				return "applied";
		}
		return "rejected-unknown-action";
	}

	//------------------------------------------------------------------------------------------------
	protected bool ReleaseAdminSlotPlayer(GC_Slot slot)
	{
		int playerId = slot.GetPlayerId();
		if (playerId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(playerId))
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller)
				return false;
			SetSpectator(playerId, true);
			IEntity body = controller.GetControlledEntity();
			if (!body || !body.FindComponent(GC_LobbySpectatorBodyComponent))
				return false;
			SCR_AIGroup group = GetPlayerGroup(playerId);
			if (group)
				group.RemovePlayer(playerId);
			SynchronizePlayerNativeMembership(playerId);
			SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
		}
		slot.ReleaseClaim();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Stage a replacement before changing the claim or deleting the old body.
	//! A failed spawn/possession keeps the original slot and character intact.
	protected string RespawnAdminSlot(GC_Slot slot, bool atCurrentPosition)
	{
		int playerId = slot.GetPlayerId();
		SCR_AIGroup group = ResolveSlotGroup(slot);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!group || GetLobbyPhase() == SCR_EGameModeState.POSTGAME
			|| (playerId > 0 && (!controller || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))))
			return "rejected-respawn-state";
		IEntity previous = slot.GetEntity();
		if (atCurrentPosition && !previous)
			return "rejected-no-current-character";

		GC_Slot replacement = new GC_Slot();
		replacement.SetID(slot.GetID());
		replacement.SetAdminSpawnPoint(slot.GetAdminSpawnPoint());
		replacement.SetGroup(group);
		replacement.SetGroupSlotIndex(slot.GetGroupSlotIndex());
		replacement.SetAuthoredPrefab(slot.GetAuthoredPrefab());
		ResourceName prefab = slot.GetResolvedPrefab();
		if (slot.GetVariantPolicy() == GC_SlotVariantPolicy.REROLL_ON_RESPAWN)
			prefab = SCR_EditableEntityComponentClass.GetRandomVariant(slot.GetAuthoredPrefab());
		if (prefab.IsEmpty())
			return "failed-prefab-resolution";
		replacement.SetResolvedPrefab(prefab);
		EntitySpawnParams spawnOverride;
		if (atCurrentPosition)
		{
			spawnOverride = new EntitySpawnParams();
			spawnOverride.TransformMode = ETransformMode.WORLD;
			previous.GetWorldTransform(spawnOverride.Transform);
		}
		if (!SpawnSlotEntity(replacement, true, group, spawnOverride))
			return "failed-character-spawn";
		IEntity character = replacement.GetEntity();
		if (playerId > 0 && GetLobbyPhase() == SCR_EGameModeState.GAME && !AssignSlotEntity(playerId, replacement))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return "failed-character-possession";
		}

		slot.SetResolvedPrefab(prefab);
		slot.SetEntity(character);
		if (playerId > 0)
		{
			slot.SetState(GC_SlotState.LOCKED);
			RefreshPlayerRadio(playerId);
		}
		else if (slot.HasClaim())
			slot.SetState(GC_SlotState.DISCONNECTED);
		else
			slot.SetState(GC_SlotState.FREE);
		if (previous && previous != character)
			SCR_EntityHelper.DeleteEntityAndChildren(previous);
		return "applied";
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminGroupLock(int requesterId, RplId groupId, bool locked)
	{
		string target = string.Format("group=%1 locked=%2", groupId, locked);
		if (!AuthorizeAdminAction(requesterId, "slot-group-lock", target))
			return "rejected-not-logged-in";
		int changed;
		foreach (GC_Slot slot : m_Roster.GetSlots())
		{
			if (groupId != RplId.Invalid() && slot.GetGroupRplId() != groupId)
				continue;
			slot.SetAdminLocked(locked);
			changed++;
		}
		string outcome = "applied";
		if (changed == 0)
			outcome = "rejected-no-slots";
		GC_AdminActionLogger.Log(requesterId, "slot-group-lock", target, string.Format("%1 slots=%2", outcome, changed));
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RemoveAdminSlot(int slotId)
	{
		if (Replication.IsServer())
			return;
		GC_Slot slot = GetSlot(slotId);
		if (!slot)
			return;
		UnsubscribeFromSlotEvents(slot);
		m_Roster.RemoveSlot(slot);
		NotifyGroupFactionUpdated(slot.GetGroup());
	}

	//------------------------------------------------------------------------------------------------
	//! Call only on authority with a requester derived from the owning controller.
	bool AuthorizeAdminAction(int requesterId, string action, string target)
	{
		if (GetManager() != this || m_bShutdown || !Replication.IsServer() || requesterId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(requesterId) || !SCR_Global.IsAdmin(requesterId))
		{
			GC_AdminActionLogger.Log(requesterId, action, target, "rejected-not-logged-in");
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Expected identity prevents a stale menu from targeting a recycled player ID.
	bool TrySetAdminVoice(int requesterId, int targetId, UUID expectedIdentity, bool priorityControl, bool enabled)
	{
		string action = "voice-mute";
		if (priorityControl)
			action = "voice-priority";
		string target = string.Format("player=%1 identity=%2 enabled=%3", targetId, expectedIdentity, enabled);
		if (!AuthorizeAdminAction(requesterId, action, target))
			return false;

		GC_VoiceChannelAssignment assignment = m_VoiceState.Get(targetId);
		if (!assignment || expectedIdentity.IsEmpty() || assignment.GetPlayerIdentity() != expectedIdentity || !GetGame().GetPlayerManager().IsPlayerConnected(targetId))
		{
			GC_AdminActionLogger.Log(requesterId, action, target, "rejected-stale-target");
			return false;
		}

		if (priorityControl)
			assignment.SetPrioritySpeaker(enabled);
		else
			assignment.SetAdminMuted(enabled);
		if (!assignment.IsPrioritySpeaker() || assignment.IsAdminMuted())
		{
			assignment.SetPriorityTransmitting(false);
			m_mPriorityVoiceExpiry.Remove(targetId);
		}
		PublishVoiceModeration(assignment);
		GC_AdminActionLogger.Log(requesterId, action, target, "applied");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool CanTransmitLobbyVoice(int playerId)
	{
		return !m_bShutdown && m_VoiceState.CanTransmit(playerId, SCR_Global.IsAdmin(playerId));
	}

	//------------------------------------------------------------------------------------------------
	//! Only the server's native VoN-used event acquires/renews a priority lease.
	void ObserveLobbyVoiceTransmission(int playerId)
	{
		if (m_bShutdown || !Replication.IsServer())
			return;
		GC_VoiceChannelAssignment assignment = m_VoiceState.Get(playerId);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!assignment || !assignment.IsPrioritySpeaker() || assignment.IsAdminMuted() || !controller || !controller.HasAuthorizedLobbyCapture())
			return;

		bool startExpiry = m_mPriorityVoiceExpiry.IsEmpty();
		m_mPriorityVoiceExpiry.Set(playerId, GetGame().GetWorld().GetWorldTime() + 750);
		if (!assignment.IsPriorityTransmitting())
		{
			assignment.SetPriorityTransmitting(true);
			PublishVoiceModeration(assignment);
		}
		if (startExpiry)
		{
			GetGame().GetCallqueue().Remove(ExpirePriorityVoice);
			GetGame().GetCallqueue().CallLater(ExpirePriorityVoice, 250);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ReleaseLobbyVoiceTransmission(int playerId)
	{
		if (!Replication.IsServer())
			return;
		m_mPriorityVoiceExpiry.Remove(playerId);
		GC_VoiceChannelAssignment assignment = m_VoiceState.Get(playerId);
		if (assignment && assignment.IsPriorityTransmitting())
		{
			assignment.SetPriorityTransmitting(false);
			PublishVoiceModeration(assignment);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ExpirePriorityVoice()
	{
		if (m_bShutdown || !Replication.IsServer())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		for (int i = m_mPriorityVoiceExpiry.Count() - 1; i >= 0; i--)
		{
			int playerId = m_mPriorityVoiceExpiry.GetKey(i);
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (m_mPriorityVoiceExpiry.GetElement(i) <= now || !controller || !controller.HasAuthorizedLobbyCapture())
				ReleaseLobbyVoiceTransmission(playerId);
		}
		if (!m_mPriorityVoiceExpiry.IsEmpty())
			GetGame().GetCallqueue().CallLater(ExpirePriorityVoice, 250);
	}

	//------------------------------------------------------------------------------------------------
	protected void PublishVoiceModeration(GC_VoiceChannelAssignment assignment)
	{
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnVoiceAssignmentChanged.Invoke(assignment.GetPlayerId());
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
					chatFrequency = m_VoiceState.GetFrequency("faction:" + assignment.GetFactionKey());
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

	//------------------------------------------------------------------------------------------------
	//! Destination existence is independent of the moved player's faction or slot.
	//! Client-local interaction selection, never a permission or gameplay owner.
	protected int m_iAdminSelectedPlayer = -1;
	protected UUID m_AdminSelectedIdentity;
	ref ScriptInvoker m_OnAdminSelectionChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void SelectAdminPlayer(int playerId)
	{
		PlayerController local = GetGame().GetPlayerController();
		if (GetManager() != this || !local || !SCR_Global.IsAdmin(local.GetPlayerId()))
			playerId = -1;
		if (playerId > 0 && !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			playerId = -1;
		m_iAdminSelectedPlayer = playerId;
		m_AdminSelectedIdentity = GetPlayerVoiceIdentity(playerId);
		m_OnAdminSelectionChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	int GetAdminSelectedPlayer()
	{
		PlayerController local = GetGame().GetPlayerController();
		if (GetManager() != this || !local || !SCR_Global.IsAdmin(local.GetPlayerId())
			|| GC_LobbyPhase.IsAtOrAfter(GetLobbyPhase(), SCR_EGameModeState.GAME)
			|| m_AdminSelectedIdentity.IsEmpty() || GetPlayerVoiceIdentity(m_iAdminSelectedPlayer) != m_AdminSelectedIdentity
			|| !GetGame().GetPlayerManager().IsPlayerConnected(m_iAdminSelectedPlayer))
		{
			m_iAdminSelectedPlayer = -1;
			m_AdminSelectedIdentity = string.Empty;
		}
		return m_iAdminSelectedPlayer;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAdminVoiceDestinationValid(GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, UUID personalOwnerIdentity)
	{
		switch (channelType)
		{
			case GC_VoiceChannelType.GLOBAL:
				return factionKey.IsEmpty() && groupRplId == RplId.Invalid() && personalOwnerIdentity.IsEmpty();
			case GC_VoiceChannelType.FACTION:
				return GetRegisteredFaction(factionKey) && groupRplId == RplId.Invalid() && personalOwnerIdentity.IsEmpty();
			case GC_VoiceChannelType.GROUP:
			{
				SCR_AIGroup group = GetRegisteredGroupByRplId(groupRplId);
				return group && group.GetFaction() && group.GetFaction().GetFactionKey() == factionKey && personalOwnerIdentity.IsEmpty();
			}
			case GC_VoiceChannelType.PERSONAL:
				return factionKey.IsEmpty() && groupRplId == RplId.Invalid() && m_VoiceState.HasPersonalChannel(personalOwnerIdentity) && GetConnectedPlayerIdByIdentity(personalOwnerIdentity) > 0;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	string ExecuteAdminVoiceChannel(int requesterId, int targetId, UUID expectedIdentity,
		GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, UUID personalOwnerIdentity)
	{
		string context = string.Format("player=%1 identity=%2 type=%3 faction=%4 group=%5 personalOwner=%6",
			targetId, expectedIdentity, channelType, factionKey, groupRplId, personalOwnerIdentity);
		if (!AuthorizeAdminAction(requesterId, "voice-channel", context))
			return "rejected-not-logged-in";
		string outcome;
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(targetId);
		SCR_EGameModeState phase = GetLobbyPhase();
		// Admins may join a room themselves in any phase; moving other players
		// retains the existing lobby-only rule.
		if (requesterId != targetId && phase != SCR_EGameModeState.PREGAME
			&& phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
			outcome = "rejected-lobby-voice-inactive";
		else if (!assignment || expectedIdentity.IsEmpty() || assignment.GetPlayerIdentity() != expectedIdentity || !GetGame().GetPlayerManager().IsPlayerConnected(targetId))
			outcome = "rejected-stale-target";
		else if (!IsAdminVoiceDestinationValid(channelType, factionKey, groupRplId, personalOwnerIdentity))
			outcome = "rejected-voice-destination";
		else
		{
			context += string.Format(" previousType=%1 previousFaction=%2 previousGroup=%3 previousOwner=%4",
				assignment.GetChannelType(), assignment.GetFactionKey(), assignment.GetGroupRplId(), assignment.GetPersonalOwnerIdentity());
			outcome = "failed-voice-channel";
			if (SetPlayerVoiceChannel(targetId, channelType, factionKey, groupRplId, personalOwnerIdentity, true, true))
				outcome = "applied";
		}
		GC_AdminActionLogger.Log(requesterId, "voice-channel", context, outcome);
		return outcome;
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
				return SetPlayerVoiceChannel(playerId, channelType, "", RplId.Invalid(), "", true);
			case GC_VoiceChannelType.FACTION:
			{
				Faction requestedFaction = GetRegisteredFaction(factionKey);
				if (!requestedFaction || GetPlayerFaction(playerId) != requestedFaction)
					return false;
				return SetPlayerVoiceChannel(playerId, channelType, factionKey, RplId.Invalid(), "", true);
			}
			case GC_VoiceChannelType.GROUP:
				return TryJoinGroupFromVoiceHeader(playerId, groupRplId);
			case GC_VoiceChannelType.PERSONAL:
				if (!CanAccessPersonalVoiceChannel(playerId, personalOwnerIdentity))
					return false;
				return SetPlayerVoiceChannel(playerId, channelType, "", RplId.Invalid(), personalOwnerIdentity, true);
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

	protected bool SetPlayerVoiceChannel(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "", bool explicitChange = false, bool adminAssigned = false)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment previousAssignment = m_VoiceState.Get(playerId);
		if (previousAssignment && previousAssignment.IsAdminAssigned() && !explicitChange
			&& IsAdminVoiceDestinationValid(previousAssignment.GetChannelType(), previousAssignment.GetFactionKey(), previousAssignment.GetGroupRplId(), previousAssignment.GetPersonalOwnerIdentity()))
			return true;
		if (previousAssignment)
			previousAssignment.SetAdminAssigned(adminAssigned);
		ReleaseLobbyVoiceTransmission(playerId);
		UUID playerIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		bool wasOpenPersonalOwner = previousAssignment && previousAssignment.GetChannelType() == GC_VoiceChannelType.PERSONAL && previousAssignment.GetPersonalOwnerIdentity() == playerIdentity;

		GC_VoiceChannelAssignment assignment;
		if (!m_VoiceState.SetChannel(playerId, channelType, factionKey, groupRplId, personalOwnerIdentity, assignment))
			return assignment != null;

		assignment.SetAdminAssigned(adminAssigned);
		PrintFormat("GC | Voice assignment changed: player=%1 type=%2 faction=%3 group=%4 personalOwner=%5", playerId, channelType, factionKey, groupRplId, personalOwnerIdentity);
		RefreshPlayerRadio(playerId);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnVoiceAssignmentChanged.Invoke(playerId);

		if (wasOpenPersonalOwner && (channelType != GC_VoiceChannelType.PERSONAL || personalOwnerIdentity != playerIdentity))
			ClosePersonalVoiceChannel(playerIdentity);
		return true;
	}

	//! Admin-owned rooms permit voluntary entry only by logged-in admins.
	//! Other personal rooms require the owner to be inside and both players to
	//! share a faction scope (or both be unassigned spectators).
	bool CanAccessPersonalVoiceChannel(int playerId, UUID ownerIdentity)
	{
		if (playerId <= 0 || ownerIdentity.IsEmpty() || !m_VoiceState.HasPersonalChannel(ownerIdentity))
			return false;

		int ownerPlayerId = GetConnectedPlayerIdByIdentity(ownerIdentity);
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId) || ownerPlayerId <= 0)
			return false;
		if (m_VoiceState.GetPlayerIdentity(playerId) == ownerIdentity)
			return true;

		// Admin-owned rooms require an authenticated admin for voluntary entry.
		// Server-authorized moves use ExecuteAdminVoiceChannel instead.
		if (SCR_Global.IsAdmin(ownerPlayerId))
			return SCR_Global.IsAdmin(playerId);

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

	protected void ClosePersonalVoiceChannel(UUID ownerIdentity, bool ownerDisconnected = false)
	{
		array<int> members = m_VoiceState.GetPersonalChannelMembers(ownerIdentity);
		foreach (int memberPlayerId : members)
		{
			if (m_VoiceState.GetPlayerIdentity(memberPlayerId) != ownerIdentity)
				SetPlayerVoiceChannel(memberPlayerId, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", ownerDisconnected);
		}
	}

	protected void RemovePlayerVoiceAssignment(int playerId)
	{
		if (!Replication.IsServer())
			return;

		ReleaseLobbyVoiceTransmission(playerId);
		m_mAdminMessageTimes.Remove(playerId);
		ClosePersonalVoiceChannel(m_VoiceState.GetPlayerIdentity(playerId), true);
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

	//------------------------------------------------------------------------------------------------
	//! Shared by player menus and the authority; membership is always queried live.
	bool CanPlayerJoinGroupVoiceChannel(int playerId, RplId groupRplId)
	{
		if (GetManager() != this || playerId <= 0 || !m_bVoiceGroupHeaderSwitchesGroup || groupRplId == RplId.Invalid())
			return false;

		SCR_AIGroup targetGroup = GetRegisteredGroupByRplId(groupRplId);
		if (!targetGroup)
			return false;

		Faction playerFaction = GetPlayerFaction(playerId);
		return playerFaction && playerFaction == targetGroup.GetFaction();
	}

	//------------------------------------------------------------------------------------------------
	bool TryJoinGroupFromVoiceHeader(int playerId, RplId groupRplId)
	{
		if (!Replication.IsServer())
		{
			Print("GC | Voice group join rejected: request reached a non-authority instance", level: LogLevel.ERROR);
			return false;
		}

		if (!CanPlayerJoinGroupVoiceChannel(playerId, groupRplId))
		{
			PrintFormat("GC | Voice group join rejected: player=%1 group=%2 channel is unavailable", playerId, groupRplId, level: LogLevel.WARNING);
			return false;
		}

		SCR_AIGroup targetGroup = GetRegisteredGroupByRplId(groupRplId);
		// Voice selection must not change native group membership or the lobby slot.
		return SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, targetGroup.GetFaction().GetFactionKey(), groupRplId, "", true);
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

		return ApplyPlayerSlotAssignment(playerId, targetSlot, true, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SynchronizePlayerNativeMembership(int playerId)
	{
		PlayerController player = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!player)
			return;
		SCR_PlayerControllerGroupComponent membership = SCR_PlayerControllerGroupComponent.Cast(player.FindComponent(SCR_PlayerControllerGroupComponent));
		if (membership)
			membership.GC_SynchronizeLobbyMembership();
	}

	//------------------------------------------------------------------------------------------------
	//! Canonical claim/group/voice transition for both self-service and admin requests.
	protected bool ApplyPlayerSlotAssignment(int playerId, GC_Slot targetSlot, bool toggleCurrent, bool immediateCatchUp)
	{
		SCR_EGameModeState phase = GetLobbyPhase();
		int slotId = targetSlot.GetID();
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
			if (!toggleCurrent)
				return true;
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);
			SynchronizePlayerNativeMembership(playerId);

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

		SynchronizePlayerNativeMembership(playerId);
		if (currentSlot && currentSlot != targetSlot)
			ReleaseSlotClaim(currentSlot);

		targetSlot.SetPlayer(playerId);
		// Binding a first-join reservation must not revive a character killed
		// while its claimant was offline.
		if (!targetSlot.IsKIA())
		{
			if (phase != SCR_EGameModeState.SLOTTING && targetSlot.GetEntity())
				targetSlot.SetState(GC_SlotState.LOCKED);
			else
				targetSlot.SetState(GC_SlotState.RESERVED);
		}
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, targetGroup.GetFaction().GetFactionKey(), Replication.FindItemId(targetGroup));
		if (phase == SCR_EGameModeState.BRIEFING && !targetSlot.GetEntity())
			SpawnSlotEntity(targetSlot, false);
		else if (phase == SCR_EGameModeState.GAME)
		{
			if (immediateCatchUp)
				CatchUpPlayer(playerId);
			else
				QueuePlayerCatchUp(playerId);
		}

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

		GC_Slot pinnedSlot = GetSlotByPlayerId(playerId);
		if (pinnedSlot && pinnedSlot.IsAssignmentPinned())
			return false;
		if (targetSlot.IsAdminLocked() && targetSlot.GetPlayerId() != playerId)
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
			return phase == SCR_EGameModeState.SLOTTING && !IsFactionSwitchLocked();

		SCR_AIGroup targetGroup = ResolveSlotGroup(targetSlot);
		if (!targetGroup || !targetGroup.GetFaction())
			return false;

		if (!CanPlayerJoinBalancedFaction(playerId, targetGroup.GetFaction()))
			return false;
		if (phase == SCR_EGameModeState.SLOTTING && !IsFactionSwitchLocked())
			return true;

		GC_Slot currentSlot = GetSlotByPlayerId(playerId);
		if (currentSlot && currentSlot.IsKIA())
			return false;

		Faction lockedFaction = GetPlayerFaction(playerId);
		return !IsFactionSwitchLocked() || !lockedFaction || lockedFaction == targetGroup.GetFaction();
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

	//------------------------------------------------------------------------------------------------
	//! POSTGAME still serves roster and briefing views. Only world termination
	//! releases their data; the base OnGameModeEnd callback is a phase event.
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
		if (m_AdminBanService)
			m_AdminBanService.Shutdown();
		GetGame().GetCallqueue().Remove(ExpirePriorityVoice);
		m_mPriorityVoiceExpiry.Clear();
		m_mAdminMessageTimes.Clear();
		m_aAdminSessionHistory.Clear();
		m_aModerationHistory.Clear();
		m_mAdminSessionLabels.Clear();
		GC_LobbyPlayerController localController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (localController)
			localController.ShutdownLobbyPresentation();
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

		GC_LobbyManager manager = GC_LobbyManager.Cast(lobbyGameMode.FindComponent(GC_LobbyManager));
		if (!manager || manager.m_bShutdown)
			return null;

		return manager;
	}
}
