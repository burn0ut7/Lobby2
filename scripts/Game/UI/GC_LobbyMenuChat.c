//! Owns native chat input/update/cleanup for Lobby2 menus that contain ChatHud.
class GC_LobbyMenuChat
{
	protected MenuBase m_Menu;
	protected SCR_ChatPanel m_Panel;
	protected InputManager m_InputManager;

	//------------------------------------------------------------------------------------------------
	void Start(MenuBase menu)
	{
		m_Menu = menu;
		Widget panelWidget = menu.GetRootWidget().FindAnyWidget("ChatPanel");
		if (!panelWidget)
			return;

		m_Panel = SCR_ChatPanel.Cast(panelWidget.FindHandler(SCR_ChatPanel));
		m_InputManager = GetGame().GetInputManager();
		if (m_Panel && m_InputManager)
			m_InputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, OnToggleRequested);
		Show();
	}

	//------------------------------------------------------------------------------------------------
	void Show()
	{
		if (!m_Panel || !GC_LobbyManager.GetManager())
			return;

		SCR_ChatPanelManager manager = SCR_ChatPanelManager.GetInstance();
		manager.HideAllChatPanels();
		manager.ShowChatPanel(m_Panel);
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		Close();
		if (m_Panel)
			m_Panel.GetWidget().SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void Update(float timeSlice)
	{
		if (!m_Panel)
			return;

		if (GetGame().GetMenuManager().GetTopMenu() != m_Menu || !m_Menu.GetRootWidget().IsVisible())
			Close();
		m_Panel.OnUpdateChat(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(Toggle);
		if (m_Panel && m_Panel.IsOpen())
			SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_Panel);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		Close();
		if (m_InputManager)
			m_InputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, OnToggleRequested);
		m_InputManager = null;
		m_Panel = null;
		m_Menu = null;
		GetGame().GetCallqueue().CallLater(SCR_ChatPanelManager.GetInstance().OnMenuClosed, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggleRequested()
	{
		GetGame().GetCallqueue().Remove(Toggle);
		GetGame().GetCallqueue().CallLater(Toggle, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Toggle()
	{
		if (!GC_LobbyManager.GetManager() || !m_Panel || !m_Menu)
			return;
		if (GetGame().GetMenuManager().GetTopMenu() != m_Menu || !m_Menu.GetRootWidget().IsVisible())
			return;

		if (m_Panel.IsOpen())
			Close();
		else
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_Panel);
	}
}

//! Lobby menus own their chat input, including while the spectator HUD is hidden.
//! Do not let the gameplay HUD open an invisible input that consumes later actions.
modded class SCR_ChatHud
{
	//------------------------------------------------------------------------------------------------
	override protected void Callback_OnToggleAction()
	{
		if (GC_LobbyManager.GetManager())
		{
			MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
			if (GC_Lobby_PreviewMenu.Cast(menu) || GC_Lobby_SlottingMenu.Cast(menu) || GC_Lobby_SpectatorMenu.Cast(menu))
				return;
		}
		super.Callback_OnToggleAction();
	}
}

//! A gameplay HUD may attach after the lobby menu. Keep native panel selection
//! consistent with MenuRootBase.ShowChat when that late panel registers.
[BaseContainerProps(configRoot: true)]
modded class SCR_ChatPanelManager
{
	//------------------------------------------------------------------------------------------------
	override void Register(SCR_ChatPanel panel)
	{
		super.Register(panel);
		if (!GC_LobbyManager.GetManager())
			return;

		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!GC_Lobby_PreviewMenu.Cast(menu) && !GC_Lobby_SlottingMenu.Cast(menu) && !GC_Lobby_SpectatorMenu.Cast(menu))
			return;

		Widget root = menu.GetRootWidget();
		Widget chatWidget = root.FindAnyWidget("ChatPanel");
		if (!chatWidget)
			return;

		SCR_ChatPanel menuPanel = SCR_ChatPanel.Cast(chatWidget.FindHandler(SCR_ChatPanel));
		if (!menuPanel)
			return;

		HideAllChatPanels();
		ShowChatPanel(menuPanel);
	}
}
