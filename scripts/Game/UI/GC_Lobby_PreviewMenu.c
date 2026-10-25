class GC_Lobby_PreviewMenu : ChimeraMenuBase
{
	protected static const string INPUT_CONTEXT = "GC_Lobby_Preview";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;

    protected SCR_MapEntity m_MapEntity;
	protected ref GC_LobbyMenuChat m_MenuChat;
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

		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		string phaseName = "NO_GAME_MODE";
		if (gameMode)
			phaseName = GC_LobbyManager.GetPhaseName(gameMode.GetState());
		PrintFormat("GC | Phase UI | preview/briefing menu opened: menu=%1 state=%2", this, phaseName);

		m_MenuChat = new GC_LobbyMenuChat();
		m_MenuChat.Start(this);

		Widget headerRoot = GetRootWidget().FindAnyWidget("MenuBar");
		if (headerRoot)
			m_Header = GC_HeaderComponent.Cast(headerRoot.FindHandler(GC_HeaderComponent));

		Widget playerListPanel = GetRootWidget().FindAnyWidget("PlayerListPanel");
		if (playerListPanel)
			m_PlayerListTabs = GC_PlayerListTabsComponent.Cast(playerListPanel.FindHandler(GC_PlayerListTabsComponent));

		m_LobbyManager = GC_LobbyManager.GetManager();
		SCR_EGameModeState phase = SCR_EGameModeState.PREGAME;
		if (m_LobbyManager)
		{
			phase = m_LobbyManager.GetLobbyPhase();
			m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);
		}

		ApplyViewPresentation();
		m_iEditorManagerAttachAttempts = 0;
		AttachEditorManager();

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);
		}

        // Match the shipped preview-menu pattern: let the menu layout finish
        // creating, then open the map on the following frame.
        GetGame().GetCallqueue().CallLater(OpenMap, 0);
    }

    override void OnMenuUpdate(float tDelta)
    {
		if (m_MenuChat)
			m_MenuChat.Update(tDelta);
    }

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		if (m_MenuChat)
			m_MenuChat.Show();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusLost()
	{
		if (m_MenuChat)
			m_MenuChat.Hide();
		super.OnMenuFocusLost();
	}

    override void OnMenuClose()
    {
		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		string phaseName = "NO_GAME_MODE";
		if (gameMode)
			phaseName = GC_LobbyManager.GetPhaseName(gameMode.GetState());
		PrintFormat("GC | Phase UI | preview/briefing menu closed: menu=%1 state=%2", this, phaseName);

		if (m_LobbyManager)
			m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
		if (m_EditorManager)
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);

		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(RestoreMapActionsAfterEditor);
        GetGame().GetCallqueue().Remove(OpenMap);
		GetGame().GetCallqueue().Remove(OpenMapNow);
		SCR_MapEntity.GetOnMapClose().Remove(OnPreviousMapClosed);
		GetGame().GetCallqueue().Remove(OpenPauseMenu);

		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);
		}

		if (m_MenuChat)
			m_MenuChat.Stop();

		if (m_MapEntity && m_MapEntity.IsOpen() && m_MapEntity.GetMapConfig().RootWidgetRef == GetRootWidget())
			m_MapEntity.CloseMap();

		m_Header = null;
		m_EditorManager = null;
		m_LobbyManager = null;
		m_PlayerListTabs = null;
		m_MenuChat = null;
		m_InputManager = null;
    }

	protected void OnPauseMenuRequested()
	{
		if (!IsFocused())
			return;
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

	protected void OnLobbyPhaseChanged(SCR_EGameModeState phase)
	{
		ApplyViewPresentation();
	}

	protected bool IsBriefingView()
	{
		return false;
	}

	protected void ApplyViewPresentation()
	{
		bool isBriefing = IsBriefingView();

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

	protected void OpenMap()
    {
        GetGame().GetCallqueue().CallLater(OpenMapNow, 0);
    }

	protected void OnPreviousMapClosed(MapConfiguration config)
	{
		SCR_MapEntity.GetOnMapClose().Remove(OnPreviousMapClosed);
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
		if (m_MapEntity.IsOpen())
		{
			// The gadget closes its menu after an animation. Let its close handlers
			// finish before replacing the singleton map configuration.
			SCR_MapEntity.GetOnMapClose().Remove(OnPreviousMapClosed);
			SCR_MapEntity.GetOnMapClose().Insert(OnPreviousMapClosed);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
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
		EMapEntityMode mapMode = EMapEntityMode.GC_BRIEFING;
		if (!IsBriefingView())
		{
			mapConfigPath = "{6A75000000000002}Configs/Map/MapPreview.conf";
			mapMode = EMapEntityMode.GC_PREVIEW;
		}
		PrintFormat("GC | OpenMap config=%1 root=%2", mapConfigPath, rootWidget);
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(mapMode, mapConfigPath, rootWidget);
		if (!mapConfigFullscreen)
		{
			Print("GC | OpenMap aborted: map configuration could not be created", LogLevel.ERROR);
			return;
		}

		m_MapEntity.OpenMap(mapConfigFullscreen);
    }
}
