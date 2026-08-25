//! The base manual camera intentionally accepts input only while no menu is
//! open or while the top menu is an EditorMenuUI. Using the camera-compatible
//! base here keeps Lobby2 scoped and avoids a global SCR_ManualCamera override.
class GC_Lobby_SpectatorMenu : EditorMenuUI
{
	protected InputManager m_InputManager;
	protected SCR_ButtonBaseComponent m_InterfaceOptionButton;
	protected TextWidget m_wInterfaceOptionState;
	protected Widget m_wInterfaceOptionStateBackground;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		WidgetManager.SetCursor(0);

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
			m_InputManager.AddActionListener("GC_ToggleSpectatorInterface", EActionTrigger.DOWN, OnToggleSpectatorInterface);
		}

		Widget root = GetRootWidget();
		Widget optionButton;
		if (root)
			optionButton = root.FindAnyWidget("SpectatorInterfaceOptionButton");
		if (optionButton)
			m_InterfaceOptionButton = SCR_ButtonBaseComponent.Cast(optionButton.FindHandler(SCR_ButtonBaseComponent));
		if (root)
		{
			m_wInterfaceOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorInterfaceOptionState"));
			m_wInterfaceOptionStateBackground = root.FindAnyWidget("SpectatorInterfaceOptionStateBackground");
		}
		if (m_InterfaceOptionButton)
			m_InterfaceOptionButton.m_OnClicked.Insert(OnToggleSpectatorInterface);

		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		SetInterfaceVisible(!playerController || playerController.IsSpectatorInterfaceVisible());
	}

	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		SetInterfaceVisible(!playerController || playerController.IsSpectatorInterfaceVisible());
	}

	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		if (m_InterfaceOptionButton)
			m_InterfaceOptionButton.m_OnClicked.Remove(OnToggleSpectatorInterface);
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorInterface", EActionTrigger.DOWN, OnToggleSpectatorInterface);
		}

		m_wInterfaceOptionStateBackground = null;
		m_wInterfaceOptionState = null;
		m_InterfaceOptionButton = null;
		m_InputManager = null;
		super.OnMenuClose();
	}

	void SetInterfaceVisible(bool visible)
	{
		if (visible)
			WidgetManager.SetCursor(0);
		else
			WidgetManager.SetCursor(-1);

		if (m_wInterfaceOptionState)
		{
			if (visible)
				m_wInterfaceOptionState.SetText("ON");
			else
				m_wInterfaceOptionState.SetText("OFF");
		}
		if (m_wInterfaceOptionStateBackground)
		{
			if (visible)
				m_wInterfaceOptionStateBackground.SetColor(Color.FromRGBA(48, 148, 112, 255));
			else
				m_wInterfaceOptionStateBackground.SetColor(Color.FromRGBA(82, 88, 90, 255));
		}

		Widget root = GetRootWidget();
		if (root)
			root.SetVisible(visible);
	}

	protected void OnToggleSpectatorInterface()
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.ToggleSpectatorInterface();
	}

	protected void OnPauseMenuRequested()
	{
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	override protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu(false, true);
	}
}
