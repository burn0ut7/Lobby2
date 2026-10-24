class GC_AdminMapPlansComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_LobbyManager m_Manager;
	protected EditBoxWidget m_Name;
	protected TextWidget m_Status;
	protected int m_iPending = -1;
	protected string m_sConfirmation;
	protected string m_sConfirmedName;
	protected bool m_bWasActive;
	protected bool m_bCheckingRecovery;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Manager = GC_LobbyManager.GetManager();
		m_Name = EditBoxWidget.Cast(w.FindAnyWidget("PlanName"));
		m_Name.SetText(string.Empty);
		m_Status = TextWidget.Cast(w.FindAnyWidget("PlanStatus"));
		array<string> buttons = {"PlanSave", "PlanLoad", "PlanClear", "PlanCancel"};
		foreach (string name : buttons)
			SCR_ButtonTextComponent.GetButtonText(name, w).m_OnClicked.Insert(OnAction);
		if (m_Controller)
			m_Controller.m_GC_OnMapPlanResult.Insert(OnResult);
		if (m_Manager)
		{
			m_Manager.m_OnLobbyPhaseChanged.Insert(OnPhaseChanged);
			m_Manager.m_OnPlayerRoleChanged.Insert(OnRoleChanged);
		}
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		bool justOpened = active && !m_bWasActive;
		m_bWasActive = active;
		m_wRoot.SetVisible(active);
		if (!active)
			m_sConfirmation = string.Empty;
		Refresh();
		if (justOpened && m_Controller && m_iPending < 0)
		{
			m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
			m_bCheckingRecovery = true;
			m_Status.SetText(string.Empty);
			Refresh();
			m_Controller.GC_RequestMapPlan("status", "", m_iPending);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		bool access = m_Controller && GC_LobbyGameMode.GetGameMode() && SCR_Global.IsAdmin(m_Controller.GetPlayerId());
		bool idle = access && m_iPending < 0;
		bool canChange = idle && !GC_LobbyPhase.ArePlayerMapMarkersLocked();
		if (!idle || (!canChange && m_sConfirmation != "PlanSave"))
			m_sConfirmation = string.Empty;
		m_Name.SetEnabled(idle);
		SCR_ButtonTextComponent save = SCR_ButtonTextComponent.GetButtonText("PlanSave", m_wRoot);
		SCR_ButtonTextComponent load = SCR_ButtonTextComponent.GetButtonText("PlanLoad", m_wRoot);
		SCR_ButtonTextComponent clear = SCR_ButtonTextComponent.GetButtonText("PlanClear", m_wRoot);
		save.SetEnabled(idle);
		load.SetEnabled(canChange);
		clear.SetEnabled(canChange);
		save.SetText("Save");
		load.SetText("Load");
		clear.SetText("Clear map");
		if (m_sConfirmation == "PlanSave")
			save.SetText("Confirm overwrite");
		else if (m_sConfirmation == "PlanLoad")
			load.SetText("Confirm load");
		else if (m_sConfirmation == "PlanClear")
			clear.SetText("Confirm clear");
		SCR_ButtonTextComponent.GetButtonText("PlanCancel", m_wRoot).SetText("Cancel");
		m_wRoot.FindAnyWidget("PlanCancel").SetVisible(!m_sConfirmation.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!m_Controller || !GC_LobbyGameMode.GetGameMode() || !SCR_Global.IsAdmin(m_Controller.GetPlayerId())
			|| m_iPending >= 0 || !button.GetRootWidget().IsEnabled())
			return;
		string action = button.GetRootWidget().GetName();
		if (action == "PlanCancel")
		{
			m_sConfirmation = string.Empty;
			m_Status.SetText("Cancelled.");
			Refresh();
			return;
		}
		string name = m_Name.GetText();
		string path = "$profile:Lobby2MapPlans/" + name + ".json";
		if (action != "PlanClear")
		{
			string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_";
			bool valid = name.Length() > 0 && name.Length() <= 64;
			for (int index; valid && index < name.Length(); index++)
				valid = allowed.Contains(name.Substring(index, 1));
			if (!valid)
			{
				m_Status.SetText("Use a plan name of 1-64 letters, numbers, spaces, hyphens or underscores.");
				return;
			}
		}
		bool needsConfirmation = action != "PlanSave" || FileIO.FileExists(path);
		if (needsConfirmation && (m_sConfirmation != action || m_sConfirmedName != name))
		{
			m_sConfirmation = action;
			m_sConfirmedName = name;
			if (action == "PlanSave")
				m_Status.SetText("Replace the saved file '" + name + "'?");
			else if (action == "PlanLoad")
				m_Status.SetText("Replace all shared markers and drawings with '" + name + "'?");
			else
				m_Status.SetText("Clear shared markers and drawings for all factions? Recovery will also be cleared.");
			Refresh();
			return;
		}
		m_sConfirmation = string.Empty;
		string input;
		string request;
		if (action == "PlanLoad")
		{
			if (!GC_MapPlanData.ReadFile(path, input))
			{
				m_Status.SetText("Cannot read plan: " + path);
				Refresh();
				return;
			}
			request = "load";
		}
		else if (action == "PlanSave")
		{
			request = "save";
			input = path;
		}
		else
			request = "clear";
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		m_Status.SetText("Waiting for server...");
		Refresh();
		m_Controller.GC_RequestMapPlan(request, input, m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome, string output)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		if (m_bCheckingRecovery && outcome == "Recovery saved.")
			outcome = string.Empty;
		m_bCheckingRecovery = false;
		m_Status.SetText(outcome);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPhaseChanged(SCR_EGameModeState phase)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRoleChanged(int playerId)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_Controller)
			m_Controller.m_GC_OnMapPlanResult.Remove(OnResult);
		if (m_Manager)
		{
			m_Manager.m_OnLobbyPhaseChanged.Remove(OnPhaseChanged);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnRoleChanged);
		}
		array<string> buttons = {"PlanSave", "PlanLoad", "PlanClear", "PlanCancel"};
		foreach (string name : buttons)
		{
			Widget buttonRoot = w.FindAnyWidget(name);
			if (!buttonRoot)
				continue;
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(buttonRoot.FindHandler(SCR_ButtonTextComponent));
			if (button)
				button.m_OnClicked.Remove(OnAction);
		}
		super.HandlerDeattached(w);
	}
}
