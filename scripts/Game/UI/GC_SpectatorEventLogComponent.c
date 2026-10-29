//! Fixed personal history and a live spectator combat feed.
class GC_SpectatorEventLogComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected bool m_bActive;
	protected bool m_bEveryone;
	protected bool m_bPending;
	protected bool m_bPersonalLoaded;
	protected int m_iRequest = -1;
	protected int m_iBefore;
	protected int m_iNext;
	protected int m_iPersonalNext;
	protected float m_fRequestedAt;
	protected ref array<string> m_aPersonal = {};
	protected ref array<string> m_aRows = {};
	protected ref map<SCR_ButtonBaseComponent, int> m_mRows = new map<SCR_ButtonBaseComponent, int>();
	ref ScriptInvoker m_OnClose = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		Bind("EventYou", "You");
		Bind("EventEveryone", "Everyone");
		Bind("EventClose", "Close");
		if (m_Controller)
			m_Controller.m_OnGC_SpectatorEvents.Insert(OnRows);
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
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(Load);
		m_iRequest = -1;
		m_bPending = false;
		if (active)
		{
			ShowTab();
			GetGame().GetCallqueue().CallLater(Poll, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Controller && GC_LobbyGameMode.GetGameMode() && m_Controller.GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR;
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowTab()
	{
		m_iRequest = -1;
		m_bPending = false;
		m_iBefore = 0;
		ClearRows();
		ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll")).SetSliderPos(0, 0);
		SCR_ButtonTextComponent.GetButtonText("EventYou", m_wRoot).SetToggled(!m_bEveryone, false, false);
		SCR_ButtonTextComponent.GetButtonText("EventEveryone", m_wRoot).SetToggled(m_bEveryone, false, false);
		SetStatus("");
		GetGame().GetCallqueue().Remove(Load);
		if (!m_bEveryone && m_bPersonalLoaded)
		{
			m_iNext = m_iPersonalNext;
			m_aRows.Copy(m_aPersonal);
			AppendRows(0);
			if (m_aRows.IsEmpty())
				SetStatus("No events");
			return;
		}
		SetStatus("Loading...");
		GetGame().GetCallqueue().CallLater(Load, 300);
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!HasAccess())
		{
			SetActive(false);
			m_aPersonal.Clear();
			m_bPersonalLoaded = false;
			ClearRows();
			return;
		}
		if (m_bPending)
		{
			if (GetGame().GetWorld().GetWorldTime() - m_fRequestedAt > 5000)
			{
				SetStatus("Connection delayed");
				m_bPending = false;
				Load();
			}
			return;
		}
		if (m_bEveryone)
		{
			Load();
			return;
		}
		if (m_iNext <= 0)
			return;
		float x;
		float y;
		ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll")).GetSliderPos(x, y);
		if (y >= 0.9)
		{
			m_iBefore = m_iNext;
			Load();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Load()
	{
		if (!m_bActive || !HasAccess() || m_bPending)
			return;
		m_bPending = true;
		m_fRequestedAt = GetGame().GetWorld().GetWorldTime();
		m_iRequest = m_Controller.GC_ReserveSpectatorEventRequest();
		m_Controller.GC_RequestSpectatorEvents(m_iRequest, m_bEveryone, m_iBefore);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!m_bActive || !HasAccess())
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "EventClose")
		{
			m_OnClose.Invoke();
			return;
		}
		bool everyone = name == "EventEveryone";
		if (m_bEveryone == everyone)
		{
			button.SetToggled(true, false, false);
			return;
		}
		m_bEveryone = everyone;
		ShowTab();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRows(int request, array<string> rows, int next, string status)
	{
		if (!m_bActive || !HasAccess() || request != m_iRequest || rows.Count() % 7 != 0)
			return;
		m_bPending = false;
		m_iNext = next;
		SetStatus(status);
		bool unchanged = rows.Count() == m_aRows.Count();
		for (int compare; unchanged && compare < rows.Count(); compare++)
			unchanged = rows[compare] == m_aRows[compare];
		if (m_bEveryone && unchanged)
			return;
		int first;
		if (m_bEveryone || m_iBefore == 0)
			ClearRows();
		else
			first = m_aRows.Count();
		float scrollX;
		float scrollY;
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll"));
		scroll.GetSliderPosPixels(scrollX, scrollY);
		foreach (string value : rows)
			m_aRows.Insert(value);
		AppendRows(first);
		if (!m_bEveryone)
		{
			m_aPersonal.Copy(m_aRows);
			m_iPersonalNext = next;
			m_bPersonalLoaded = true;
			scroll.SetSliderPosPixels(scrollX, scrollY);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRows(int first)
	{
		Widget parent = m_wRoot.FindAnyWidget("EventRows");
		for (int i = first; i < m_aRows.Count(); i += 7)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets("{D378A10029000002}UI/layouts/Spectator/Lobby_Spectator_Event_Row.layout", parent);
			row.SetName("SpectatorEvent_" + m_aRows[i]);
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Insert(OnRow);
			m_mRows.Set(button, i);
			TextWidget.Cast(row.FindAnyWidget("EventSummary")).SetText(m_aRows[i + 1] + "   " + m_aRows[i + 2] + "   " + m_aRows[i + 3]);
			TextWidget.Cast(row.FindAnyWidget("EventInfo")).SetText(m_aRows[i + 4]);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRow(SCR_ButtonBaseComponent button)
	{
		int index;
		if (!m_bActive || !HasAccess() || !m_mRows.Find(button, index))
			return;
		if (m_aRows[index + 6].IsEmpty())
		{
			SetStatus("Location unavailable");
			return;
		}
		GC_SpectatorManualCamera camera = GC_SpectatorManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera || !camera.TeleportToWorldPosition(m_aRows[index + 6].ToVector(), true))
			SetStatus("Camera unavailable");
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text)
	{
		TextWidget status = TextWidget.Cast(m_wRoot.FindAnyWidget("EventStatus"));
		status.SetText(text);
		status.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		foreach (SCR_ButtonBaseComponent button, int index : m_mRows)
			button.m_OnClicked.Remove(OnRow);
		m_mRows.Clear();
		m_aRows.Clear();
		Widget parent = m_wRoot.FindAnyWidget("EventRows");
		while (parent.GetChildren())
			parent.GetChildren().RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(Load);
		if (m_Controller)
			m_Controller.m_OnGC_SpectatorEvents.Remove(OnRows);
		super.HandlerDeattached(w);
	}
}
