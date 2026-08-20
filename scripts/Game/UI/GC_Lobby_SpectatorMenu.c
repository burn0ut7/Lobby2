//! The base manual camera intentionally accepts input only while no menu is
//! open or while the top menu is an EditorMenuUI. Using the camera-compatible
//! base here keeps Lobby2 scoped and avoids a global SCR_ManualCamera override.
class GC_Lobby_SpectatorMenu : EditorMenuUI
{
	protected InputManager m_InputManager;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		WidgetManager.SetCursor(0);

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
	}

	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		WidgetManager.SetCursor(0);
	}

	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		if (m_InputManager)
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);

		m_InputManager = null;
		super.OnMenuClose();
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
