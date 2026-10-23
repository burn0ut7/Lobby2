//! Optional explicit pause-menu navigation through the production entry point.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPauseOpen()
	{
		if (GetManager() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE local Lobby2 player";
		ArmaReforgerScripted.OpenPauseMenu(false, true);
		return "DISPATCHED inspect pause menu";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPauseAction(string action)
	{
		if (GetManager() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE local Lobby2 player";
		if (action == "open")
			return GC_TestPauseOpen();
		if (action == "tools")
			return GC_TestPauseClick("PauseAdminTools");
		GC_AdminSignInDialog dialog = GC_AdminSignInDialog.Cast(SCR_ConfigurableDialogUi.GetCurrentDialog());
		if (action == "snapshot")
		{
			MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
			string result = string.Format("admin=%1 menu=%2", SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()), menu.GetRootWidget().GetName());
			if (dialog)
				result += "\n" + dialog.GC_TestSignInState();
			return result;
		}
		if (action == "close-tools")
			return GC_TestPauseClick("AdminClose");
		if (!dialog)
			return "UNAVAILABLE sign-in dialog";
		return dialog.GC_TestSignInAction(action);
	}
	//------------------------------------------------------------------------------------------------
	protected string GC_TestPauseClick(string name)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "UNAVAILABLE menu";
		Widget target = menu.GetRootWidget().FindAnyWidget(name);
		Widget ancestor = target;
		while (ancestor)
		{
			if (!ancestor.IsVisible() || !ancestor.IsEnabled())
				return "REJECTED hidden/disabled";
			ancestor = ancestor.GetParent();
		}
		if (!target)
			return "UNAVAILABLE widget";
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
		if (!button)
			return "UNAVAILABLE button";
		button.OnClick(target, 0, 0, 0);
		return "DISPATCHED " + name;
	}

}

modded class GC_AdminSignInDialog
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSignInState()
	{
		if (m_bIsClosing || !m_wRoot || !m_Editbox || !m_wTxtWarningMessage)
			return "signIn=closed";
		return string.Format("signIn=1 pending=%1 closing=%2 passwordLength=%3 warning=%4",
			m_bPending, m_bIsClosing, m_Editbox.GetValue().Length(), m_wTxtWarningMessage.GetText());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSignInAction(string action)
	{
		if (action == "wrong-password")
		{
			m_Editbox.SetValue("Lobby2DeliberatelyInvalidTestPassword");
			OnConfirm();
		}
		else if (action == "cancel")
			OnCancel();
		else
			return "REJECTED unsupported sign-in test";
		return GC_TestSignInState();
	}
}
