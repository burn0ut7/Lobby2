#define GC_LOBBY

class GC_TestCompClass : ScriptComponentClass
{
}

class GC_TestComp : ScriptComponent
{
    override protected void OnPostInit(IEntity owner)
    { 	
		if (SCR_Global.IsEditMode())
			return;

		GetGame().GetCallqueue().CallLater(CheckSlots, 1);
    }
	
	void CheckSlots()
	{
		if (SCR_Global.IsEditMode())
			return;

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
    }
}


modded enum SCR_EGameModeState
{
	BRIEFING
}

class GC_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_LobbyManager : SCR_BaseGameModeComponent
{
    protected const ResourceName SPECTATOR_PREFAB = "{8A578DE95771CC1D}Prefabs/Characters/Core/Character_Spectator.et";
    protected const int JIP_REFERENCE_RETRY_MS = 1000;
    protected const int MAX_BRIEFINGS = 32;
    protected const int MAX_BRIEFING_TITLE_LENGTH = 128;
    protected const int MAX_BRIEFING_CONTENT_LENGTH = 16384;

    //! Game-mode setting: group headers in the voice list may be selectable
    //! without changing the player's lobby slot or base-game group.
    [Attribute("1", UIWidgets.CheckBox, "Allow group voice-channel selection")]
    protected bool m_bVoiceGroupHeaderSwitchesGroup;
    
    [Attribute("", UIWidgets.Object, "Formations for player groups")]
    protected ref array<ref GC_PlayerFormationDefinition> m_aFormations;

    [Attribute("", UIWidgets.Object, "Server-authoritative briefing definitions")]
    protected ref array<ref GC_Briefing> m_aBriefings;

    protected ref array<SCR_AIGroup> m_aGroups = {};
    protected ref array<RplId> m_aPendingJipGroupRplIds = {};
    protected ref array<Faction> m_aRegisteredFactions = {};
    protected ref array<ref GC_Slot> m_aSlots = {};
    protected ref array<ref GC_VoiceChannelAssignment> m_aVoiceAssignments = {};
    ref ScriptInvoker<Faction> m_OnFactionUpdated = new ScriptInvoker<Faction>();
    //! Signals that connected-player or authoritative group membership data changed.
    //! Consumers re-query PlayerManager and SCR_GroupsManagerComponent instead of
    //! maintaining a second player roster in Lobby2.
    ref ScriptInvoker<int> m_OnPlayerRegistryUpdated = new ScriptInvoker<int>();
    //! Signals that the complete local briefing view changed. Consumers re-query
    //! this manager and current authoritative faction state.
    ref ScriptInvoker<int> m_OnBriefingsChanged = new ScriptInvoker<int>();
    protected int m_iNextSlotID = 1;
    protected int m_iJipResolveAttempt;
    protected int m_iBriefingRevision;

	//! Re-evaluate briefing presentation on every machine when the authoritative
	//! base-game phase changes. The phase itself remains owned and replicated by
	//! SCR_BaseGameMode.
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		m_OnBriefingsChanged.Invoke(m_iBriefingRevision);
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

            SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
            pc.SetSpectator(spectator);
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
        
        if (m_aGroups.Contains(group))
            return;

        bool wasEmpty = m_aGroups.IsEmpty();
        m_aGroups.Insert(group);
        SubscribeToGroupEvents(group);

        if (wasEmpty)
            SubscribeToGroupPlayerEvents();

        Faction faction = group.GetFaction();
        bool factionWasRegistered = faction && m_aRegisteredFactions.Contains(faction);
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

        int groupIndex = m_aGroups.Find(group);
        if (groupIndex < 0)
            return false;

        Faction faction = group.GetFaction();
        UnsubscribeFromGroupEvents(group);
        m_aGroups.RemoveOrdered(groupIndex);

        for (int i = m_aSlots.Count() - 1; i >= 0; i--)
        {
            GC_Slot slot = m_aSlots[i];
            if (slot && slot.GetGroup() == group)
            {
                UnsubscribeFromSlotEvents(slot);
                m_aSlots.RemoveOrdered(i);
            }
        }

