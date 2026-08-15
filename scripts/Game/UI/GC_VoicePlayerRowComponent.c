//! Populates one reusable voice-channel player row from authoritative player,
//! group, and Lobby2 slot data.
class GC_VoicePlayerRowComponent : SCR_ScriptedWidgetComponent
{
    protected int m_iPlayerId = -1;
    protected GC_Slot m_Slot;
    protected SCR_AIGroup m_Group;
    protected ResourceName m_RoleInfoPrefab;
    protected ref SCR_UIInfo m_RoleInfo;

    void SetPlayer(int playerId, GC_Slot slot = null, SCR_AIGroup channelGroup = null)
    {
        m_iPlayerId = playerId;
        m_Slot = slot;
        m_Group = channelGroup;

        if (!m_Group && m_iPlayerId >= 0)
        {
            SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
            if (groupsManager)
                m_Group = groupsManager.GetPlayerGroup(m_iPlayerId);
        }

        Refresh();
    }

    protected void Refresh()
    {
        if (!m_wRoot || m_iPlayerId < 0)
            return;

        TextWidget playerName = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerName"));
        if (playerName)
        {
            string name = GetGame().GetPlayerManager().GetPlayerName(m_iPlayerId);
            if (name.IsEmpty() && m_Slot)
                name = m_Slot.GetCachedPlayerName();

            playerName.SetText(name);
        }

        TextWidget groupCallsign = TextWidget.Cast(m_wRoot.FindAnyWidget("GroupCallsign"));
        if (groupCallsign)
        {
            if (m_Group)
                groupCallsign.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));
            else
                groupCallsign.SetText(string.Empty);
        }

        UpdateFactionAccent();
        UpdateRoleVisuals();
    }

    protected void UpdateFactionAccent()
    {
        ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("FactionAccent"));
        if (!accent)
            return;

        Faction playerFaction = ResolvePlayerFaction();
        if (playerFaction)
        {
            accent.SetColor(playerFaction.GetFactionColor());
            accent.SetOpacity(1);
        }
        else
        {
            accent.SetOpacity(0);
        }
    }

    //! Resolve the player's current faction instead of using the channel's
    //! faction as presentation state. This keeps Global rows correct too.
    protected Faction ResolvePlayerFaction()
    {
        if (m_iPlayerId >= 0)
        {
            SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
            if (groupsManager)
            {
                SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(m_iPlayerId);
                if (playerGroup && playerGroup.GetFaction())
                    return playerGroup.GetFaction();
            }
        }

        if (m_Slot && m_Slot.GetGroup() && m_Slot.GetGroup().GetFaction())
            return m_Slot.GetGroup().GetFaction();

        if (m_Group && m_Group.GetFaction())
            return m_Group.GetFaction();

        return null;
    }

    protected void UpdateRoleVisuals()
    {
        ImageWidget roleIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleIcon"));
        if (!roleIcon)
            return;

        SCR_UIInfo roleInfo = GetRoleInfo();
        if (!roleInfo || !roleInfo.SetIconTo(roleIcon))
        {
            roleIcon.SetOpacity(0);
            return;
        }

        roleIcon.SetOpacity(1);
    }

    //! Role presentation is derived from the authored slot prefab, never from
    //! the live gameplay entity. Unslotted players simply have no role icon.
    protected SCR_UIInfo GetRoleInfo()
    {
        if (!m_Slot)
            return null;

        ResourceName prefab = m_Slot.GetAuthoredPrefab();
        if (prefab == m_RoleInfoPrefab)
            return m_RoleInfo;

        m_RoleInfoPrefab = prefab;
        m_RoleInfo = null;

        Resource prefabResource = Resource.Load(prefab);
        if (!prefabResource || !prefabResource.IsValid())
            return null;

        IEntityComponentSource editableCharacterSource = SCR_EditableEntityComponentClass.GetEditableEntitySource(prefabResource);
        if (!editableCharacterSource)
            return null;

        BaseContainer uiInfoContainer = editableCharacterSource.GetObject("m_UIInfo");
        if (uiInfoContainer)
            m_RoleInfo = SCR_UIInfo.Cast(BaseContainerTools.CreateInstanceFromContainer(uiInfoContainer));

        return m_RoleInfo;
    }
}
