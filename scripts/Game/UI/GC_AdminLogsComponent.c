//! Current mission only. Page contents are private to the authenticated requesting controller.
class GC_AdminLogsComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_AdminSearchFilter m_Players;
	protected GC_AdminSearchFilter m_Type;
	protected int m_iRosterRequest;
	protected TextWidget m_Status;
	protected TextWidget m_Page;
	protected bool m_bPageLoading = true;
	protected bool m_bActive;
	protected bool m_bLive = true;
	protected int m_iRequest;
	protected int m_iBefore;
	protected int m_iNext;
	protected float m_fRequestedAt = -1000;
	protected ref array<int> m_aBack = {};
	protected ref array<string> m_aDisplayed = {};
	protected ref map<SCR_ButtonBaseComponent, int> m_mTableRows = new map<SCR_ButtonBaseComponent, int>();
	protected string m_sSelectedEvent;
	protected ref array<string> m_aTypes = {"", "shot", "grenade", "damage", "kill", "teamKill", "death", "admin", "joined", "left", "session", "aiKill", "slot"};

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Players = GC_AdminSearchFilter.Cast(w.FindAnyWidget("LogPlayers").FindHandler(GC_AdminSearchFilter));
		m_Type = GC_AdminSearchFilter.Cast(w.FindAnyWidget("LogType").FindHandler(GC_AdminSearchFilter));
		EditBoxWidget.Cast(m_Players.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("All players");
		EditBoxWidget.Cast(m_Type.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("All events");
		array<string> labels = {"All events", "Shots", "Grenades", "Damage", "Player kills", "Team kills", "Deaths (all)", "Admin", "Joined", "Left", "Session", "AI kills", "Slotting"};
		m_Type.SetChoices(labels, m_aTypes, m_aTypes);
		m_Players.SetChoices({"All players"}, {""}, {""});
		m_Players.m_OnFilterChanged.Insert(FiltersChanged);
		m_Type.m_OnFilterChanged.Insert(FiltersChanged);
		m_Players.m_OnOpened.Insert(OnOpenFilter);
		m_Type.m_OnOpened.Insert(OnOpenFilter);
		m_Status = TextWidget.Cast(w.FindAnyWidget("LogStatus"));
		Bind("LogLive", "Live");
		m_Page = TextWidget.Cast(w.FindAnyWidget("LogPage"));
		Bind("LogNewer", "Previous");
		Bind("LogOlder", "Next");
		BeginPageLoad();
		if (m_Controller)
		{
			m_Controller.m_OnSessionLogRows.Insert(OnRows);
			m_Controller.m_OnModerationRows.Insert(OnPlayers);
			m_Controller.m_OnModerationChanged.Insert(RequestPlayers);
		}
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
			RequestPlayers();
			Latest();
		}
		else
		{
			m_iRequest = -1;
			m_Players.CloseList();
			m_Type.CloseList();
			ClearRows();
		}
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
		m_mTableRows.Clear();
		m_wRoot.FindAnyWidget("LogEmpty").SetVisible(false);
		TextWidget.Cast(m_wRoot.FindAnyWidget("LogSelectionDetail")).SetText("");
		Widget root = m_wRoot.FindAnyWidget("LogRows");
		while (root.GetChildren())
			root.GetChildren().RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!HasAccess())
		{
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
		BeginPageLoad();
		m_iBefore = 0;
		m_aBack.Clear();
		Load();
	}

	//------------------------------------------------------------------------------------------------
	protected void BeginPageLoad()
	{
		m_bPageLoading = true;
		m_iRequest = -1;
		m_Status.SetText("Loading...");
		m_wRoot.FindAnyWidget("LogOlder").SetEnabled(false);
		m_wRoot.FindAnyWidget("LogNewer").SetEnabled(false);
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
		string type = m_Type.GetFilterValue();
		if (!m_Type.HasChoice() && !type.IsEmpty())
			type = "~" + type;
		m_iRequest = m_Controller.ReserveAdminActionRequest();
		m_fRequestedAt = now;
		m_Controller.RequestSessionLog(m_iRequest, m_Players.GetFilterValue(), type, m_iBefore);
		m_wRoot.FindAnyWidget("LogLive").FindAnyWidget("Tick").SetVisible(m_bLive);
	}

	//------------------------------------------------------------------------------------------------
	protected void FiltersChanged()
	{
		BeginPageLoad();
		GetGame().GetCallqueue().Remove(Latest);
		GetGame().GetCallqueue().CallLater(Latest, 350);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenFilter(SCR_ComboBoxComponent opened)
	{
		if (opened == m_Type)
			m_Players.CloseList();
		else
			m_Type.CloseList();
		SCR_MenuHelper.SetActiveWidgetInteractionState(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestPlayers()
	{
		if (!m_bActive || !HasAccess())
			return;
		m_iRosterRequest = m_Controller.ReserveAdminActionRequest();
		m_Controller.RequestModerationSession(m_iRosterRequest);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayers(array<string> rows, int requestId)
	{
		if (!m_bActive || !HasAccess() || requestId != m_iRosterRequest || rows.Count() % 7 != 0)
			return;
		array<string> labels = {"All players"};
		array<string> values = {""};
		array<string> terms = {""};
		for (int i; i < rows.Count(); i += 7)
		{
			labels.Insert(rows[i + 1]);
			values.Insert(rows[i + 2]);
			terms.Insert(rows[i + 2] + " " + rows[i + 3]);
		}
		m_Players.SetChoices(labels, values, terms);
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
			if (m_bLive)
			{
				Latest();
			}
			m_wRoot.FindAnyWidget("LogLive").FindAnyWidget("Tick").SetVisible(m_bLive);
			return;
		}
		if (m_bPageLoading)
			return;
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
		else
			return;
		m_bLive = false;
		m_wRoot.FindAnyWidget("LogLive").FindAnyWidget("Tick").SetVisible(false);
		BeginPageLoad();
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
		m_bPageLoading = false;
		m_Page.SetText(string.Format("Page %1", m_aBack.Count() + 1));
		m_iNext = next;
		m_Status.SetText(status);
		m_wRoot.FindAnyWidget("LogOlder").SetEnabled(next > 0);
		m_wRoot.FindAnyWidget("LogNewer").SetEnabled(!m_aBack.IsEmpty());
		m_wRoot.FindAnyWidget("LogEmpty").SetVisible(rows.IsEmpty());
		bool unchanged = rows.Count() == m_aDisplayed.Count();
		for (int compare; unchanged && compare < rows.Count(); compare++)
			unchanged = rows[compare] == m_aDisplayed[compare];
		if (unchanged)
			return;
		ClearRows();
		m_aDisplayed.Copy(rows);
		m_wRoot.FindAnyWidget("LogEmpty").SetVisible(rows.IsEmpty());
		Widget root = m_wRoot.FindAnyWidget("LogRows");
		array<string> fields = {"Cell1", "Cell2", "Cell3", "Cell4"};
		for (int i; i < rows.Count(); i += 5)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets("{A81F9D3047B6E2C5}UI/layouts/Admin/Lobby_AdminLogRow.layout", root);
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Insert(OnTableRow);
			m_mTableRows.Set(button, i);
			for (int field; field < fields.Count(); field++)
				TextWidget.Cast(row.FindAnyWidget(fields[field])).SetText(rows[i + field + 1]);
			if (rows[i + 2] == "teamKill")
			{
				TextWidget eventLabel = TextWidget.Cast(row.FindAnyWidget("Cell2"));
				eventLabel.SetText("Team kill");
			}
			else if (rows[i + 2] == "aiKill")
				TextWidget.Cast(row.FindAnyWidget("Cell2")).SetText("AI kill");
			else if (rows[i + 2] == "slot")
				TextWidget.Cast(row.FindAnyWidget("Cell2")).SetText("Slotting");
		}
		RefreshSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTableRow(SCR_ButtonBaseComponent button)
	{
		int index;
		if (!HasAccess() || !m_mTableRows.Find(button, index))
			return;
		m_sSelectedEvent = m_aDisplayed[index];
		RefreshSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshSelection()
	{
		string detail;
		foreach (SCR_ButtonBaseComponent button, int index : m_mTableRows)
		{
			bool selected = m_aDisplayed[index] == m_sSelectedEvent;
			button.SetToggled(selected, false, false);
			if (selected)
				detail = m_aDisplayed[index + 1] + " | " + m_aDisplayed[index + 2] + "\n" + m_aDisplayed[index + 3] + "\n" + m_aDisplayed[index + 4];
		}
		TextWidget.Cast(m_wRoot.FindAnyWidget("LogSelectionDetail")).SetText(detail);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(Latest);
		GetGame().GetCallqueue().Remove(Load);
		if (m_Controller)
		{
			m_Controller.m_OnSessionLogRows.Remove(OnRows);
			m_Controller.m_OnModerationRows.Remove(OnPlayers);
			m_Controller.m_OnModerationChanged.Remove(RequestPlayers);
		}
		// Child handlers may already be destroyed when the parent detaches.
		if (m_Players)
		{
			m_Players.m_OnFilterChanged.Remove(FiltersChanged);
			m_Players.m_OnOpened.Remove(OnOpenFilter);
			m_Players.CloseList();
		}
		if (m_Type)
		{
			m_Type.m_OnFilterChanged.Remove(FiltersChanged);
			m_Type.m_OnOpened.Remove(OnOpenFilter);
			m_Type.CloseList();
		}
		super.HandlerDeattached(w);
	}
}
