//! The base manual camera intentionally accepts input only while no menu is
//! open or while the top menu is an EditorMenuUI. Using the camera-compatible
//! base here keeps Lobby2 scoped and avoids a global SCR_ManualCamera override.
class GC_Lobby_SpectatorMenu : EditorMenuUI
{
	protected static const float OPTION_TOOLTIP_GAP = 8;
	protected static const string INTERFACE_OPTION_DESCRIPTION = "Hide every spectator HUD element. Press the shown key to restore it.";

	protected InputManager m_InputManager;
	protected SCR_ButtonBaseComponent m_InterfaceOptionButton;
	protected Widget m_wInterfaceOptionSource;
	protected TextWidget m_wInterfaceOptionState;
	protected Widget m_wInterfaceOptionStateBackground;
	protected Widget m_wOptionTooltipLayer;
	protected Widget m_wOptionTooltip;
	protected TextWidget m_wOptionTooltipText;

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
		{
			m_wInterfaceOptionSource = optionButton;
			m_InterfaceOptionButton = SCR_ButtonBaseComponent.Cast(optionButton.FindHandler(SCR_ButtonBaseComponent));
		}
		if (root)
		{
			m_wInterfaceOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorInterfaceOptionState"));
			m_wInterfaceOptionStateBackground = root.FindAnyWidget("SpectatorInterfaceOptionStateBackground");
			m_wOptionTooltipLayer = root.FindAnyWidget("SpectatorOptionTooltipLayer");
			m_wOptionTooltip = root.FindAnyWidget("SpectatorOptionTooltip");
			m_wOptionTooltipText = TextWidget.Cast(root.FindAnyWidget("SpectatorOptionTooltipText"));
		}
		if (m_InterfaceOptionButton)
		{
			m_InterfaceOptionButton.m_OnClicked.Insert(OnToggleSpectatorInterface);
			m_InterfaceOptionButton.m_OnShowBorder.Insert(OnInterfaceOptionHoverChanged);
		}
		HideOptionTooltip();

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
		{
			m_InterfaceOptionButton.m_OnClicked.Remove(OnToggleSpectatorInterface);
			m_InterfaceOptionButton.m_OnShowBorder.Remove(OnInterfaceOptionHoverChanged);
		}
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorInterface", EActionTrigger.DOWN, OnToggleSpectatorInterface);
		}

		HideOptionTooltip();
		m_wOptionTooltipText = null;
		m_wOptionTooltip = null;
		m_wOptionTooltipLayer = null;
		m_wInterfaceOptionStateBackground = null;
		m_wInterfaceOptionState = null;
		m_wInterfaceOptionSource = null;
		m_InterfaceOptionButton = null;
		m_InputManager = null;
		super.OnMenuClose();
	}

	void SetInterfaceVisible(bool visible)
	{
		if (!visible)
			HideOptionTooltip();

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

	protected void OnInterfaceOptionHoverChanged(SCR_ButtonBaseComponent button, bool hovered)
	{
		if (button != m_InterfaceOptionButton)
			return;

		if (hovered)
			ShowOptionTooltip();
		else
			HideOptionTooltip();
	}

	protected void ShowOptionTooltip()
	{
		if (!m_wInterfaceOptionSource || !m_wOptionTooltipLayer || !m_wOptionTooltip || !m_wOptionTooltipText)
			return;

		m_wOptionTooltipText.SetText(INTERFACE_OPTION_DESCRIPTION);
		m_wOptionTooltip.SetVisible(true);

		float sourceX;
		float sourceY;
		float sourceWidth;
		float sourceHeight;
		float layerX;
		float layerY;
		float layerWidth;
		float layerHeight;
		float tooltipWidth;
		float tooltipHeight;
		m_wInterfaceOptionSource.GetScreenPos(sourceX, sourceY);
		m_wInterfaceOptionSource.GetScreenSize(sourceWidth, sourceHeight);
		m_wOptionTooltipLayer.GetScreenPos(layerX, layerY);
		m_wOptionTooltipLayer.GetScreenSize(layerWidth, layerHeight);
		m_wOptionTooltip.GetScreenSize(tooltipWidth, tooltipHeight);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float localX = workspace.DPIUnscale(sourceX - layerX - tooltipWidth) - OPTION_TOOLTIP_GAP;
		float localY = workspace.DPIUnscale(sourceY - layerY + (sourceHeight - tooltipHeight) * 0.5);
		float localTooltipHeight = workspace.DPIUnscale(tooltipHeight);
		float availableHeight = workspace.DPIUnscale(layerHeight);
		if (localY + localTooltipHeight > availableHeight)
			localY = availableHeight - localTooltipHeight;
		if (localY < 0)
			localY = 0;

		FrameSlot.SetPos(m_wOptionTooltip, localX, localY);
	}

	protected void HideOptionTooltip()
	{
		if (m_wOptionTooltip)
			m_wOptionTooltip.SetVisible(false);
	}

	protected void OnPauseMenuRequested()
	{
		HideOptionTooltip();
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	override protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu(false, true);
	}
}
