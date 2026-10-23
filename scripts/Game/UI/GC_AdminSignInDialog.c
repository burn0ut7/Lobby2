//! Uses the game's #login command. This dialog never grants an admin role.
class GC_AdminSignInDialog : SCR_EditboxDialogUi
{
	ref ScriptInvoker m_OnAuthenticated = new ScriptInvoker();
	protected GC_LobbyManager m_Manager;
	protected bool m_bPending;
	protected bool m_bAuthenticated;

	//------------------------------------------------------------------------------------------------
	override protected void OnMenuOpen(SCR_ConfigurableDialogUiPreset preset)
	{
		super.OnMenuOpen(preset);
		SetTitle("Sign in");
		SetMessage("Admin password");
		SetWarningMessage("");
		FindButton("confirm").SetLabel("Sign in");
		m_Manager = GC_LobbyManager.GetManager();
		if (m_Manager)
			m_Manager.m_OnPlayerRoleChanged.Insert(OnRoleChanged);
		m_OnCloseStart.Insert(OnClosing);
		m_OnClose.Insert(OnClosed);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnConfirm()
	{
		if (m_bPending || m_bIsClosing || !m_Manager || m_Manager != GC_LobbyManager.GetManager())
			return;

		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return;
		if (SCR_Global.IsAdmin(controller.GetPlayerId()))
		{
			CompleteSignIn();
			return;
		}

		SCR_ChatComponent chat = SCR_ChatComponent.Cast(controller.FindComponent(SCR_ChatComponent));
		ScriptedChatEntity chatEntity = ScriptedChatEntity.Cast(GetGame().GetChat());
		if (!chat || !chatEntity)
		{
			SetWarningMessage("Sign-in is unavailable. Rejoin the server and try again.");
			return;
		}

		string password = m_Editbox.GetValue();
		if (password.Contains("\n") || password.Contains("\r"))
		{
			SetWarningMessage("Enter the admin password on one line.");
			return;
		}

		m_bPending = true;
		FindButton("confirm").SetEnabled(false);
		SetWarningMessage("");
		SetMessage("Signing in...");
		m_Editbox.SetValue("");
		// Same native command path as the standard chat, without chat UI history.
		chat.SendMessage("#login " + password, chatEntity.GetChannelId(chatEntity.GetDefaultChannel()));
		password = string.Empty;
		GetGame().GetCallqueue().CallLater(OnSignInTimeout, 10000);
		OnRoleChanged(controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRoleChanged(int playerId)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!m_bPending || m_bIsClosing || !controller || controller.GetPlayerId() != playerId)
			return;
		if (m_Manager == GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(playerId))
			CompleteSignIn();
	}

	//------------------------------------------------------------------------------------------------
	protected void CompleteSignIn()
	{
		m_bAuthenticated = true;
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSignInTimeout()
	{
		if (m_bIsClosing)
			return;
		m_bPending = false;
		SetMessage("Admin password");
		FindButton("confirm").SetEnabled(true);
		SetWarningMessage("Sign-in was not confirmed. Check your password and try again.");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnClosing()
	{
		GetGame().GetCallqueue().Remove(OnSignInTimeout);
		m_bPending = false;
		if (m_Editbox)
			m_Editbox.SetValue("");
		if (m_Manager)
			m_Manager.m_OnPlayerRoleChanged.Remove(OnRoleChanged);
		m_Manager = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnClosed()
	{
		if (m_bAuthenticated)
			m_OnAuthenticated.Invoke();
	}
}
