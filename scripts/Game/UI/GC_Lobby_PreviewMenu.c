class GC_Lobby_PreviewMenu : ChimeraMenuBase
{
	protected static const string INPUT_CONTEXT = "GC_Lobby_Preview";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;

    protected SCR_MapEntity m_MapEntity;
    protected SCR_ChatPanel m_ChatPanel;
	protected GC_HeaderComponent m_Header;
	protected GC_LobbyManager m_LobbyManager;
	protected GC_PlayerListTabsComponent m_PlayerListTabs;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected int m_iEditorManagerAttachAttempts;
	protected InputManager m_InputManager;

    override void OnMenuInit()
    {
        m_MapEntity = SCR_MapEntity.GetMapInstance();
        PrintFormat("GC | Preview menu map entity: %1", m_MapEntity);
    }

    override void OnMenuOpen()
    {
        if (SCR_Global.IsEditMode())
            return;

        Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
        if (wChatPanel)
            m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));

		Widget headerRoot = GetRootWidget().FindAnyWidget("MenuBar");
		if (headerRoot)
			m_Header = GC_HeaderComponent.Cast(headerRoot.FindHandler(GC_HeaderComponent));

		Widget playerListPanel = GetRootWidget().FindAnyWidget("PlayerListPanel");
		if (playerListPanel)
			m_PlayerListTabs = GC_PlayerListTabsComponent.Cast(playerListPanel.FindHandler(GC_PlayerListTabsComponent));

		m_LobbyManager = GC_LobbyManager.GetManager();
		GC_ELobbyPhase phase = GC_ELobbyPhase.PREGAME;
		if (m_LobbyManager)
		{
			phase = m_LobbyManager.GetLobbyPhase();
			m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);
		}

		ApplyPhasePresentation(phase);
		m_iEditorManagerAttachAttempts = 0;
		AttachEditorManager();

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
			m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);
		}

        // Match the shipped preview-menu pattern: let the menu layout finish
        // creating, then open the map on the following frame.
        GetGame().GetCallqueue().CallLater(OpenMap, 0);
    }

    override void OnMenuUpdate(float tDelta)
    {
        if (m_ChatPanel)
            m_ChatPanel.OnUpdateChat(tDelta);
    }

    override void OnMenuClose()
    {
		if (m_LobbyManager)
			m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
		if (m_EditorManager)
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);

		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(RestoreMapActionsAfterEditor);
        GetGame().GetCallqueue().Remove(OpenMap);
		GetGame().GetCallqueue().Remove(OpenMapNow);
		GetGame().GetCallqueue().Remove(ToggleChatWrap);
		GetGame().GetCallqueue().Remove(OpenPauseMenu);

		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
			m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);
		}

        if (m_ChatPanel && m_ChatPanel.IsOpen())
            SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);

        if (m_MapEntity)
            m_MapEntity.CloseMap();

		m_Header = null;
		m_EditorManager = null;
		m_LobbyManager = null;
		m_PlayerListTabs = null;
		m_ChatPanel = null;
		m_InputManager = null;
    }

	protected void OnPauseMenuRequested()
	{
		// Let the MenuBack event finish before opening the pause menu, otherwise
		// the same Escape press can immediately close the menu it just opened.
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	protected void OpenPauseMenu()
	{
		ArmaReforgerScripted.OpenPauseMenu(false, true);
	}

	protected void AttachEditorManager()
	{
		if (m_EditorManager)
			return;

		m_EditorManager = SCR_EditorManagerEntity.GetInstance();
		if (m_EditorManager)
		{
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);
			return;
		}

		m_iEditorManagerAttachAttempts++;
		if (m_iEditorManagerAttachAttempts < EDITOR_MANAGER_ATTACH_RETRIES)
			GetGame().GetCallqueue().CallLater(AttachEditorManager, 250);
	}

	protected void OnEditorClosed()
	{
		GetGame().GetCallqueue().Remove(RestoreMapActionsAfterEditor);
		GetGame().GetCallqueue().CallLater(RestoreMapActionsAfterEditor, 0);
	}

	protected void RestoreMapActionsAfterEditor()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
		{
			Print("GC | Cannot restore map actions after editor close: input manager is null", LogLevel.WARNING);
			return;
		}

		inputManager.ActivateContext(INPUT_CONTEXT);
		if (!inputManager.IsContextActive(INPUT_CONTEXT))
		{
			PrintFormat("GC | Failed to restore map action context '%1' after editor close", INPUT_CONTEXT, level: LogLevel.WARNING);
			return;
		}

		PrintFormat("GC | Restored map action context '%1' after editor close", INPUT_CONTEXT);
	}

	protected void OnLobbyPhaseChanged(GC_ELobbyPhase phase)
	{
		ApplyPhasePresentation(phase);
	}

	protected void ApplyPhasePresentation(GC_ELobbyPhase phase)
	{
		bool isBriefing = phase == GC_ELobbyPhase.BRIEFING;

		if (m_Header)
		{
			if (isBriefing)
				m_Header.SetActiveState(2);
			else
				m_Header.SetActiveState(0);
		}

		if (m_PlayerListTabs)
		{
			m_PlayerListTabs.SetPlayerListDefault(!isBriefing);
			m_PlayerListTabs.SetDarkBackground(!isBriefing);
		}
	}

    protected void Action_OnChatToggleAction()
    {
        if (!m_ChatPanel)
            return;

        GetGame().GetCallqueue().CallLater(ToggleChatWrap, 0);
    }

    protected void ToggleChatWrap()
    {
        if (!m_ChatPanel)
            return;

        if (m_ChatPanel.IsOpen())
            SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);
        else
            SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
    }

	protected void OpenMap()
    {
        GetGame().GetCallqueue().CallLater(OpenMapNow, 0);
    }

	protected void OpenMapNow()
    {
        Print("GC | OpenMap");

		if (!m_MapEntity)
		{
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			if (!m_MapEntity)
			{
				Print("GC | OpenMap aborted: no SCR_MapEntity exists in the active world", LogLevel.ERROR);
				return;
			}
		}

		Widget rootWidget = GetRootWidget();
		if (!rootWidget)
		{
			Print("GC | OpenMap aborted: preview menu root widget is null", LogLevel.ERROR);
			return;
		}

		if (!rootWidget.FindAnyWidget("MapWidget"))
		{
			Print("GC | OpenMap aborted: preview menu layout has no MapWidget", LogLevel.ERROR);
			return;
		}

		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		if (!gameMode)
		{
			Print("GC | OpenMap aborted: game mode is null", LogLevel.ERROR);
			return;
		}
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
		{
			Print("GC | OpenMap aborted: SCR_MapConfigComponent is missing", LogLevel.ERROR);
			return;
		}

		ResourceName mapConfigPath = "{6A115D5DFF7ED871}Configs/Map/MapLobby.conf";
		PrintFormat("GC | OpenMap config=%1 root=%2", mapConfigPath, rootWidget);
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, mapConfigPath, rootWidget);
		if (!mapConfigFullscreen)
		{
			Print("GC | OpenMap aborted: map configuration could not be created", LogLevel.ERROR);
			return;
		}

		m_MapEntity.OpenMap(mapConfigFullscreen);
    }
}
