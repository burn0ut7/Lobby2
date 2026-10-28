//! Populates one reusable player row from authoritative player and Lobby2 slot data.
class GC_VoicePlayerRowComponent : SCR_ScriptedWidgetComponent
{
    protected int m_iPlayerId = -1;
    protected bool m_bSpeaking;
	protected bool m_bShowLifeState;
    protected GC_LobbyPlayerController m_VoiceController;
	protected GC_LobbyManager m_SelectionManager;

    override void HandlerDeattached(Widget w)
    {
        if (m_VoiceController)
            m_VoiceController.m_OnVoiceActivity.Remove(OnVoiceActivity);
        m_VoiceController = null;
        GetGame().GetCallqueue().Remove(EndVoiceActivity);
        if (m_SelectionManager)
			m_SelectionManager.m_OnAdminSelectionChanged.Remove(Refresh);
		m_SelectionManager = null;
        super.HandlerDeattached(w);
    }

    protected void OnVoiceActivity(int playerId)
    {
        if (playerId != m_iPlayerId)
            return;
        m_bSpeaking = true;
        UpdateVoiceVisuals();
        GetGame().GetCallqueue().Remove(EndVoiceActivity);
        GetGame().GetCallqueue().CallLater(EndVoiceActivity, 500);
    }

    protected void EndVoiceActivity()
    {
        m_bSpeaking = false;
        UpdateVoiceVisuals();
    }
    protected GC_Slot m_Slot;
    protected SCR_AIGroup m_Group;
    protected ResourceName m_RoleInfoPrefab;
    protected ref SCR_UIInfo m_RoleInfo;

	//------------------------------------------------------------------------------------------------
	void SetShowLifeState(bool show)
	{
		m_bShowLifeState = show;
		Refresh();
	}

    void SetPlayer(int playerId, GC_Slot slot = null)
    {
        if (m_iPlayerId != playerId)
        {
            GetGame().GetCallqueue().Remove(EndVoiceActivity);
            m_bSpeaking = false;
        }
        if (m_VoiceController)
            m_VoiceController.m_OnVoiceActivity.Remove(OnVoiceActivity);
        m_VoiceController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
        if (m_VoiceController)
            m_VoiceController.m_OnVoiceActivity.Insert(OnVoiceActivity);
		if (m_SelectionManager)
			m_SelectionManager.m_OnAdminSelectionChanged.Remove(Refresh);
		m_SelectionManager = GC_LobbyManager.GetManager();
		if (m_SelectionManager)
			m_SelectionManager.m_OnAdminSelectionChanged.Insert(Refresh);
        m_iPlayerId = playerId;
        m_Slot = slot;
        m_Group = null;
        if (m_Slot)
            m_Group = m_Slot.GetGroup();

        Refresh();
    }

	override bool OnClick(Widget w, int x, int y, int button)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		PlayerController local = GetGame().GetPlayerController();
		if (button == 0 && m_bShowLifeState)
		{
			GC_LobbyPlayerController spectator = GC_LobbyPlayerController.Cast(local);
			if (spectator && spectator.GC_CanJumpSpectatorCamera())
			{
				spectator.GC_RequestSpectatorPlayerPosition(m_iPlayerId);
				return true;
			}
		}
		if ((button == 0 || button == 1) && manager && local && SCR_Global.IsAdmin(local.GetPlayerId()))
			manager.SelectAdminPlayer(m_iPlayerId);
		if (button == 0)
			return manager && local && SCR_Global.IsAdmin(local.GetPlayerId());
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

			if (m_SelectionManager && m_SelectionManager.GetAdminSelectedPlayer() == m_iPlayerId)
				name += "  [SELECTED]";
            playerName.SetText(name);
			GC_PlayerNamePresentation.Apply(playerName, m_iPlayerId);
        }

        TextWidget groupCallsign = TextWidget.Cast(m_wRoot.FindAnyWidget("GroupCallsign"));
        if (groupCallsign)
        {
            if (m_bShowLifeState && m_Slot && m_Slot.GetState() == GC_SlotState.KIA)
                groupCallsign.SetText("Dead");
            else if (m_Group)
                groupCallsign.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));
            else
                groupCallsign.SetText(string.Empty);
        }

        UpdateFactionAccent();
        UpdateRoleVisuals();
        UpdateVoiceVisuals();
        UpdateLocalPlayerBackground();
    }

	void RefreshAdminNamePresentation()
	{
		if (!m_wRoot)
			return;

		GC_PlayerNamePresentation.Apply(m_wRoot.FindAnyWidget("PlayerName"), m_iPlayerId);
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

		if (m_SelectionManager && m_SelectionManager.GetAdminSelectedPlayer() == m_iPlayerId)
			background.SetColor(Color.FromRGBA(146, 104, 35, 180));
        else if (isLocalPlayer)
            background.SetColor(Color.FromRGBA(194, 100, 20, 20));
        else
            background.SetColor(Color.FromRGBA(1, 1, 1, 158));
    }

	//------------------------------------------------------------------------------------------------
	//! Speech uses native local capture/receive activity; deafen remains replicated.
	protected void UpdateVoiceVisuals()
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		GC_VoiceChannelAssignment assignment;
		if (lobbyManager)
			assignment = lobbyManager.GetPlayerVoiceAssignment(m_iPlayerId);
		ImageWidget voiceIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("VoiceIcon"));
		Widget deafenedIcon = m_wRoot.FindAnyWidget("VoiceDeafenedIcon");
		if (!voiceIcon || !deafenedIcon)
			return;

		voiceIcon.SetColor(Color.White);
		voiceIcon.SetOpacity(1);
		voiceIcon.SetVisible(true);
		deafenedIcon.SetVisible(false);
		// Moderation takes precedence over priority and local activity.
		if (assignment && assignment.IsAdminMuted())
		{
			voiceIcon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", "lockedCircle");
			return;
		}
		if (assignment && assignment.IsPrioritySpeaker())
		{
			voiceIcon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", "microphone");
			if (!m_bSpeaking)
				voiceIcon.SetColor(Color.FromRGBA(170, 170, 170, 255));
			return;
		}

		bool deafened;
		if (lobbyManager)
			deafened = lobbyManager.IsPlayerVoiceDeafened(m_iPlayerId);

		voiceIcon.SetVisible(m_bSpeaking || !deafened);
		deafenedIcon.SetVisible(deafened && !m_bSpeaking);
		if (m_bSpeaking)
			voiceIcon.LoadImageFromSet(0, "{CB9C19A2219A4BFC}UI/Textures/Icons/Lobby_PS_Atlas_x64.imageset", "VoNDirect");
		else
		{
			voiceIcon.LoadImageFromSet(0, "{CB9C19A2219A4BFC}UI/Textures/Icons/Lobby_PS_Atlas_x64.imageset", "VoNIdle");
			voiceIcon.SetOpacity(0.38);
		}
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
