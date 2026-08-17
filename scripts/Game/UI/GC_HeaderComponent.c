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
    protected int m_iActiveState = -1;
    protected bool m_bIsAdmin;
    protected bool m_bInitialized;
    protected int m_iAdminCheckAttempts;
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

        foreach (GC_HeaderButtonComponent button : m_aButtons)
        {
            if (button)
                button.m_OnClicked.Remove(OnButtonClicked);
        }

        m_aButtons.Clear();
        m_LobbyManager = null;
        m_bInitialized = false;
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
            m_bIsAdmin = false;
            if (m_iAdminCheckAttempts++ < 20)
                GetGame().GetCallqueue().CallLater(RefreshAdminPermission, 250);

            RefreshButtonStates();
            return;
        }

        m_iAdminCheckAttempts = 0;
        m_bIsAdmin = SCR_Global.IsAdmin();
        RefreshButtonStates();
		GetGame().GetCallqueue().CallLater(RefreshAdminPermission, 1000);
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

    void SetCanChangeState(bool canChange)
    {
        m_bIsAdmin = canChange;
        RefreshButtonStates();
    }

    bool CanChangeState()
    {
        return m_bIsAdmin;
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

    protected bool IsButtonInteractive(int view, bool selected)
    {
        if (selected || !m_LobbyManager)
            return false;

        GC_ELobbyPhase phase = m_LobbyManager.GetLobbyPhase();
        if (CanNavigateToView(view, phase))
            return true;

        return m_bIsAdmin && IsNextPhaseView(view, phase);
    }

    protected bool CanNavigateToView(int view, GC_ELobbyPhase phase)
    {
        switch (phase)
        {
            case GC_ELobbyPhase.PREGAME:
                return view == VIEW_PREVIEW || view == VIEW_LOBBY;

            case GC_ELobbyPhase.BRIEFING:
                return view == VIEW_LOBBY || view == VIEW_BRIEFING;
        }

        return false;
    }

    protected bool IsNextPhaseView(int view, GC_ELobbyPhase phase)
    {
        switch (phase)
        {
            case GC_ELobbyPhase.PREGAME:
                return view == VIEW_BRIEFING;

            case GC_ELobbyPhase.BRIEFING:
                return view == VIEW_GAME;

            case GC_ELobbyPhase.GAME:
                return view == VIEW_DEBRIEF;
        }

        return false;
    }

    protected GC_ELobbyPhase GetRequestedPhase(int view)
    {
        switch (view)
        {
            case VIEW_BRIEFING: return GC_ELobbyPhase.BRIEFING;
            case VIEW_GAME: return GC_ELobbyPhase.GAME;
            case VIEW_DEBRIEF: return GC_ELobbyPhase.POSTGAME;
        }

        return -1;
    }

    protected int GetPhaseView(GC_ELobbyPhase phase)
    {
        switch (phase)
        {
            case GC_ELobbyPhase.BRIEFING: return VIEW_BRIEFING;
            case GC_ELobbyPhase.GAME: return VIEW_GAME;
            case GC_ELobbyPhase.POSTGAME: return VIEW_DEBRIEF;
        }

        return VIEW_PREVIEW;
    }

    protected void OnButtonClicked(SCR_ButtonBaseComponent baseButton)
    {
        GC_HeaderButtonComponent button = GC_HeaderButtonComponent.Cast(baseButton);
        if (!button || !m_LobbyManager)
            return;

        RefreshAdminPermission();

        int view = button.GetHeaderState();
        bool selected = view == m_iActiveState;
        if (!IsButtonInteractive(view, selected))
            return;

        GC_ELobbyPhase phase = m_LobbyManager.GetLobbyPhase();
        if (CanNavigateToView(view, phase))
        {
			PrintFormat("GC | Phase UI | navigation selected: phase=%1 fromView=%2 toView=%3", GC_LobbyManager.GetPhaseName(phase), m_iActiveState, view);
            NavigateToView(view);
            return;
        }

        if (!m_bIsAdmin || !IsNextPhaseView(view, phase))
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
        else if (view == VIEW_PREVIEW || view == VIEW_BRIEFING)
            SwitchToPreviewMenu();
    }

    protected void OnLobbyPhaseChanged(GC_ELobbyPhase phase)
    {
		PrintFormat("GC | Phase UI | replicated phase update: phase=%1 activeView=%2", GC_LobbyManager.GetPhaseName(phase), m_iActiveState);
        RefreshButtonStates();

        if (phase == GC_ELobbyPhase.BRIEFING)
        {
            if (m_iActiveState == VIEW_PREVIEW)
                SetActiveState(VIEW_BRIEFING);
            else
                SwitchToPreviewMenu();

            return;
        }

        if (phase == GC_ELobbyPhase.GAME || phase == GC_ELobbyPhase.POSTGAME)
        {
            SetActiveState(GetPhaseView(phase));
            CloseCurrentMenu();
        }
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

        GC_ELobbyPhase phase = lobbyManager.GetLobbyPhase();
        if (phase != GC_ELobbyPhase.PREGAME && phase != GC_ELobbyPhase.BRIEFING)
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_PreviewMenu, 0, true);
    }

    protected void OpenLobbyMenu()
    {
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (!lobbyManager)
            return;

        GC_ELobbyPhase phase = lobbyManager.GetLobbyPhase();
        if (phase != GC_ELobbyPhase.PREGAME && phase != GC_ELobbyPhase.BRIEFING)
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
    }
}
