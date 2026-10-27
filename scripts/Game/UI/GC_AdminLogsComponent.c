//! Current mission only. Page contents are private to the authenticated requesting controller.
class GC_AdminLogsComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_AdminPlayerPicker m_Players;
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected SCR_ComboBoxComponent m_Type;
	protected TextWidget m_Status;
	protected bool m_bActive;
	protected bool m_bLive = true;
	protected bool m_bUpdatingFilter;
	protected int m_iRequest;
	protected int m_iBefore;
	protected int m_iNext;
	protected float m_fRequestedAt = -1000;
	protected ref array<int> m_aBack = {};
	protected ref array<string> m_aDisplayed = {};
	protected ref array<string> m_aTypes = {"", "shot", "grenade", "damage", "kill", "teamKill", "death", "admin", "joined", "left", "session", "aiKill", "slot"};

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Players = GC_AdminPlayerPicker.Cast(w.FindAnyWidget("LogPlayers").FindHandler(GC_AdminPlayerPicker));
		m_Players.SetAllowAll(true);
		m_Players.m_OnChanged.Insert(OnPlayerChanged);
		m_Search = EditBoxWidget.Cast(w.FindAnyWidget("LogSearch"));
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearch);
		m_Type = SCR_ComboBoxComponent.Cast(w.FindAnyWidget("LogType").FindHandler(SCR_ComboBoxComponent));
		array<string> labels = {"All events", "Shots", "Grenades", "Damage", "Player kills", "Team kills", "Deaths (all)", "Admin", "Joined", "Left", "Session", "AI kills", "Slotting"};
		foreach (string label : labels)
			m_Type.AddItem(label);
		m_Type.SetCurrentItem(0, false, false, false);
		m_Type.m_OnChanged.Insert(OnType);
		SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("ComboButton").FindHandler(SCR_ButtonBaseComponent)).m_OnClicked.Insert(OnOpenType);
		m_Status = TextWidget.Cast(w.FindAnyWidget("LogStatus"));
		Bind("LogLive", "Live");
		Bind("LogRefresh", "Refresh");
		Bind("LogNewer", "Newer");
		Bind("LogOlder", "Older");
		if (m_Controller)
			m_Controller.m_OnSessionLogRows.Insert(OnRows);
		GetGame().GetCallqueue().CallLater(Poll, 2000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
		button.SetText(label);
		button.m_OnClicked.Insert(OnAction);
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		m_wRoot.SetVisible(active);
		if (m_bActive == active)
			return;
		m_bActive = active;
		if (active)
		{
			if (m_Controller)
				m_Controller.GC_RequestAdminViewOpen(true);
			Latest();
		}
		else
		{
			m_Players.CloseRoster();
			m_iRequest = -1;
			ClearRows();
		}
	}

	//------------------------------------------------------------------------------------------------
	void ClosePlayerList()
	{
		m_Players.CloseRoster();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Controller && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		m_aDisplayed.Clear();
		Widget root = m_wRoot.FindAnyWidget("LogRows");
		while (root.GetChildren())
			root.GetChildren().RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!HasAccess())
		{
			m_Players.CloseRoster();
			ClearRows();
			m_Status.SetText("Admin access required.");
			return;
		}
		if (m_bActive && m_bLive)
			Load();
	}

	//------------------------------------------------------------------------------------------------
	protected void Latest()
	{
		m_iBefore = 0;
		m_aBack.Clear();
		Load();
	}

	//------------------------------------------------------------------------------------------------
	protected void Load()
	{
		if (!m_bActive || !HasAccess())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fRequestedAt < 500)
		{
			GetGame().GetCallqueue().Remove(Load);
			GetGame().GetCallqueue().CallLater(Load, 500);
			return;
		}
		int selection = m_Type.GetCurrentIndex();
		if (selection < 0 || selection >= m_aTypes.Count())
			return;
		m_iRequest = m_Controller.ReserveAdminActionRequest();
		m_fRequestedAt = now;
		array<int> players;
		array<string> identities;
		string playerFilter = m_Search.GetText();
		if (m_Players.GetSelection(players, identities))
			playerFilter = identities[0];
		m_Controller.RequestSessionLog(m_iRequest, playerFilter, m_aTypes[selection], m_iBefore);
		SCR_ButtonTextComponent.GetButtonText("LogLive", m_wRoot).SetToggled(m_bLive, false, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerChanged()
	{
		if (m_bUpdatingFilter)
			return;
		m_bUpdatingFilter = true;
		m_Search.SetText("");
		m_bUpdatingFilter = false;
		FiltersChanged();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearch(Widget widget)
	{
		if (m_bUpdatingFilter)
			return;
		m_bUpdatingFilter = true;
		m_Players.SelectPlayer(0);
		m_bUpdatingFilter = false;
		FiltersChanged();
	}

	//------------------------------------------------------------------------------------------------
	protected void FiltersChanged()
	{
		m_iRequest = -1;
		m_Status.SetText("Loading...");
		GetGame().GetCallqueue().Remove(Latest);
		GetGame().GetCallqueue().CallLater(Latest, 350);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnType(SCR_SelectionWidgetComponent combo, int index)
	{
		FiltersChanged();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenType(SCR_ButtonBaseComponent button)
	{
		if (m_bActive && HasAccess())
		{
			m_Players.CloseRoster();
			m_Type.OpenList();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess())
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "LogLive")
		{
			m_bLive = !m_bLive;
			SCR_ButtonTextComponent.GetButtonText("LogLive", m_wRoot).SetText("Paused");
			if (m_bLive)
			{
				SCR_ButtonTextComponent.GetButtonText("LogLive", m_wRoot).SetText("Live");
				Latest();
			}
			SCR_ButtonTextComponent.GetButtonText("LogLive", m_wRoot).SetToggled(m_bLive, false, false);
			return;
		}
		if (name == "LogRefresh")
		{
			Latest();
			return;
		}
		m_bLive = false;
		SCR_ButtonTextComponent.GetButtonText("LogLive", m_wRoot).SetText("Paused");
		if (name == "LogOlder" && m_iNext > 0)
		{
			m_aBack.Insert(m_iBefore);
			m_iBefore = m_iNext;
		}
		else if (name == "LogNewer" && !m_aBack.IsEmpty())
		{
			m_iBefore = m_aBack[m_aBack.Count() - 1];
			m_aBack.Remove(m_aBack.Count() - 1);
		}
		Load();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRows(int requestId, array<string> rows, int next, string status)
	{
		if (!m_bActive || !HasAccess() || requestId != m_iRequest || rows.Count() % 5 != 0)
			return;
		if (next < 0)
		{
			m_Status.SetText(status);
			return;
		}
		m_iNext = next;
		m_Status.SetText(status);
		m_wRoot.FindAnyWidget("LogOlder").SetEnabled(next > 0);
		m_wRoot.FindAnyWidget("LogNewer").SetEnabled(!m_aBack.IsEmpty());
		bool unchanged = rows.Count() == m_aDisplayed.Count();
		for (int compare; unchanged && compare < rows.Count(); compare++)
			unchanged = rows[compare] == m_aDisplayed[compare];
		if (unchanged)
			return;
		ClearRows();
		m_aDisplayed.Copy(rows);
		Widget root = m_wRoot.FindAnyWidget("LogRows");
		array<string> fields = {"LogTime", "LogEvent", "LogPeople", "LogDetail"};
		for (int i; i < rows.Count(); i += 5)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets("{A81F9D3047B6E2C5}UI/layouts/Admin/Lobby_AdminLogRow.layout", root);
			for (int field; field < fields.Count(); field++)
				TextWidget.Cast(row.FindAnyWidget(fields[field])).SetText(rows[i + field + 1]);
			if (rows[i + 2] == "teamKill")
			{
				TextWidget eventLabel = TextWidget.Cast(row.FindAnyWidget("LogEvent"));
				eventLabel.SetText("Team kill");
				eventLabel.SetColor(Color.FromRGBA(255, 90, 80, 255));
			}
			else if (rows[i + 2] == "aiKill")
				TextWidget.Cast(row.FindAnyWidget("LogEvent")).SetText("AI kill");
			else if (rows[i + 2] == "slot")
				TextWidget.Cast(row.FindAnyWidget("LogEvent")).SetText("Slotting");
		}
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(Latest);
		GetGame().GetCallqueue().Remove(Load);
		if (m_Controller)
			m_Controller.m_OnSessionLogRows.Remove(OnRows);
		if (m_Players)
			m_Players.m_OnChanged.Remove(OnPlayerChanged);
		if (m_SearchEvents)
			m_SearchEvents.GetOnChange().Remove(OnSearch);
		if (m_Type)
			m_Type.m_OnChanged.Remove(OnType);
		super.HandlerDeattached(w);
	}
}
