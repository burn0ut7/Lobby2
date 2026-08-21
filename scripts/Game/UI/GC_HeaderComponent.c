class GC_HeaderComponent : SCR_ScriptedWidgetComponent
{
    ref ScriptInvoker<GC_HeaderComponent> m_OnStateChanged = new ScriptInvoker<GC_HeaderComponent>();

    protected const int VIEW_PREVIEW = 0;
    protected const int VIEW_LOBBY = 1;
    protected const int VIEW_BRIEFING = 2;
    protected const int VIEW_GAME = 3;
    protected const int VIEW_DEBRIEF = 4;

    protected ref array<GC_HeaderButtonComponent> m_aButtons = {};
    protected GC_LobbyManager m_LobbyManager;
    protected GC_LobbyGameMode m_LobbyGameMode;
    protected int m_iActiveState = -1;
    protected bool m_bInitialized;
    protected bool m_bAdminPermissionLogged;
    protected int m_iManagerCheckAttempts;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        GetGame().GetCallqueue().CallLater(InitializeHeader, 0);
    }

    override void HandlerDeattached(Widget w)
    {
        GetGame().GetCallqueue().Remove(InitializeHeader);
        GetGame().GetCallqueue().Remove(RefreshAdminPermission);
        GetGame().GetCallqueue().Remove(AttachLobbyManager);

        if (m_LobbyManager)
            m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);

        if (m_LobbyGameMode)
            m_LobbyGameMode.GetOnPlayerRoleChange().Remove(OnPlayerRoleChanged);

        foreach (GC_HeaderButtonComponent button : m_aButtons)
        {
            if (button)
                button.m_OnClicked.Remove(OnButtonClicked);
        }

        m_aButtons.Clear();
        m_LobbyManager = null;
        m_LobbyGameMode = null;
        m_bInitialized = false;
        m_bAdminPermissionLogged = false;
        super.HandlerDeattached(w);
    }

    protected void InitializeHeader()
    {
        if (m_bInitialized)
            return;

        m_bInitialized = true;

        RegisterButton("Mission Preview Button");
        RegisterButton("Lobby Button");
        RegisterButton("Briefing Button");
        RegisterButton("Game Button");
        RegisterButton("Debrief Button");

        AttachLobbyManager();
        RefreshAdminPermission();
        RefreshButtonStates();
    }

    protected void AttachLobbyManager()
    {
        if (m_LobbyManager)
            return;

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (!m_LobbyManager)
        {
            if (m_iManagerCheckAttempts++ < 20)
                GetGame().GetCallqueue().CallLater(AttachLobbyManager, 250);

            RefreshButtonStates();
            return;
        }

        m_iManagerCheckAttempts = 0;
        m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);

        m_LobbyGameMode = GC_LobbyGameMode.GetGameMode();
        if (m_LobbyGameMode)
            m_LobbyGameMode.GetOnPlayerRoleChange().Insert(OnPlayerRoleChanged);

        if (m_iActiveState < 0)
            m_iActiveState = GetPhaseView(m_LobbyManager.GetLobbyPhase());

        RefreshButtonStates();
    }

    protected void RefreshAdminPermission()
    {
		GetGame().GetCallqueue().Remove(RefreshAdminPermission);
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
        {
            RefreshButtonStates();
            GetGame().GetCallqueue().CallLater(RefreshAdminPermission, 250);
            return;
        }

        if (!m_bAdminPermissionLogged)
        {
            m_bAdminPermissionLogged = true;
            EPlayerRole roleFlags = GetGame().GetPlayerManager().GetPlayerRoles(playerController.GetPlayerId());
            string phaseName = "UNAVAILABLE";
            if (m_LobbyManager)
                phaseName = GC_LobbyManager.GetPhaseName(m_LobbyManager.GetLobbyPhase());
            PrintFormat("GC | Admin Trace | header initial permission: player=%1 roleFlags=%2 flagsAdmin=%3 queriedAdmin=%4 manager=%5 phase=%6", playerController.GetPlayerId(), roleFlags, SCR_Global.IsAdminRole(roleFlags), SCR_Global.IsAdmin(playerController.GetPlayerId()), m_LobbyManager, phaseName);
        }

        RefreshButtonStates();
        GetGame().GetCallqueue().CallLater(RefreshAdminPermission, 250);
    }

    protected void OnPlayerRoleChanged(int playerId, EPlayerRole roleFlags)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        int localPlayerId = -1;
        if (playerController)
            localPlayerId = playerController.GetPlayerId();

        bool queriedAdmin = playerController && SCR_Global.IsAdmin(localPlayerId);
        PrintFormat("GC | Admin Trace | header role event: changedPlayer=%1 localPlayer=%2 roleFlags=%3 flagsAdmin=%4 queriedAdmin=%5 isLocal=%6", playerId, localPlayerId, roleFlags, SCR_Global.IsAdminRole(roleFlags), queriedAdmin, localPlayerId == playerId);
        if (!playerController || localPlayerId != playerId)
            return;

        RefreshButtonStates();
        TraceButtonStates("role-event");
    }

    protected void RegisterButton(string buttonName)
    {
        Widget buttonRoot = m_wRoot.FindAnyWidget(buttonName);
        if (!buttonRoot)
        {
            PrintFormat("GC | Header button not found: %1", buttonName);
            return;
        }

        GC_HeaderButtonComponent button = GC_HeaderButtonComponent.Cast(buttonRoot.FindHandler(GC_HeaderButtonComponent));
        if (!button)
        {
            PrintFormat("GC | Header component missing on button: %1", buttonName);
            return;
        }

        m_aButtons.Insert(button);
        button.m_OnClicked.Insert(OnButtonClicked);
    }

    void SetActiveState(int state)
    {
        m_iActiveState = state;
        RefreshButtonStates();
    }

    int GetActiveState()
    {
        return m_iActiveState;
    }

    protected void RefreshButtonStates()
    {
        foreach (GC_HeaderButtonComponent button : m_aButtons)
        {
            if (!button)
                continue;

            bool selected = button.GetHeaderState() == m_iActiveState;
            bool interactive = IsButtonInteractive(button.GetHeaderState(), selected);
            button.SetHeaderSelected(selected);
            button.SetHeaderLocked(selected);
            button.SetHeaderEnabled(interactive);

            if (!interactive)
                button.SetHeaderHovered(false);
        }
    }

    protected void TraceButtonStates(string source)
    {
        SCR_EGameModeState phase = -1;
        if (m_LobbyManager)
            phase = m_LobbyManager.GetLobbyPhase();

        foreach (GC_HeaderButtonComponent button : m_aButtons)
        {
            if (!button)
                continue;

            bool selected = button.GetHeaderState() == m_iActiveState;
            PrintFormat("GC | Admin Trace | header button state: source=%1 playerAdmin=%2 phase=%3 activeView=%4 buttonView=%5 selected=%6 interactive=%7", source, IsLocalAdmin(), GC_LobbyManager.GetPhaseName(phase), m_iActiveState, button.GetHeaderState(), selected, IsButtonInteractive(button.GetHeaderState(), selected));
        }
    }

    protected bool IsButtonInteractive(int view, bool selected)
    {
        if (selected || !m_LobbyManager)
            return false;

        SCR_EGameModeState phase = m_LobbyManager.GetLobbyPhase();
        if (CanNavigateToView(view, phase))
            return true;

        return IsLocalAdmin() && IsNextPhaseView(view, phase);
    }

    protected bool IsLocalAdmin()
    {
        PlayerController playerController = GetGame().GetPlayerController();
        return playerController && SCR_Global.IsAdmin(playerController.GetPlayerId());
    }

    protected bool CanNavigateToView(int view, SCR_EGameModeState phase)
    {
        switch (phase)
        {
            case SCR_EGameModeState.PREGAME:
				return view == VIEW_PREVIEW;

			case SCR_EGameModeState.SLOTTING:
				return view == VIEW_LOBBY;

            case SCR_EGameModeState.BRIEFING:
				return view == VIEW_BRIEFING;
        }

        return false;
    }

    protected bool IsNextPhaseView(int view, SCR_EGameModeState phase)
    {
        switch (phase)
        {
            case SCR_EGameModeState.PREGAME:
				return view == VIEW_LOBBY;

			case SCR_EGameModeState.SLOTTING:
				return view == VIEW_BRIEFING;

            case SCR_EGameModeState.BRIEFING:
                return view == VIEW_GAME;

            case SCR_EGameModeState.GAME:
                return view == VIEW_DEBRIEF;
        }

        return false;
    }

    protected SCR_EGameModeState GetRequestedPhase(int view)
    {
        switch (view)
        {
			case VIEW_LOBBY: return SCR_EGameModeState.SLOTTING;
            case VIEW_BRIEFING: return SCR_EGameModeState.BRIEFING;
            case VIEW_GAME: return SCR_EGameModeState.GAME;
            case VIEW_DEBRIEF: return SCR_EGameModeState.POSTGAME;
        }

        return -1;
    }

    protected int GetPhaseView(SCR_EGameModeState phase)
    {
        switch (phase)
        {
			case SCR_EGameModeState.SLOTTING: return VIEW_LOBBY;
            case SCR_EGameModeState.BRIEFING: return VIEW_BRIEFING;
            case SCR_EGameModeState.GAME: return VIEW_GAME;
            case SCR_EGameModeState.POSTGAME: return VIEW_DEBRIEF;
        }

        return VIEW_PREVIEW;
    }

    protected void OnButtonClicked(SCR_ButtonBaseComponent baseButton)
    {
        GC_HeaderButtonComponent button = GC_HeaderButtonComponent.Cast(baseButton);
        if (!button || !m_LobbyManager)
            return;

        int view = button.GetHeaderState();
        bool selected = view == m_iActiveState;
        bool interactive = IsButtonInteractive(view, selected);
        PrintFormat("GC | Admin Trace | header click dispatched: playerAdmin=%1 phase=%2 activeView=%3 buttonView=%4 selected=%5 interactive=%6", IsLocalAdmin(), GC_LobbyManager.GetPhaseName(m_LobbyManager.GetLobbyPhase()), m_iActiveState, view, selected, interactive);
        if (!interactive)
            return;

        SCR_EGameModeState phase = m_LobbyManager.GetLobbyPhase();
        if (CanNavigateToView(view, phase))
        {
			PrintFormat("GC | Phase UI | navigation selected: phase=%1 fromView=%2 toView=%3", GC_LobbyManager.GetPhaseName(phase), m_iActiveState, view);
            NavigateToView(view);
            return;
        }

        if (!IsLocalAdmin() || !IsNextPhaseView(view, phase))
            return;

        GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
        if (!playerController)
            return;

		PrintFormat("GC | Phase UI | admin advance selected: player=%1 current=%2 target=%3", playerController.GetPlayerId(), GC_LobbyManager.GetPhaseName(phase), GC_LobbyManager.GetPhaseName(GetRequestedPhase(view)));
        playerController.RequestLobbyPhaseAdvance(GetRequestedPhase(view));
    }

    protected void NavigateToView(int view)
    {
        SetActiveState(view);
        m_OnStateChanged.Invoke(this);

        if (view == VIEW_LOBBY)
            SwitchToLobbyMenu();
        else if (view == VIEW_PREVIEW)
            SwitchToPreviewMenu();
        else if (view == VIEW_BRIEFING)
            SwitchToBriefingMenu();
    }

    protected void OnLobbyPhaseChanged(SCR_EGameModeState phase)
    {
		PrintFormat("GC | Phase UI | replicated phase update: phase=%1 activeView=%2", GC_LobbyManager.GetPhaseName(phase), m_iActiveState);
		// GC_LobbyGameMode owns authoritative phase presentation so transitions
		// still apply when no header exists yet, including JIP. The header only
		// reflects that phase and handles local navigation within it.
		SetActiveState(GetPhaseView(phase));
    }

    protected void SwitchToPreviewMenu()
    {
        CloseCurrentMenu();
        GetGame().GetCallqueue().CallLater(OpenPreviewMenu, 0);
    }

    protected void SwitchToLobbyMenu()
    {
        CloseCurrentMenu();
        GetGame().GetCallqueue().CallLater(OpenLobbyMenu, 0);
    }

    protected void SwitchToBriefingMenu()
    {
        CloseCurrentMenu();
        GetGame().GetCallqueue().CallLater(OpenBriefingMenu, 0);
    }

    protected void CloseCurrentMenu()
    {
        MenuManager menuManager = GetGame().GetMenuManager();
        if (!menuManager)
            return;

        MenuBase currentMenu = menuManager.GetTopMenu();
        if (currentMenu)
            currentMenu.Close();
    }

    protected void OpenPreviewMenu()
    {
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (!lobbyManager)
            return;

        SCR_EGameModeState phase = lobbyManager.GetLobbyPhase();
        if (phase != SCR_EGameModeState.PREGAME)
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_PreviewMenu, 0, true);
    }

    protected void OpenBriefingMenu()
    {
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (!lobbyManager || lobbyManager.GetLobbyPhase() != SCR_EGameModeState.BRIEFING)
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_BriefingMenu, 0, true);
    }

    protected void OpenLobbyMenu()
    {
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (!lobbyManager)
            return;

        SCR_EGameModeState phase = lobbyManager.GetLobbyPhase();
		if (phase != SCR_EGameModeState.SLOTTING)
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
    }
}
