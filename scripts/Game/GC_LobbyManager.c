#define GC_LOBBY

class GC_TestCompClass : ScriptComponentClass
{
}

class GC_TestComp : ScriptComponent
{
    override protected void OnPostInit(IEntity owner)
    {	
		GetGame().GetCallqueue().CallLater(CheckSlots, 1);
    }
	
	void CheckSlots()
	{
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
    
    [Attribute("", UIWidgets.Object, "Formations for player groups")]
    protected ref array<ref GC_PlayerFormationDefinition> m_aFormations;

    protected ref array<SCR_AIGroup> m_aGroups = {};
    protected ref array<Faction> m_aRegisteredFactions = {};
    protected ref array<ref GC_Slot> m_aSlots = {};
    protected int m_iNextSlotID = 1;

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
        if (!Replication.IsServer() || !group)
            return;
        
        if (m_aGroups.Contains(group))
            return;

        m_aGroups.Insert(group);
        RegisterGroupMembers(group);
        //On group register event?
    }

    protected void RegisterGroupMembers(SCR_AIGroup group)
    {
        if (!Replication.IsServer())
            return;

        if (!group || !group.m_aUnitPrefabSlots)
            return;
        
        foreach (ResourceName prefab : group.m_aUnitPrefabSlots)
        {
            GC_Slot slot = new GC_Slot();
            slot.SetID(m_iNextSlotID++);
            slot.SetPrefab(prefab);
            slot.SetGroup(group);
            
            m_aSlots.Insert(slot);

            UpdateSlot(slot);
            PrintFormat("GC | Registered slot: %1", m_iNextSlotID - 1);
        }
    }

    //! Serializes the registered groups and slots for initial replication and JIP.
    override protected bool RplSave(ScriptBitWriter writer)
    {
        int groupCount = m_aGroups.Count();
        writer.WriteInt(groupCount);

        foreach (SCR_AIGroup group : m_aGroups)
        {
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

        return true;
    }

    //! Deserializes the registered groups and slots from the authority.
    override protected bool RplLoad(ScriptBitReader reader)
    {
        int groupCount;
        if (!reader.ReadInt(groupCount) || groupCount < 0)
            return false;

        m_aGroups.Clear();
        for (int i = 0; i < groupCount; i++)
        {
            RplId groupRplId;
            if (!reader.ReadRplId(groupRplId))
                return false;

            SCR_AIGroup group = SCR_AIGroup.Cast(Replication.FindItem(groupRplId));
            if (group)
                m_aGroups.Insert(group);
        }

        int slotCount;
        if (!reader.ReadInt(slotCount) || slotCount < 0)
            return false;

        m_aSlots.Clear();
        for (int i = 0; i < slotCount; i++)
        {
            GC_Slot slot = new GC_Slot();
            if (!slot.RplLoad(reader))
                return false;

            m_aSlots.Insert(slot);
        }

        return true;
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
        return true;
    }

    //! Removes a faction key from the lobby manager.
    bool UnregisterFaction(FactionKey factionKey)
    {
        if (factionKey == "")
            return false;

        for (int i = 0; i < m_aRegisteredFactions.Count(); i++)
        {
            if (m_aRegisteredFactions[i].GetFactionKey() != factionKey)
                continue;

            m_aRegisteredFactions.RemoveOrdered(i);
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
            return faction;

        return null;
    }

    //! Returns the faction keys currently registered with the lobby manager.
    array<Faction> GetRegisteredFactions()
    {
        return m_aRegisteredFactions;
    }

    array<ref GC_Slot> GetSlots()
    {
        return m_aSlots;
    }

    GC_Slot GetSlot(int id)
    {
        foreach (ref GC_Slot slot : m_aSlots)
        {
            if (slot.GetID() == id)
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
            if (slot.GetPlayerId() == playerId)
                return slot;
        }

        return null;
    }

    //! Broadcasts a complete server-authoritative slot state.
    bool UpdateSlot(GC_Slot slot)
    {
        if (!Replication.IsServer() || !slot)
            return false;

        string playerUUID = slot.GetUUID();
        string prefab = slot.GetPrefab();
        RplId groupRplId = RplId.Invalid();
        if (slot.GetGroup())
            groupRplId = Replication.FindItemId(slot.GetGroup());

        Rpc(RpcDo_UpdateSlot, slot);
        return true;
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_UpdateSlot(GC_Slot updateSlot)
    {
        if (Replication.IsServer())
            return;

        int slotId = updateSlot.GetID();
        GC_Slot slot = GetSlot(slotId);
        if (!slot)
        {
            slot = new GC_Slot();
            slot.SetID(slotId);
            m_aSlots.Insert(slot);
        }
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

    static GC_LobbyManager GetManager()
    {
        return GC_LobbyManager.Cast(GetGame().GetGameMode().FindComponent(GC_LobbyManager));
    }
}
