class GC_SpectatorOptionButtonComponent : SCR_ButtonBaseComponent
{
	ref ScriptInvoker m_OnHoverChanged = new ScriptInvoker();

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_OnHoverChanged.Invoke(true);
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_OnHoverChanged.Invoke(false);
		return super.OnMouseLeave(w, enterW, x, y);
	}
}

//! The base manual camera intentionally accepts input only while no menu is
//! open or while the top menu is an EditorMenuUI. Using the camera-compatible
//! base here keeps Lobby2 scoped and avoids a global SCR_ManualCamera override.
class GC_Lobby_SpectatorMenu : EditorMenuUI
{
	protected static const float OPTION_TOOLTIP_GAP = 16;
	protected static const int OPTION_TOOLTIP_DELAY_MS = 1500;
	protected static const string INTERFACE_OPTION_DESCRIPTION = "Hide the spectator interface, chat, notifications, and all other game HUD elements. Press the shown key to restore them.";

	protected InputManager m_InputManager;
	protected GC_SpectatorOptionButtonComponent m_InterfaceOptionButton;
	protected Widget m_wInterfaceOptionSource;
	protected TextWidget m_wInterfaceOptionState;
	protected Widget m_wInterfaceOptionStateBackground;
	protected Widget m_wOptionTooltipLayer;
	protected Widget m_wOptionTooltip;
	protected TextWidget m_wOptionTooltipText;
	protected bool m_bInterfaceOptionHovered;

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
			m_InterfaceOptionButton = GC_SpectatorOptionButtonComponent.Cast(optionButton.FindHandler(GC_SpectatorOptionButtonComponent));
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
			m_InterfaceOptionButton.m_OnHoverChanged.Insert(OnInterfaceOptionHoverChanged);
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
			m_InterfaceOptionButton.m_OnHoverChanged.Remove(OnInterfaceOptionHoverChanged);
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

	protected void OnInterfaceOptionHoverChanged(bool hovered)
	{
		m_bInterfaceOptionHovered = hovered;
		GetGame().GetCallqueue().Remove(ShowOptionTooltip);
		if (hovered)
			GetGame().GetCallqueue().CallLater(ShowOptionTooltip, OPTION_TOOLTIP_DELAY_MS);
		else
			HideOptionTooltip();
	}

	protected void ShowOptionTooltip()
	{
		if (!m_bInterfaceOptionHovered)
			return;

		if (!m_wInterfaceOptionSource || !m_wOptionTooltipLayer || !m_wOptionTooltip || !m_wOptionTooltipText)
			return;

		m_wOptionTooltipText.SetText(INTERFACE_OPTION_DESCRIPTION);
		m_wOptionTooltip.SetVisible(true);
		GetGame().GetCallqueue().Remove(PositionOptionTooltip);
		GetGame().GetCallqueue().CallLater(PositionOptionTooltip, 0);
	}

	protected void PositionOptionTooltip()
	{
		if (!m_bInterfaceOptionHovered || !m_wInterfaceOptionSource || !m_wOptionTooltipLayer || !m_wOptionTooltip)
			return;

		float layerX;
		float layerY;
		float layerWidth;
		float layerHeight;
		float tooltipWidth;
		float tooltipHeight;
		m_wOptionTooltipLayer.GetScreenPos(layerX, layerY);
		m_wOptionTooltipLayer.GetScreenSize(layerWidth, layerHeight);
		m_wOptionTooltip.GetScreenSize(tooltipWidth, tooltipHeight);
		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float localX = workspace.DPIUnscale(mouseX - layerX) + OPTION_TOOLTIP_GAP;
		float localY = workspace.DPIUnscale(mouseY - layerY) + OPTION_TOOLTIP_GAP;
		float localTooltipWidth = workspace.DPIUnscale(tooltipWidth);
		float localTooltipHeight = workspace.DPIUnscale(tooltipHeight);
		float availableWidth = workspace.DPIUnscale(layerWidth);
		float availableHeight = workspace.DPIUnscale(layerHeight);
		if (localX + localTooltipWidth > availableWidth)
			localX = workspace.DPIUnscale(mouseX - layerX) - localTooltipWidth - OPTION_TOOLTIP_GAP;
		if (localY + localTooltipHeight > availableHeight)
			localY = workspace.DPIUnscale(mouseY - layerY) - localTooltipHeight - OPTION_TOOLTIP_GAP;
		if (localX + localTooltipWidth > availableWidth)
			localX = availableWidth - localTooltipWidth;
		if (localX < 0)
			localX = 0;
		if (localY + localTooltipHeight > availableHeight)
			localY = availableHeight - localTooltipHeight;
		if (localY < 0)
			localY = 0;

		FrameSlot.SetPos(m_wOptionTooltip, localX, localY);
	}

	protected void HideOptionTooltip()
	{
		GetGame().GetCallqueue().Remove(ShowOptionTooltip);
		GetGame().GetCallqueue().Remove(PositionOptionTooltip);
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
