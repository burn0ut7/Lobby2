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

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu);
		
		Print("GC | CheckSlots!!!");
		foreach (GC_Slot slot : GC_LobbyManager.GetManager().GetSlots())
        {
            PrintFormat("GC | Slot:%1", slot.GetID());
        }
	}
}

class GC_LobbyManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_LobbyManager : SCR_BaseGameModeComponent
{
    protected const ResourceName SPECTATOR_PREFAB = "{8A578DE95771CC1D}Prefabs/Characters/Core/Character_Spectator.et";
    protected const int JIP_REFERENCE_RETRY_MS = 1000;
    
    [Attribute("", UIWidgets.Object, "Formations for player groups")]
    protected ref array<ref GC_PlayerFormationDefinition> m_aFormations;

    protected ref array<SCR_AIGroup> m_aGroups = {};
    protected ref array<RplId> m_aPendingJipGroupRplIds = {};
    protected ref array<Faction> m_aRegisteredFactions = {};
    protected ref array<ref GC_Slot> m_aSlots = {};
    ref ScriptInvoker<Faction> m_OnFactionUpdated = new ScriptInvoker<Faction>();
    protected int m_iNextSlotID = 1;
    protected int m_iJipResolveAttempt;

    override void OnPlayerAuditSuccess(int playerId)
    {
        PrintFormat("GC | OnPlayerAuditSuccess: %1, connected!", playerId);
    
        //Check if player is already existing in world

        //If not setup player in spectator mode
        SetSpectator(playerId, true)
    }
    
    //! Make the player enter spectator mode from the server
    void SetSpectator(int playerId, bool state)
    {
        if (!Replication.IsServer())
            return;

        if (state)
        {
            EntitySpawnParams params = new EntitySpawnParams();
            params.Transform[3] = "125.581 1 127.501";
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
        if (faction)
            RegisterFaction(faction.GetFactionKey());

        if (Replication.IsServer())
            RegisterGroupMembers(group);
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
        QueueAllRegisteredFactionUpdates();
    }

    protected void OnGroupPlayerRemoved(SCR_AIGroup group, int playerId)
    {
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
            PrintFormat("GC | Registered slot: id=%1 unitIndex=%2 authored=%3 resolved=%4 variantPolicy=%5", m_iNextSlotID - 1, unitIndex, prefab, resolvedPrefab, variantPolicies[unitIndex]);
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
			PrintFormat("GC | JIP save group: index=%1 rplId=%2 group=%3", groupIndex, groupRplId, group);
        }

        int slotCount = m_aSlots.Count();
        writer.WriteInt(slotCount);

        foreach (GC_Slot slot : m_aSlots)
        {
            if (!slot || !slot.RplSave(writer))
                return false;
        }

		PrintFormat("GC | JIP save complete: groups=%1 slots=%2", groupCount, slotCount);
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

			PrintFormat("GC | JIP load group: index=%1 rplId=%2 resolved=%3", i, groupRplId, resolved);
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

		PrintFormat("GC | JIP group resolved: rplId=%1 group=%2 faction=%3", groupRplId, group, faction);
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

			PrintFormat("GC | JIP slot group resolved: slot=%1 groupRplId=%2 group=%3", slot.GetID(), slotGroupRplId, slot.GetGroup());
		}

		if (!m_aPendingJipGroupRplIds.IsEmpty() || pendingSlotGroups > 0)
		{
			PrintFormat("GC | JIP topology pending: attempt=%1 unresolvedGroups=%2 unresolvedSlotGroups=%3 resolvedGroups=%4 totalSlots=%5", m_iJipResolveAttempt, m_aPendingJipGroupRplIds.Count(), pendingSlotGroups, m_aGroups.Count(), m_aSlots.Count());
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
        SCR_AIGroup currentGroup = groupsManager.GetPlayerGroup(playerId);

        // Clicking the player's current slot toggles them back out of it.
        if (occupyingPlayerId == playerId)
        {
            if (currentGroup)
                currentGroup.RemovePlayer(playerId);

            targetSlot.SetPlayer(-1);
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
