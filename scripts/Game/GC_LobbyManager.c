class GC_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_LobbyManager : SCR_BaseGameModeComponent
{
    protected const ResourceName SPECTATOR_PREFAB = "{8A578DE95771CC1D}Prefabs/Characters/Core/Character_Spectator.et";
    protected const int JIP_REFERENCE_RETRY_MS = 1000;

    //! Game-mode setting: group headers in the voice list may be selectable
    //! without changing the player's lobby slot or base-game group.
    [Attribute("1", UIWidgets.CheckBox, "Allow group voice-channel selection")]
    protected bool m_bVoiceGroupHeaderSwitchesGroup;
    
    [Attribute("", UIWidgets.Object, "Formations for player groups")]
    protected ref array<ref GC_PlayerFormationDefinition> m_aFormations;

	[Attribute("", UIWidgets.Object, "Lobby2 briefing and marker catalog")]
	protected ref GC_LobbyBriefingCatalog m_BriefingCatalog;

	protected ref GC_LobbyRoster m_Roster = new GC_LobbyRoster();
	protected ref GC_LobbyVoiceState m_VoiceState = new GC_LobbyVoiceState();
    ref ScriptInvoker<Faction> m_OnFactionUpdated = new ScriptInvoker<Faction>();
    //! Signals that connected-player or authoritative group membership data changed.
    //! Consumers re-query PlayerManager and SCR_GroupsManagerComponent instead of
    //! maintaining a second player roster in Lobby2.
    ref ScriptInvoker<int> m_OnPlayerRegistryUpdated = new ScriptInvoker<int>();
    //! Signals that the complete local briefing view changed. Consumers re-query
    //! this manager and current authoritative faction state.
    ref ScriptInvoker<int> m_OnBriefingsChanged = new ScriptInvoker<int>();
	ref ScriptInvoker<int> m_OnBriefingMarkersChanged = new ScriptInvoker<int>();
	ref ScriptInvoker<GC_ELobbyPhase> m_OnLobbyPhaseChanged = new ScriptInvoker<GC_ELobbyPhase>();
	protected ref GC_LobbyPhaseMachine m_PhaseMachine = new GC_LobbyPhaseMachine();
	protected bool m_bShutdown;

	//! Re-evaluate briefing presentation on every machine when the authoritative
	//! base-game phase changes. The phase itself remains owned and replicated by
	//! SCR_BaseGameMode.
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		string role = "proxy";
		if (Replication.IsServer())
			role = "authority";

		PrintFormat("GC | Phase | base state event: base=%1 lobbyBefore=%2 role=%3", GetBaseStateName(state), GetPhaseName(GetLobbyPhase()), role);

		// Keep the Lobby2 phase aligned if trusted server game logic invokes the
		// native base-game transition API directly.
		if (state == SCR_EGameModeState.GAME && GetLobbyPhase() == GC_ELobbyPhase.BRIEFING)
		{
			if (Replication.IsServer())
				PublishLobbyPhase(GC_ELobbyPhase.GAME);
			else
				ApplyLobbyPhase(GC_ELobbyPhase.GAME);
		}
		else if (state == SCR_EGameModeState.POSTGAME && GetLobbyPhase() == GC_ELobbyPhase.GAME)
		{
			if (Replication.IsServer())
				PublishLobbyPhase(GC_ELobbyPhase.POSTGAME);
			else
				ApplyLobbyPhase(GC_ELobbyPhase.POSTGAME);
		}

		m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
		m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
	}

	GC_ELobbyPhase GetLobbyPhase()
	{
		return m_PhaseMachine.GetCurrent();
	}

	static string GetPhaseName(GC_ELobbyPhase phase)
	{
		return GC_LobbyPhaseMachine.GetName(phase);
	}

	static string GetBaseStateName(SCR_EGameModeState state)
	{
		switch (state)
		{
			case SCR_EGameModeState.PREGAME: return "PREGAME";
			case SCR_EGameModeState.GAME: return "GAME";
			case SCR_EGameModeState.POSTGAME: return "POSTGAME";
		}

		return "UNKNOWN";
	}

	//! Validates and applies an admin-owned request on the server authority.
	bool TryAdvanceLobbyPhase(int playerId, GC_ELobbyPhase requestedPhase)
	{
		if (m_bShutdown)
		{
			Print("GC | Phase | request rejected: Lobby2 manager is shutting down", LogLevel.WARNING);
			return false;
		}

		if (!Replication.IsServer())
		{
			Print("GC | Phase | request rejected: handler is not running on authority", LogLevel.ERROR);
			return false;
		}

		PrintFormat("GC | Phase | request received: player=%1 current=%2 requested=%3 requestedValue=%4", playerId, GetPhaseName(GetLobbyPhase()), GetPhaseName(requestedPhase), requestedPhase);

		if (playerId <= 0)
		{
			PrintFormat("GC | Phase | request rejected: invalid player id=%1", playerId, level: LogLevel.WARNING);
			return false;
		}

		if (!GC_LobbyPhaseMachine.IsValid(requestedPhase))
		{
			PrintFormat("GC | Phase | request rejected: invalid target value=%1", requestedPhase, level: LogLevel.WARNING);
			return false;
		}

		if (!SCR_Global.IsAdmin(playerId))
		{
			PrintFormat("GC | Phase | request rejected: player=%1 is not an admin", playerId, level: LogLevel.WARNING);
			return false;
		}

		GC_ELobbyPhase nextPhase = GC_LobbyPhaseMachine.GetNext(GetLobbyPhase());
		if (!m_PhaseMachine.CanAdvanceTo(requestedPhase))
		{
			PrintFormat("GC | Phase | request rejected: player=%1 current=%2 requested=%3 expected=%4", playerId, GetPhaseName(GetLobbyPhase()), GetPhaseName(requestedPhase), GetPhaseName(nextPhase), level: LogLevel.WARNING);
			return false;
		}

		if (requestedPhase == GC_ELobbyPhase.POSTGAME)
			return EnterPostGame(null);

		SCR_BaseGameMode gameMode = GetGameMode();
		if (!gameMode)
		{
			Print("GC | Phase | request rejected: SCR_BaseGameMode is unavailable", LogLevel.ERROR);
			return false;
		}

		if (requestedPhase == GC_ELobbyPhase.GAME)
		{
			if (gameMode.GetState() != SCR_EGameModeState.PREGAME)
			{
				PrintFormat("GC | Phase | GAME transition rejected: expected base=PREGAME actual=%1", GetBaseStateName(gameMode.GetState()), level: LogLevel.ERROR);
				return false;
			}

			PrintFormat("GC | Phase | invoking native StartGameMode: lobby=%1 base=%2", GetPhaseName(GetLobbyPhase()), GetBaseStateName(gameMode.GetState()));
			gameMode.StartGameMode();
			if (gameMode.GetState() != SCR_EGameModeState.GAME)
			{
				PrintFormat("GC | Phase | native StartGameMode failed: base=%1", GetBaseStateName(gameMode.GetState()), level: LogLevel.ERROR);
				return false;
			}

			// OnGameStateChanged normally publishes this synchronously.
			PublishLobbyPhase(GC_ELobbyPhase.GAME);
			return true;
		}

		PublishLobbyPhase(GC_ELobbyPhase.BRIEFING);
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

		if (GetLobbyPhase() != GC_ELobbyPhase.GAME)
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

		// OnGameStateChanged normally publishes this synchronously.
		PublishLobbyPhase(GC_ELobbyPhase.POSTGAME);
		return true;
	}

	protected void PublishLobbyPhase(GC_ELobbyPhase phase)
	{
		if (!Replication.IsServer() || !ApplyLobbyPhase(phase))
			return;

		Rpc(RpcDo_SetLobbyPhase, phase);
	}

	protected bool ApplyLobbyPhase(GC_ELobbyPhase phase)
	{
		GC_ELobbyPhase previousPhase;
		if (!m_PhaseMachine.Apply(phase, previousPhase))
			return false;

		SCR_BaseGameMode gameMode = GetGameMode();
		string baseState = "UNAVAILABLE";
		if (gameMode)
			baseState = GetBaseStateName(gameMode.GetState());

		string role = "proxy";
		if (Replication.IsServer())
			role = "authority";

		PrintFormat("GC | Phase | applied: role=%1 lobby=%2 -> %3 base=%4", role, GetPhaseName(previousPhase), GetPhaseName(phase), baseState);
		m_OnLobbyPhaseChanged.Invoke(phase);
		m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
		m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
		return true;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetLobbyPhase(GC_ELobbyPhase phase)
	{
		if (Replication.IsServer())
			return;

		PrintFormat("GC | Phase RPC | broadcast received: target=%1 value=%2", GetPhaseName(phase), phase);
		ApplyLobbyPhase(phase);
	}

    override void OnPlayerAuditSuccess(int playerId)
    {
        PrintFormat("GC | OnPlayerAuditSuccess: %1, connected!", playerId);
    
        //Check if player is already existing in world

        //If not setup player in spectator mode
        SetSpectator(playerId, true);
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
        m_OnPlayerRegistryUpdated.Invoke(playerId);
    }

    override void OnPlayerConnected(int playerId)
    {
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
        m_OnPlayerRegistryUpdated.Invoke(playerId);
        super.OnPlayerConnected(playerId);
    }

    override void OnPlayerRegistered(int playerId)
    {
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
        m_OnPlayerRegistryUpdated.Invoke(playerId);
        super.OnPlayerRegistered(playerId);
    }

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
        RemovePlayerVoiceAssignment(playerId);
        m_OnPlayerRegistryUpdated.Invoke(playerId);
        super.OnPlayerDisconnected(playerId, cause, timeout);
    }
    
    //! Make the player enter spectator mode from the server
    void SetSpectator(int playerId, bool state)
    {
        if (!Replication.IsServer())
            return;

        if (state)
        {
            EntitySpawnParams params = new EntitySpawnParams();
            params.Transform[3] = "1194.457 39 2723.129";
            IEntity spectator = GetGame().SpawnEntityPrefab(Resource.Load(SPECTATOR_PREFAB), null, params);

            GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
            if (playerController)
                playerController.SetLobbySpectator(spectator);
        }
        else
        {
            
        }

    }

    void RegisterGroup(SCR_AIGroup group)
    {
        PrintFormat("GC | RegisterGroup: %1", group);
        if (!group)
            return;
        
        bool wasEmpty;
        if (!m_Roster.AddGroup(group, wasEmpty))
            return;

        SubscribeToGroupEvents(group);

        if (wasEmpty)
            SubscribeToGroupPlayerEvents();

        Faction faction = group.GetFaction();
        bool factionWasRegistered = faction && m_Roster.GetFactions().Contains(faction);
        if (faction)
            RegisterFaction(faction.GetFactionKey());

        if (Replication.IsServer())
            RegisterGroupMembers(group);

        // RegisterFaction already notifies when this is a new faction. Notify
        // again only for an existing faction so dynamic faction/group views
        // can discover an additional group without duplicating the new-faction
        // callback.
        if (faction && factionWasRegistered)
            m_OnFactionUpdated.Invoke(faction);
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
            m_OnFactionUpdated.Invoke(faction);

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
            m_Roster.AddFaction(faction);

        // The event does not identify which group changed. Notify all registered
        // faction views so each view re-queries the authoritative group state.
        foreach (Faction registeredFaction : m_Roster.GetFactions())
        {
            if (registeredFaction)
                m_OnFactionUpdated.Invoke(registeredFaction);
        }
    }

    protected void OnGroupPlayerAdded(SCR_AIGroup group, int playerId)
    {
        if (Replication.IsServer() && group && group.GetFaction())
            SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, group.GetFaction().GetFactionKey(), Replication.FindItemId(group));

        m_OnPlayerRegistryUpdated.Invoke(playerId);
        QueueAllRegisteredFactionUpdates();
    }

    protected void OnGroupPlayerRemoved(SCR_AIGroup group, int playerId)
    {
        if (Replication.IsServer() && group)
        {
            GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
            if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.GROUP && assignment.GetGroupRplId() == Replication.FindItemId(group))
                SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GLOBAL);
        }

        m_OnPlayerRegistryUpdated.Invoke(playerId);
        QueueAllRegisteredFactionUpdates();
    }

    protected void NotifyGroupFactionUpdated(SCR_AIGroup group)
    {
        if (!group || !m_Roster.GetGroups().Contains(group))
            return;

        Faction faction = group.GetFaction();
        if (faction)
            m_OnFactionUpdated.Invoke(faction);
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

        // Slot assignment and base-group membership are changed together, but
        // their callbacks can run before both states have settled. Refresh all
        // dependent views on the next call-queue tick so they query one
        // consistent authoritative state.
        QueueAllRegisteredFactionUpdates();
    }

    protected void QueueAllRegisteredFactionUpdates()
    {
        GetGame().GetCallqueue().Remove(NotifyAllRegisteredFactions);
        GetGame().GetCallqueue().CallLater(NotifyAllRegisteredFactions);
    }

    protected void NotifyAllRegisteredFactions()
    {
        // A slot change can move one player between groups, and the callback can
        // arrive before the base group event. Signal a full re-query so channel
        // views never depend on the slot registry as their membership source.
        m_OnPlayerRegistryUpdated.Invoke(-1);

        foreach (Faction faction : m_Roster.GetFactions())
        {
            if (faction)
                m_OnFactionUpdated.Invoke(faction);
        }
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
		GC_ELobbyPhase phase = GetLobbyPhase();
		writer.WriteIntRange(phase, GC_ELobbyPhase.PREGAME, GC_ELobbyPhase.POSTGAME);
		PrintFormat("GC | Phase JIP | saving snapshot: lobby=%1 ordinal=%2", GetPhaseName(phase), phase);

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

		if (!m_VoiceState.Save(writer) || !m_BriefingCatalog.Save(writer))
			return false;

		PrintFormat("GC | JIP save complete: groups=%1 slots=%2", groups.Count(), slots.Count());
        return true;
    }

    //! Deserializes Lobby2's snapshot from the authority.
    override protected bool RplLoad(ScriptBitReader reader)
    {
		int phaseOrdinal;
		if (!reader.ReadIntRange(phaseOrdinal, GC_ELobbyPhase.PREGAME, GC_ELobbyPhase.POSTGAME))
			return false;

		GC_ELobbyPhase phase = phaseOrdinal;
		PrintFormat("GC | Phase JIP | loading snapshot: lobby=%1 ordinal=%2", GetPhaseName(phase), phaseOrdinal);
		ApplyLobbyPhase(phase);

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

			if (!ResolveJipGroup(groupRplId))
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
				return false;

			m_Roster.AddSlot(slot);
			SubscribeToSlotEvents(slot);
		}

		if (!m_VoiceState.Load(reader) || !m_BriefingCatalog.Load(reader))
			return false;

		m_OnBriefingsChanged.Invoke(m_BriefingCatalog.GetBriefingRevision());
		m_OnBriefingMarkersChanged.Invoke(m_BriefingCatalog.GetMarkerRevision());
		ResolvePendingJipReferences();
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

		for (int groupIndex = pendingGroups.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			if (!ResolveJipGroup(pendingGroups[groupIndex]))
				continue;

			pendingGroups.RemoveOrdered(groupIndex);
		}

		int pendingSlotGroups;
		foreach (GC_Slot slot : m_Roster.GetSlots())
		{
			if (!slot || slot.GetGroup())
				continue;

			RplId slotGroupRplId = slot.GetGroupRplId();
			if (slotGroupRplId == RplId.Invalid())
			{
				PrintFormat("GC | JIP slot has invalid group RplId: slot=%1", slot.GetID(), level: LogLevel.WARNING);
				continue;
			}

			if (!slot.ResolveReplicatedGroup())
				pendingSlotGroups++;
		}

		if (!pendingGroups.IsEmpty() || pendingSlotGroups > 0)
		{
			GetGame().GetCallqueue().CallLater(ResolvePendingJipReferences, JIP_REFERENCE_RETRY_MS);
			return;
		}

		PrintFormat("GC | JIP topology ready: attempts=%1 groups=%2 factions=%3 slots=%4", attempt,
			m_Roster.GetGroups().Count(), m_Roster.GetFactions().Count(), m_Roster.GetSlots().Count());
		NotifyAllRegisteredFactions();
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

		m_OnFactionUpdated.Invoke(faction);
		return true;
    }

    bool UnregisterFaction(FactionKey factionKey)
    {
		Faction faction = m_Roster.GetFaction(factionKey);
		if (!m_Roster.RemoveFaction(faction))
			return false;

		m_OnFactionUpdated.Invoke(faction);
		return true;
    }

    Faction GetRegisteredFaction(FactionKey factionKey) { return m_Roster.GetFaction(factionKey); }

    void NotifyFactionUpdated(Faction faction)
    {
		if (m_Roster.ContainsFaction(faction))
			m_OnFactionUpdated.Invoke(faction);
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
    SCR_AIGroup GetRegisteredGroupByRplId(RplId groupRplId) { return m_Roster.GetGroupByRplId(groupRplId); }
    SCR_AIGroup GetPlayerGroup(int playerId) { return m_Roster.GetPlayerGroup(playerId); }
    Faction GetPlayerFaction(int playerId) { return m_Roster.GetPlayerFaction(playerId); }

	GC_VoiceChannelAssignment GetPlayerVoiceAssignment(int playerId) { return m_VoiceState.Get(playerId); }
	bool IsPlayerVoiceDeafened(int playerId) { return m_VoiceState.IsDeafened(playerId); }

	bool TrySetPlayerVoiceDeafened(int playerId, bool deafened)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment assignment;
		if (!m_VoiceState.SetDeafened(playerId, deafened, assignment))
			return assignment != null;

		PrintFormat("GC | Voice deafen state changed: player=%1 deafened=%2", playerId, deafened);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnPlayerRegistryUpdated.Invoke(playerId);
		return true;
	}

	bool TrySetPlayerVoiceChannel(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid())
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		switch (channelType)
		{
			case GC_VoiceChannelType.GLOBAL:
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
		}

		return false;
	}

	protected bool SetPlayerVoiceChannel(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid())
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment assignment;
		if (!m_VoiceState.SetChannel(playerId, channelType, factionKey, groupRplId, assignment))
			return assignment != null;

		PrintFormat("GC | Voice assignment changed: player=%1 type=%2 faction=%3 group=%4", playerId, channelType, factionKey, groupRplId);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnPlayerRegistryUpdated.Invoke(playerId);
		return true;
	}

	protected void RemovePlayerVoiceAssignment(int playerId)
	{
		if (!Replication.IsServer() || !m_VoiceState.Remove(playerId))
			return;

		Rpc(RpcDo_RemoveVoiceAssignment, playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateVoiceAssignment(GC_VoiceChannelAssignment updateAssignment)
	{
		if (Replication.IsServer() || !m_VoiceState.Apply(updateAssignment))
			return;

		m_OnPlayerRegistryUpdated.Invoke(updateAssignment.GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RemoveVoiceAssignment(int playerId)
	{
		if (Replication.IsServer())
			return;

		m_VoiceState.Remove(playerId);
		m_OnPlayerRegistryUpdated.Invoke(playerId);
	}

    //! Returns one briefing by its one-based authored order/index.
    GC_Briefing GetBriefing(int briefingOrder)
    {
        return m_BriefingCatalog.GetBriefing(briefingOrder);
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

    bool SetBriefing(int order, string title, string content, array<string> factionKeys = null, GC_ELobbyPhase showAtGameModeState = GC_ELobbyPhase.GAME, bool invertGameModeState = false, bool visible = true)
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
            || !GC_LobbyPhaseMachine.IsValid(briefing.GetShowAtGameModeState()))
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

    bool SetBriefingMarker(string markerName, ResourceName image, string quadName, vector worldPosition,
        float rotation, bool worldScale, float size, string description, array<string> factionKeys = null,
        GC_ELobbyPhase showAtGameModeState = GC_ELobbyPhase.GAME,
        bool invertGameModeState = false, bool visible = true, int color = Color.WHITE)
    {
        GC_BriefingMarker update = new GC_BriefingMarker();
        update.Set(markerName, image, quadName, worldPosition, rotation, worldScale, size,
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

        return SetBriefingMarker(marker.GetName(), marker.GetImage(), marker.GetQuadName(),
            marker.GetWorldPosition(), marker.GetRotation(), marker.IsWorldScale(), marker.GetSize(),
            marker.GetDescription(), marker.GetFactionKeys(), marker.GetShowAtGameModeState(),
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
            || !GC_LobbyPhaseMachine.IsValid(marker.GetShowAtGameModeState()))
            return false;

        if (marker.GetImage().EndsWith(".imageset") && marker.GetQuadName().IsEmpty())
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
        m_OnPlayerRegistryUpdated.Invoke(playerId);
        QueueAllRegisteredFactionUpdates();
        return true;
    }

    //! Validates and applies a player-owned slot request on the server authority.
    bool TryAssignPlayerToSlot(int playerId, int slotId)
    {
        if (!Replication.IsServer() || playerId <= 0)
            return false;

        GC_Slot targetSlot = GetSlot(slotId);
        if (!targetSlot || targetSlot.GetState() != GC_SlotState.FREE)
            return false;

        int occupyingPlayerId = targetSlot.GetPlayerId();
        if (occupyingPlayerId >= 0 && occupyingPlayerId != playerId)
            return false;

        SCR_AIGroup targetGroup = targetSlot.GetGroup();
        if (!targetGroup || !m_Roster.GetGroups().Contains(targetGroup))
            return false;

        SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
        if (!groupsManager)
            return false;

        GC_Slot currentSlot = GetSlotByPlayerId(playerId);
        SCR_AIGroup currentGroup = GetPlayerGroup(playerId);

        // Clicking the player's current slot toggles them back out of it.
        if (occupyingPlayerId == playerId)
        {
            if (currentGroup)
                currentGroup.RemovePlayer(playerId);

            targetSlot.SetPlayer(-1);
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
            currentSlot.SetPlayer(-1);

        targetSlot.SetPlayer(playerId);
        SetPlayerVoiceChannel(playerId, GC_VoiceChannelType.GROUP, targetGroup.GetFaction().GetFactionKey(), Replication.FindItemId(targetGroup));
        PrintFormat("GC | Slot request accepted: player=%1 slot=%2", playerId, slotId);
        return true;
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

    GC_PlayerFormationDefinition GetFormation(string name)
    {
        foreach (ref GC_PlayerFormationDefinition formation : m_aFormations)
        {
            if (formation.m_sName == name)
                return formation;
        }
        
        return null;
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
        GetGame().GetCallqueue().Remove(NotifyAllRegisteredFactions);
		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
        UnsubscribeFromAllGroupEvents();
        UnsubscribeFromAllSlotEvents();
		m_Roster.Clear();
		m_VoiceState.Clear();
		m_BriefingCatalog.Clear();
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
