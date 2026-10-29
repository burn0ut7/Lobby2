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
    protected bool m_bInitialized;
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
        GetGame().GetCallqueue().Remove(AttachLobbyManager);
		GetGame().GetCallqueue().Remove(RefreshPlayerCount);
		GetGame().GetCallqueue().Remove(RefreshPhaseClock);

        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);
			m_LobbyManager.m_OnPlayerConnected.Remove(OnPlayerCountChanged);
			m_LobbyManager.m_OnPlayerDisconnected.Remove(OnPlayerCountChanged);
		}

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
		UpdateMissionHeader(SCR_MissionHeader.Cast(GetGame().GetMissionHeader()));

        RegisterButton("Mission Preview Button");
        RegisterButton("Lobby Button");
        RegisterButton("Briefing Button");
        RegisterButton("Game Button");
        RegisterButton("Debrief Button");

        AttachLobbyManager();
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
		m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);
		m_LobbyManager.m_OnPlayerConnected.Insert(OnPlayerCountChanged);
		m_LobbyManager.m_OnPlayerDisconnected.Insert(OnPlayerCountChanged);
		RefreshPlayerCount();
		UpdatePhaseClockSchedule();

        if (m_iActiveState < 0)
            m_iActiveState = GetPhaseView(m_LobbyManager.GetLobbyPhase());

        RefreshButtonStates();
    }

	//------------------------------------------------------------------------------------------------
	//! Bind authored information only; editor Play can have no mission header.
	protected void UpdateMissionHeader(SCR_MissionHeader mission)
	{
		TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("TitleText"));
		TextWidget description = TextWidget.Cast(m_wRoot.FindAnyWidget("Description"));
		string missionName;
		string missionDescription;
		if (mission)
		{
			missionName = mission.m_sName;
			missionDescription = WidgetManager.Translate(mission.m_sGameMode);
			if (!mission.m_sDescription.IsEmpty())
			{
				if (!missionDescription.IsEmpty())
					missionDescription += " · ";
				missionDescription += WidgetManager.Translate(mission.m_sDescription);
			}
		}

		if (title)
		{
			if (missionName.IsEmpty())
				title.SetText("Mission information unavailable");
			else
				title.SetText(missionName);
		}
		if (description)
		{
			description.SetText(missionDescription);
			description.SetVisible(!missionDescription.IsEmpty());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePhaseClockSchedule()
	{
		GetGame().GetCallqueue().Remove(RefreshPhaseClock);
		RefreshPhaseClock();
		if (m_LobbyManager)
			GetGame().GetCallqueue().CallLater(RefreshPhaseClock, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshPhaseClock()
	{
		TextWidget clock = TextWidget.Cast(m_wRoot.FindAnyWidget("PhaseElapsed"));
		if (!clock)
			return;
		int elapsed = -1;
		if (m_LobbyManager)
			elapsed = m_LobbyManager.GetPhaseElapsedSeconds();
		clock.SetVisible(elapsed >= 0);
		if (elapsed < 0)
			return;

		int minutes = elapsed / 60;
		int seconds = elapsed % 60;
		clock.SetTextFormat("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerCountChanged(int playerId)
	{
		// Disconnect notification precedes native player-list cleanup.
		GetGame().GetCallqueue().Remove(RefreshPlayerCount);
		GetGame().GetCallqueue().CallLater(RefreshPlayerCount, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshPlayerCount()
	{
		TextWidget count = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerCountText"));
		PlayerManager players = GetGame().GetPlayerManager();
		if (!count)
			return;
		if (!players)
		{
			count.SetText(string.Empty);
			return;
		}
		array<int> connected = {};
		players.GetPlayers(connected);
		count.SetTextFormat("%1 connected", connected.Count());
	}

    protected void OnPlayerRoleChanged(int playerId)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        int localPlayerId = -1;
        if (playerController)
            localPlayerId = playerController.GetPlayerId();

        if (!playerController || localPlayerId != playerId)
            return;

        RefreshButtonStates();
		if (GC_UIEventDiagnostics.ENABLED)
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
		if (view == VIEW_DEBRIEF)
			return phase == SCR_EGameModeState.POSTGAME;
		if (view == VIEW_PREVIEW)
			return phase == SCR_EGameModeState.PREGAME;
		if (view == VIEW_LOBBY)
			return GC_LobbyPhase.IsAtOrAfter(phase, SCR_EGameModeState.SLOTTING);
		if (view == VIEW_BRIEFING)
			return GC_LobbyPhase.IsAtOrAfter(phase, SCR_EGameModeState.BRIEFING) && HasLocalPlayerFaction();
		if (view == VIEW_GAME)
			return GC_LobbyPhase.IsAtOrAfter(phase, SCR_EGameModeState.GAME);

		return false;
    }

	protected bool HasLocalPlayerFaction()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		return playerController && m_LobbyManager && m_LobbyManager.GetPlayerFaction(playerController.GetPlayerId());
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
		if (GC_UIEventDiagnostics.ENABLED)
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
		else if (view == VIEW_GAME)
			CloseCurrentMenu();
		else if (view == VIEW_DEBRIEF)
		{
			CloseCurrentMenu();
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_DebriefMenu, 0, true);
		}
    }

    protected void OnLobbyPhaseChanged(SCR_EGameModeState phase)
    {
		UpdatePhaseClockSchedule();
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
        if (!lobbyManager || !GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.BRIEFING))
            return;
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController || !lobbyManager.GetPlayerFaction(playerController.GetPlayerId()))
			return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_BriefingMenu, 0, true);
    }

    protected void OpenLobbyMenu()
    {
        GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
        if (!lobbyManager)
            return;

		SCR_EGameModeState phase = lobbyManager.GetLobbyPhase();
		if (!GC_LobbyPhase.IsAtOrAfter(phase, SCR_EGameModeState.SLOTTING))
            return;

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
    }
}
