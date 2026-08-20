//! Populates one reusable player row from authoritative player and Lobby2 slot data.
class GC_VoicePlayerRowComponent : SCR_ScriptedWidgetComponent
{
    protected int m_iPlayerId = -1;
    protected GC_Slot m_Slot;
    protected SCR_AIGroup m_Group;
    protected ResourceName m_RoleInfoPrefab;
    protected ref SCR_UIInfo m_RoleInfo;

    void SetPlayer(int playerId, GC_Slot slot = null)
    {
        m_iPlayerId = playerId;
        m_Slot = slot;
        m_Group = null;
        if (m_Slot)
            m_Group = m_Slot.GetGroup();

        Refresh();
    }

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 1 || m_iPlayerId < 0)
			return false;

		IEntity controlledEntity;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
			controlledEntity = playerManager.GetPlayerControlledEntity(m_iPlayerId);

		GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForPlayer(m_iPlayerId, controlledEntity), m_wRoot);
		return true;
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
        UpdateVoiceVisuals();
        UpdateLocalPlayerBackground();
    }

    //! Recolors this row only for the player represented by the local client.
    //! The same row layout is used by both the player and voice-channel lists.
    protected void UpdateLocalPlayerBackground()
    {
        Widget background = m_wRoot.FindAnyWidget("Background");
        if (!background)
            return;

        bool isLocalPlayer;
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController)
            isLocalPlayer = playerController.GetPlayerId() == m_iPlayerId;

        if (isLocalPlayer)
            background.SetColor(Color.FromRGBA(194, 100, 20, 20));
        else
            background.SetColor(Color.FromRGBA(1, 1, 1, 158));
    }

    //! Shows the replicated lobby presentation state. The normal microphone
    //! remains the fallback until talking/muted VoN states are implemented.
    protected void UpdateVoiceVisuals()
    {
        Widget voiceIcon = m_wRoot.FindAnyWidget("VoiceIcon");
        Widget deafenedIcon = m_wRoot.FindAnyWidget("VoiceDeafenedIcon");
        if (!voiceIcon || !deafenedIcon)
            return;

        bool deafened;
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (lobbyManager)
            deafened = lobbyManager.IsPlayerVoiceDeafened(m_iPlayerId);

        voiceIcon.SetVisible(!deafened);
        deafenedIcon.SetVisible(deafened);
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

    //! Resolve the player's current faction instead of using the voice
    //! channel's faction as presentation state. This keeps Global rows
    //! correct too.
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
