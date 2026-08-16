modded enum ChimeraMenuPreset
{
    GC_Lobby_PreviewMenu
};

class GC_Lobby_PreviewMenu : ChimeraMenuBase
{
    protected SCR_MapEntity m_MapEntity;
    protected SCR_ChatPanel m_ChatPanel;

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

        GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

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
        GetGame().GetCallqueue().Remove(OpenMap);
        GetGame().GetCallqueue().Remove(OpenMapNow);
        GetGame().GetCallqueue().Remove(ToggleChatWrap);

        GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

        if (m_ChatPanel && m_ChatPanel.IsOpen())
            SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);

        if (m_MapEntity)
            m_MapEntity.CloseMap();
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

        BaseGameMode gameMode = GetGame().GetGameMode();
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
