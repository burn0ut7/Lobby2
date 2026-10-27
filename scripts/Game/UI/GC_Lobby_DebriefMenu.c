//! This mission's personal report. Persistent history remains owned by the server outbox.
class GC_Lobby_DebriefMenu : ChimeraMenuBase
{
	protected GC_LobbyPlayerController m_Controller;
	protected SCR_ButtonTextComponent m_Back;
	protected ref map<SCR_ButtonBaseComponent, string> m_mSelections = new map<SCR_ButtonBaseComponent, string>();
	protected string m_sFaction;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		Widget root = GetRootWidget();
		GC_HeaderComponent header = GC_HeaderComponent.Cast(root.FindAnyWidget("MenuBar").FindHandler(GC_HeaderComponent));
		if (header)
			header.SetActiveState(4);
		m_Back = SCR_ButtonTextComponent.GetButtonText("DebriefBack", root);
		m_Back.SetText("Back (Esc)");
		m_Back.m_OnClicked.Insert(OnBackClicked);
		AddSelection("My stats (private)", string.Empty);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			foreach (Faction faction : manager.GetRegisteredFactions())
				AddSelection(WidgetManager.Translate(faction.GetFactionName()), faction.GetFactionKey());
		}
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		if (m_Controller)
		{
			TextWidget.Cast(root.FindAnyWidget("DebriefPlayer")).SetText("This mission · " + GetGame().GetPlayerManager().GetPlayerName(m_Controller.GetPlayerId()));
			m_Controller.m_GC_OnDebrief.Insert(OnSummary);
			m_Controller.GC_RequestDebrief();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSummary(string factionKey, string summary, string ammunition, bool complete)
	{
		if (factionKey != m_sFaction)
			return;
		TextWidget ammo = TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo"));
		ammo.SetText(ammunition);
		GetGame().GetCallqueue().CallLater(ResizeAmmo, 0);
		array<string> values = {};
		summary.Split("\n", values, false);
		TextWidget status = TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefStatus"));
		if (values.Count() != 9)
		{
			status.SetText(summary);
			return;
		}
		status.SetText(string.Empty);
		if (!complete)
			status.SetText("Partial report: some statistics could not be recorded.");
		for (int i = 0; i < values.Count(); i++)
			TextWidget.Cast(GetRootWidget().FindAnyWidget("StatValue" + i)).SetText(values[i]);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResizeAmmo()
	{
		// Resolve wrapping at the current panel width before measuring the scroll content.
		GetRootWidget().Update();
		float width;
		float height;
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo")).GetTextSize(width, height);
		SizeLayoutWidget.Cast(GetRootWidget().FindAnyWidget("AmmoSize")).SetHeightOverride(height + 16);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddSelection(string label, string factionKey)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets("{7C41F8A018B24C10}UI/layouts/Admin/Lobby_AdminToolButton.layout", GetRootWidget().FindAnyWidget("DebriefSelections"));
		row.SetName("DebriefSelection_" + factionKey);
		LayoutSlot.SetFillWeight(row, 1);
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
		button.SetText(label);
		button.SetToggled(factionKey == m_sFaction, false, false);
		button.m_OnClicked.Insert(OnSelection);
		m_mSelections.Set(button, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelection(SCR_ButtonBaseComponent selected)
	{
		if (!m_Controller || !m_mSelections.Contains(selected))
			return;
		m_sFaction = m_mSelections.Get(selected);
		foreach (SCR_ButtonBaseComponent button, string key : m_mSelections)
			button.SetToggled(key == m_sFaction, false, false);
		string title = "This mission · My stats (private)";
		string lastLabel = "Command";
		if (!m_sFaction.IsEmpty())
		{
			title = "This mission · " + m_sFaction + " totals";
			lastLabel = "Participants";
		}
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefPlayer")).SetText(title);
		TextWidget.Cast(GetRootWidget().FindAnyWidget("StatLabel8")).SetText(lastLabel);
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefStatus")).SetText("Loading mission statistics…");
		for (int i = 0; i < 9; i++)
			TextWidget.Cast(GetRootWidget().FindAnyWidget("StatValue" + i)).SetText("—");
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo")).SetText(string.Empty);
		m_Controller.GC_RequestDebrief(m_sFaction);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBackClicked(SCR_ButtonBaseComponent button)
	{
		OnBack();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (IsFocused())
			GetGame().GetCallqueue().CallLater(Close, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_Controller)
			m_Controller.m_GC_OnDebrief.Remove(OnSummary);
		if (m_Back)
			m_Back.m_OnClicked.Remove(OnBackClicked);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		GetGame().GetCallqueue().Remove(Close);
		GetGame().GetCallqueue().Remove(ResizeAmmo);
		foreach (SCR_ButtonBaseComponent button, string key : m_mSelections)
			button.m_OnClicked.Remove(OnSelection);
		m_mSelections.Clear();
		super.OnMenuClose();
	}
}