        if (m_aGroups.IsEmpty())
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
        foreach (SCR_AIGroup group : m_aGroups)
            UnsubscribeFromGroupEvents(group);

        UnsubscribeFromGroupPlayerEvents();
    }

    protected void OnGroupFactionChanged(Faction faction)
    {
        if (faction && !m_aRegisteredFactions.Contains(faction))
            m_aRegisteredFactions.Insert(faction);

        // The event does not identify which group changed. Notify all registered
        // faction views so each view re-queries the authoritative group state.
        foreach (Faction registeredFaction : m_aRegisteredFactions)
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
        if (!group || !m_aGroups.Contains(group))
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
        foreach (GC_Slot slot : m_aSlots)
            UnsubscribeFromSlotEvents(slot);
    }

    protected void OnSlotChanged(GC_Slot slot)
    {
        if (!slot || !m_aSlots.Contains(slot))
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

        foreach (Faction faction : m_aRegisteredFactions)
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
            slot.SetID(m_iNextSlotID++);
            slot.SetAuthoredPrefab(prefab);
            slot.SetResolvedPrefab(resolvedPrefab);
            slot.SetVariantPolicy(variantPolicies[unitIndex]);
            slot.SetGroup(group);
            
            m_aSlots.Insert(slot);
            SubscribeToSlotEvents(slot);

            UpdateSlot(slot);
        }

        NotifyGroupFactionUpdated(group);
    }

    //! Serializes the registered groups and slots for initial replication and JIP.
    override protected bool RplSave(ScriptBitWriter writer)
    {
        int groupCount = m_aGroups.Count();
        writer.WriteInt(groupCount);
		PrintFormat("GC | JIP save begin: groups=%1 slots=%2", groupCount, m_aSlots.Count());

        for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
        {
            SCR_AIGroup group = m_aGroups[groupIndex];
            RplId groupRplId = RplId.Invalid();
            if (group)
                groupRplId = Replication.FindItemId(group);

            writer.WriteRplId(groupRplId);
        }

        int slotCount = m_aSlots.Count();
        writer.WriteInt(slotCount);

        foreach (GC_Slot slot : m_aSlots)
        {
            if (!slot || !slot.RplSave(writer))
                return false;
        }

		PrintFormat("GC | JIP save complete: groups=%1 slots=%2", groupCount, slotCount);

		writer.WriteInt(m_aVoiceAssignments.Count());
		foreach (GC_VoiceChannelAssignment assignment : m_aVoiceAssignments)
		{
			if (!assignment || !assignment.RplSave(writer))
				return false;
		}

		writer.WriteInt(m_iBriefingRevision);
		writer.WriteInt(m_aBriefings.Count());
		foreach (GC_Briefing briefing : m_aBriefings)
		{
			if (!briefing || !briefing.RplSave(writer))
				return false;
		}

        return true;
    }

    //! Deserializes the registered groups and slots from the authority.
    override protected bool RplLoad(ScriptBitReader reader)
    {
        GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		m_iJipResolveAttempt = 0;

        int groupCount;
        if (!reader.ReadInt(groupCount) || groupCount < 0)
            return false;
		PrintFormat("GC | JIP load begin: serializedGroups=%1", groupCount);

        UnsubscribeFromAllGroupEvents();
        UnsubscribeFromAllSlotEvents();
        m_aGroups.Clear();
        m_aPendingJipGroupRplIds.Clear();
        m_aRegisteredFactions.Clear();
        for (int i = 0; i < groupCount; i++)
        {
            RplId groupRplId;
            if (!reader.ReadRplId(groupRplId))
                return false;

			if (groupRplId == RplId.Invalid())
            {
				PrintFormat("GC | JIP load group has invalid RplId: index=%1", i, level: LogLevel.WARNING);
				continue;
            }

			bool resolved = ResolveJipGroup(groupRplId);
			if (!resolved)
				m_aPendingJipGroupRplIds.Insert(groupRplId);

        }

        int slotCount;
        if (!reader.ReadInt(slotCount) || slotCount < 0)
            return false;
		PrintFormat("GC | JIP load slots: serializedSlots=%1", slotCount);

        m_aSlots.Clear();
        for (int i = 0; i < slotCount; i++)
        {
            GC_Slot slot = new GC_Slot();
            if (!slot.RplLoad(reader))
                return false;

            m_aSlots.Insert(slot);
            SubscribeToSlotEvents(slot);
        }

		int assignmentCount;
		if (!reader.ReadInt(assignmentCount) || assignmentCount < 0)
			return false;

		m_aVoiceAssignments.Clear();
		for (int assignmentIndex = 0; assignmentIndex < assignmentCount; assignmentIndex++)
		{
			GC_VoiceChannelAssignment assignment = new GC_VoiceChannelAssignment();
			if (!assignment.RplLoad(reader))
				return false;

			m_aVoiceAssignments.Insert(assignment);
		}

		if (!reader.ReadInt(m_iBriefingRevision))
			return false;

		int briefingCount;
		if (!reader.ReadInt(briefingCount) || briefingCount < 0 || briefingCount > MAX_BRIEFINGS)
			return false;

		m_aBriefings.Clear();
		for (int briefingIndex = 0; briefingIndex < briefingCount; briefingIndex++)
		{
			GC_Briefing briefing = new GC_Briefing();
			if (!briefing.RplLoad(reader))
				return false;

			m_aBriefings.Insert(briefing);
		}

		m_OnBriefingsChanged.Invoke(m_iBriefingRevision);

		ResolvePendingJipReferences();

        return true;
    }

	//! Resolves one serialized group ID after its proxy becomes available on a JIP client.
	protected bool ResolveJipGroup(RplId groupRplId)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(Replication.FindItem(groupRplId));
		if (!group)
			return false;

		if (!m_aGroups.Contains(group))
		{
			bool wasEmpty = m_aGroups.IsEmpty();
			m_aGroups.Insert(group);
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

	//! Retries cross-entity references because JIP RplLoad can run before group proxies stream in.
	protected void ResolvePendingJipReferences()
	{
		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
		m_iJipResolveAttempt++;

		for (int groupIndex = m_aPendingJipGroupRplIds.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			RplId groupRplId = m_aPendingJipGroupRplIds[groupIndex];
			if (!ResolveJipGroup(groupRplId))
				continue;

			m_aPendingJipGroupRplIds.RemoveOrdered(groupIndex);
		}

		int pendingSlotGroups;
		foreach (GC_Slot slot : m_aSlots)
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
			{
				pendingSlotGroups++;
				continue;
			}

		}

		if (!m_aPendingJipGroupRplIds.IsEmpty() || pendingSlotGroups > 0)
		{
			GetGame().GetCallqueue().CallLater(ResolvePendingJipReferences, JIP_REFERENCE_RETRY_MS);
			return;
		}

		PrintFormat("GC | JIP topology ready: attempts=%1 groups=%2 factions=%3 slots=%4", m_iJipResolveAttempt, m_aGroups.Count(), m_aRegisteredFactions.Count(), m_aSlots.Count());
		NotifyAllRegisteredFactions();
	}

    //! Registers a faction key with the lobby manager.
    bool RegisterFaction(FactionKey factionKey)
    {
        if (factionKey == "")
            return false;

        Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
        if (!faction  || m_aRegisteredFactions.Contains(faction))
            return false;

        m_aRegisteredFactions.Insert(faction);
        m_OnFactionUpdated.Invoke(faction);
        return true;
    }

    //! Removes a faction key from the lobby manager.
    bool UnregisterFaction(FactionKey factionKey)
    {
        if (factionKey == "")
            return false;

        Faction faction;
        for (int i = 0; i < m_aRegisteredFactions.Count(); i++)
        {
            faction = m_aRegisteredFactions[i];
            if (!faction || faction.GetFactionKey() != factionKey)
                continue;

            m_aRegisteredFactions.RemoveOrdered(i);
            m_OnFactionUpdated.Invoke(faction);
            return true;
        }

        return false;
    }

    //! Returns whether a faction key is currently registered.
    Faction GetRegisteredFaction(FactionKey factionKey)
    {
        Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
        if (!faction)
            return null;

        if (!m_aRegisteredFactions.Contains(faction))
            return null;

        return faction;
    }

    //! Notifies subscribed UI when an already registered faction changes.
    void NotifyFactionUpdated(Faction faction)
    {
        if (!faction || !m_aRegisteredFactions.Contains(faction))
            return;

        m_OnFactionUpdated.Invoke(faction);
    }

    //! Returns the faction keys currently registered with the lobby manager.
    array<Faction> GetRegisteredFactions()
    {
        return m_aRegisteredFactions;
    }

    //! Returns the registered base-game groups used by the lobby presentation.
    array<SCR_AIGroup> GetRegisteredGroups()
    {
        return m_aGroups;
    }

    array<ref GC_Slot> GetSlots()
    {
        return m_aSlots;
    }

    //! Returns registered groups whose current base-game faction matches the key.
    array<SCR_AIGroup> GetGroupsForFaction(FactionKey factionKey)
    {
        array<SCR_AIGroup> groups = {};
        if (factionKey == "")
            return groups;

        foreach (SCR_AIGroup group : m_aGroups)
        {
            if (!group)
                continue;

            Faction faction = group.GetFaction();
            if (faction && faction.GetFactionKey() == factionKey)
                groups.Insert(group);
        }

        return groups;
    }

    //! Counts unique players occupying lobby slots whose authoritative groups belong to the faction.
    int GetFactionPlayerCount(FactionKey factionKey)
    {
        array<int> playerIDs = {};

        foreach (SCR_AIGroup group : GetGroupsForFaction(factionKey))
        {
            foreach (GC_Slot slot : GetSlotsForGroup(group))
            {
                if (!slot)
                    continue;

                int playerID = slot.GetPlayerId();
                if (playerID > 0 && !playerIDs.Contains(playerID))
                    playerIDs.Insert(playerID);
            }
        }

        return playerIDs.Count();
    }

    //! Counts unique players currently occupying lobby slots in the specified base-game group.
    int GetGroupPlayerCount(SCR_AIGroup group)
    {
        array<int> playerIDs = {};

        foreach (GC_Slot slot : GetSlotsForGroup(group))
        {
            if (!slot)
                continue;

            int playerID = slot.GetPlayerId();
            if (playerID > 0 && !playerIDs.Contains(playerID))
                playerIDs.Insert(playerID);
        }

        return playerIDs.Count();
    }

    //! Returns lobby slot records belonging to the specified base-game group.
    array<ref GC_Slot> GetSlotsForGroup(SCR_AIGroup group)
    {
        array<ref GC_Slot> slots = {};
        if (!group)
            return slots;

        foreach (GC_Slot slot : m_aSlots)
        {
            if (slot && slot.GetGroup() == group)
                slots.Insert(slot);
        }

        return slots;
    }

    //! Counts lobby slots belonging to groups whose authoritative faction matches the key.
    int GetFactionSlotCount(FactionKey factionKey)
    {
        int slotCount;

        foreach (SCR_AIGroup group : GetGroupsForFaction(factionKey))
        {
            foreach (GC_Slot slot : GetSlotsForGroup(group))
            {
                if (slot)
                    slotCount++;
            }
        }

        return slotCount;
    }

    GC_Slot GetSlot(int id)
    {
        foreach (ref GC_Slot slot : m_aSlots)
        {
            if (slot && slot.GetID() == id)
                return slot;
        }

        return null;
    }

    GC_Slot GetSlotByPlayerId(int playerId)
    {
        if (playerId < 0)
            return null;

        foreach (ref GC_Slot slot : m_aSlots)
        {
            if (slot && slot.GetPlayerId() == playerId)
                return slot;
        }

        return null;
    }

	GC_VoiceChannelAssignment GetPlayerVoiceAssignment(int playerId)
	{
		foreach (GC_VoiceChannelAssignment assignment : m_aVoiceAssignments)
		{
			if (assignment && assignment.GetPlayerId() == playerId)
				return assignment;
		}

		return null;
	}

	bool IsPlayerVoiceDeafened(int playerId)
	{
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		return assignment && assignment.IsDeafened();
	}

	//! Applies presentation-only deafen state on the server. Actual VoN audio
	//! routing is intentionally outside this lobby UI slice.
	bool TrySetPlayerVoiceDeafened(int playerId, bool deafened)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (!assignment)
		{
			assignment = new GC_VoiceChannelAssignment();
			assignment.Set(playerId, GC_VoiceChannelType.GLOBAL);
			m_aVoiceAssignments.Insert(assignment);
		}

		if (assignment.IsDeafened() == deafened)
			return true;

		assignment.SetDeafened(deafened);
		PrintFormat("GC | Voice deafen state changed: player=%1 deafened=%2", playerId, deafened);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnPlayerRegistryUpdated.Invoke(playerId);
		return true;
	}

	//! Validates a requested exclusive channel and applies it on the authority.
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

		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (!assignment)
		{
			assignment = new GC_VoiceChannelAssignment();
			m_aVoiceAssignments.Insert(assignment);
		}
		else if (assignment.GetChannelType() == channelType && assignment.GetFactionKey() == factionKey && assignment.GetGroupRplId() == groupRplId)
		{
			return true;
		}

		bool deafened = assignment.IsDeafened();
		assignment.Set(playerId, channelType, factionKey, groupRplId);
		assignment.SetDeafened(deafened);
		PrintFormat("GC | Voice assignment changed: player=%1 type=%2 faction=%3 group=%4", playerId, channelType, factionKey, groupRplId);
		Rpc(RpcDo_UpdateVoiceAssignment, assignment);
		m_OnPlayerRegistryUpdated.Invoke(playerId);
		return true;
	}

	protected void RemovePlayerVoiceAssignment(int playerId)
	{
		if (!Replication.IsServer())
			return;

		for (int i = m_aVoiceAssignments.Count() - 1; i >= 0; i--)
		{
			GC_VoiceChannelAssignment assignment = m_aVoiceAssignments[i];
			if (!assignment || assignment.GetPlayerId() != playerId)
				continue;

			m_aVoiceAssignments.RemoveOrdered(i);
			Rpc(RpcDo_RemoveVoiceAssignment, playerId);
			return;
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateVoiceAssignment(GC_VoiceChannelAssignment updateAssignment)
	{
		if (Replication.IsServer() || !updateAssignment)
			return;

		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(updateAssignment.GetPlayerId());
		if (!assignment)
		{
			assignment = new GC_VoiceChannelAssignment();
			m_aVoiceAssignments.Insert(assignment);
		}

		assignment.CopyFrom(updateAssignment);
		m_OnPlayerRegistryUpdated.Invoke(assignment.GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RemoveVoiceAssignment(int playerId)
	{
		if (Replication.IsServer())
			return;

		for (int i = m_aVoiceAssignments.Count() - 1; i >= 0; i--)
		{
			GC_VoiceChannelAssignment assignment = m_aVoiceAssignments[i];
			if (assignment && assignment.GetPlayerId() == playerId)
				m_aVoiceAssignments.RemoveOrdered(i);
		}

		m_OnPlayerRegistryUpdated.Invoke(playerId);
	}

    //! Resolves a registered group from the replicated identity supplied by a
    //! client. The client never sends an object reference as authority.
    SCR_AIGroup GetRegisteredGroupByRplId(RplId groupRplId)
    {
        if (groupRplId == RplId.Invalid())
            return null;

        foreach (SCR_AIGroup group : m_aGroups)
        {
            if (group && Replication.FindItemId(group) == groupRplId)
                return group;
        }

        return null;
    }

    //! Returns the player's current authoritative base-game group. The groups
    //! manager is preferred, while registered groups provide a race-safe query
    //! of the same base-game membership when its index has not settled yet.
    SCR_AIGroup GetPlayerGroup(int playerId)
    {
        foreach (SCR_AIGroup registeredGroup : m_aGroups)
        {
            if (registeredGroup && registeredGroup.GetPlayerIDs().Contains(playerId))
                return registeredGroup;
        }

        SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
        if (groupsManager)
            return groupsManager.GetPlayerGroup(playerId);

        return null;
    }

    //! Returns the player's current authoritative faction, with the lobby slot
    //! as a short-lived fallback while base-group membership is settling.
    Faction GetPlayerFaction(int playerId)
    {
        SCR_AIGroup playerGroup = GetPlayerGroup(playerId);
        if (playerGroup && playerGroup.GetFaction())
            return playerGroup.GetFaction();

        GC_Slot slot = GetSlotByPlayerId(playerId);
        if (slot && slot.GetGroup())
            return slot.GetGroup().GetFaction();

        return null;
    }

    //! Returns one briefing by its one-based authored order/index.
    GC_Briefing GetBriefing(int briefingOrder)
    {
        if (briefingOrder <= 0)
            return null;

        foreach (GC_Briefing briefing : m_aBriefings)
        {
            if (briefing && briefing.GetOrder() == briefingOrder)
                return briefing;
        }

        return null;
    }

    //! Returns the complete local replicated briefing snapshot.
    array<ref GC_Briefing> GetBriefings()
    {
        return m_aBriefings;
    }

    //! Derives a player's visible briefing list from current base-game faction state.
    array<ref GC_Briefing> GetBriefingsForPlayer(int playerId)
    {
        array<ref GC_Briefing> result = {};
        Faction faction = GetPlayerFaction(playerId);
		SCR_EGameModeState gameModeState = SCR_EGameModeState.GAME;
		SCR_BaseGameMode gameMode = GetGameMode();
		if (gameMode)
			gameModeState = gameMode.GetState();

        foreach (GC_Briefing briefing : m_aBriefings)
        {
            if (briefing && briefing.IsVisibleToFaction(faction, gameModeState))
                result.Insert(briefing);
        }

        return result;
    }

    //! Server-only upsert used by mission systems when briefing data changes.
    bool UpsertBriefing(GC_Briefing update)
    {
        if (!Replication.IsServer() || !ValidateBriefing(update))
            return false;

        GC_Briefing briefing = GetBriefing(update.GetOrder());
        if (!briefing)
        {
            if (m_aBriefings.Count() >= MAX_BRIEFINGS)
                return false;

            briefing = new GC_Briefing();
            m_aBriefings.Insert(briefing);
        }

        briefing.CopyFrom(update);
        briefing.SetRevision(++m_iBriefingRevision);
        m_OnBriefingsChanged.Invoke(m_iBriefingRevision);
        Rpc(RpcDo_UpsertBriefing, briefing);
        return true;
    }

    //! Convenience server API for creating a complete replacement payload.
    bool SetBriefing(int order, string title, string content, array<string> factionKeys = null, SCR_EGameModeState showAtGameModeState = SCR_EGameModeState.GAME, bool invertGameModeState = false, bool visible = true)
    {
        GC_Briefing update = new GC_Briefing();
        update.Set(order, title, content, factionKeys, showAtGameModeState, invertGameModeState, visible);
        return UpsertBriefing(update);
    }

    //! Server-only visibility mutation that preserves the rest of the briefing payload.
    bool SetBriefingVisible(int briefingOrder, bool visible)
    {
        if (!Replication.IsServer())
            return false;

        GC_Briefing briefing = GetBriefing(briefingOrder);
        if (!briefing || briefing.IsVisible() == visible)
            return briefing != null;

        GC_Briefing update = new GC_Briefing();
        update.Set(
            briefing.GetOrder(),
            briefing.GetTitle(),
            briefing.GetContent(),
			briefing.GetFactionKeys(),
			briefing.GetShowAtGameModeState(),
			briefing.IsGameModeStateInverted(),
            visible);
        return UpsertBriefing(update);
    }

    //! Server-only removal. Reliable RPC ordering keeps proxies on the same revision.
    bool RemoveBriefing(int briefingOrder)
    {
        if (!Replication.IsServer() || briefingOrder <= 0)
            return false;

        for (int index = 0; index < m_aBriefings.Count(); index++)
        {
            GC_Briefing briefing = m_aBriefings[index];
            if (!briefing || briefing.GetOrder() != briefingOrder)
                continue;

            m_aBriefings.RemoveOrdered(index);
            m_iBriefingRevision++;
            m_OnBriefingsChanged.Invoke(m_iBriefingRevision);
            Rpc(RpcDo_RemoveBriefing, briefingOrder, m_iBriefingRevision);
            return true;
        }

        return false;
    }

    protected bool ValidateBriefing(GC_Briefing briefing)
    {
        if (!briefing || briefing.GetOrder() <= 0 || briefing.GetOrder() > MAX_BRIEFINGS)
            return false;

        if (briefing.GetTitle().Length() > MAX_BRIEFING_TITLE_LENGTH
            || briefing.GetContent().Length() > MAX_BRIEFING_CONTENT_LENGTH)
            return false;

		if (briefing.GetShowAtGameModeState() < SCR_EGameModeState.PREGAME
			|| briefing.GetShowAtGameModeState() > SCR_EGameModeState.POSTGAME)
			return false;

        array<string> factionKeys = briefing.GetFactionKeys();
        if (!factionKeys || factionKeys.Count() > GC_Briefing.MAX_FACTION_KEYS)
            return false;

        FactionManager factionManager;
        if (factionKeys.Count() > 0)
        {
            factionManager = GetGame().GetFactionManager();
            if (!factionManager)
                return false;
        }

        array<string> validatedFactionKeys = {};
        foreach (string factionKey : factionKeys)
        {
            if (factionKey.IsEmpty()
                || validatedFactionKeys.Contains(factionKey)
                || !factionManager.GetFactionByKey(factionKey))
                return false;

            validatedFactionKeys.Insert(factionKey);
        }

        return true;
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_UpsertBriefing(GC_Briefing update)
    {
        if (Replication.IsServer() || !update)
            return;

        GC_Briefing briefing = GetBriefing(update.GetOrder());
        if (briefing && update.GetRevision() <= briefing.GetRevision())
            return;

        if (!briefing)
        {
            if (m_aBriefings.Count() >= MAX_BRIEFINGS)
                return;

            briefing = new GC_Briefing();
            m_aBriefings.Insert(briefing);
        }

        briefing.CopyFrom(update);
        if (update.GetRevision() > m_iBriefingRevision)
            m_iBriefingRevision = update.GetRevision();

        m_OnBriefingsChanged.Invoke(m_iBriefingRevision);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_RemoveBriefing(int briefingOrder, int revision)
    {
        if (Replication.IsServer() || revision <= m_iBriefingRevision)
            return;

        for (int index = m_aBriefings.Count() - 1; index >= 0; index--)
        {
            GC_Briefing briefing = m_aBriefings[index];
            if (briefing && briefing.GetOrder() == briefingOrder)
                m_aBriefings.RemoveOrdered(index);
        }

        m_iBriefingRevision = revision;
        m_OnBriefingsChanged.Invoke(m_iBriefingRevision);
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
        if (!targetGroup || !m_aGroups.Contains(targetGroup))
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
        if (!Replication.IsServer() || !slot || !m_aSlots.Contains(slot))
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
            m_aSlots.Insert(slot);
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
        GetGame().GetCallqueue().Remove(NotifyAllRegisteredFactions);
		GetGame().GetCallqueue().Remove(ResolvePendingJipReferences);
        UnsubscribeFromAllGroupEvents();
        UnsubscribeFromAllSlotEvents();
        m_aGroups.Clear();
		m_aPendingJipGroupRplIds.Clear();
        m_aSlots.Clear();
		m_aVoiceAssignments.Clear();
		m_aBriefings.Clear();

        super.OnGameModeEnd(data);
    }

    static GC_LobbyManager GetManager()
    {
        if (SCR_Global.IsEditMode())
            return null;

        if (!GetGame().GetGameMode())
            return null;

        return GC_LobbyManager.Cast(GetGame().GetGameMode().FindComponent(GC_LobbyManager));
    }
}
