//! Fixed personal history and a live spectator combat feed.
class GC_SpectatorEventLogComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected bool m_bActive;
	protected bool m_bEveryone = true;
	protected bool m_bFocused;
	protected ref array<string> m_aRows = {};
	protected ref map<SCR_ButtonBaseComponent, int> m_mRows = new map<SCR_ButtonBaseComponent, int>();
	ref ScriptInvoker m_OnClose = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		Bind("EventYou", "You");
		Bind("EventPlayer", "Player");
		Bind("EventEveryone", "Everyone");
		Bind("EventClose", "Close");
		if (m_Controller)
		{
			m_Controller.m_OnGC_SpectatorEvents.Insert(OnRows);
			m_Controller.m_OnGC_SpectatorFocus.Insert(OnFocusChanged);
		}
	}

	protected void OnFocusChanged()
	{
		if (m_Controller.GC_GetFocusedPlayer() > 0)
			return;
		m_wRoot.FindAnyWidget("EventPlayer").SetVisible(false);
		if (!m_bFocused)
			return;
		m_bFocused = false;
		m_bEveryone = true;
		if (m_bActive)
			ShowTab();
	}

	//------------------------------------------------------------------------------------------------
	void ShowFocusedPlayer()
	{
		m_bFocused = true;
		m_bEveryone = false;
		ShowTab();
	}

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
		ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll")).SetSliderPos(0, 0);
		SCR_ButtonTextComponent.GetButtonText("EventYou", m_wRoot).SetToggled(!m_bEveryone && !m_bFocused, false, false);
		SCR_ButtonTextComponent.GetButtonText("EventEveryone", m_wRoot).SetToggled(m_bEveryone && !m_bFocused, false, false);
		SCR_ButtonTextComponent.GetButtonText("EventPlayer", m_wRoot).SetToggled(m_bFocused, false, false);
		m_wRoot.FindAnyWidget("EventPlayer").SetVisible(m_Controller && m_Controller.GC_GetFocusedPlayer() > 0);
		if (HasAccess() && !m_bFocused)
			m_Controller.GC_EnsureSpectatorEvents(m_bEveryone);
		RefreshRows();
	}

	//------------------------------------------------------------------------------------------------
	//! Local pagination only. Everyone updates arrive from the controller cache.
	protected void Poll()
	{
		if (!HasAccess())
		{
			SetActive(false);
			ClearRows();
			return;
		}
		if (!m_bFocused && (m_bEveryone || m_Controller.GC_GetPersonalEventNext() <= 0))
			return;
		float x, y;
		ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll")).GetSliderPos(x, y);
		if (y >= 0.9)
		{
			if (m_bFocused)
				m_Controller.GC_LoadFocusedEventPage();
			else
				m_Controller.GC_LoadPersonalEventPage();
		}
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
		bool focused = name == "EventPlayer";
		if (m_bEveryone == everyone && m_bFocused == focused)
		{
			button.SetToggled(true, false, false);
			return;
		}
		m_bEveryone = everyone;
		m_bFocused = focused;
		ShowTab();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRows(bool everyone)
	{
		if (m_bActive && HasAccess() && (m_bFocused || everyone == m_bEveryone))
			RefreshRows();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshRows()
	{
		if (!HasAccess())
			return;
		array<string> rows = m_Controller.GC_GetCachedSpectatorEvents(m_bEveryone);
		if (m_bFocused)
		{
			rows = m_Controller.GC_GetFocusEvents();
			SetStatus(m_Controller.GC_GetFocusEventStatus());
		}
		else
			SetStatus(m_Controller.GC_GetSpectatorEventStatus(m_bEveryone));
		bool same = rows.Count() == m_aRows.Count();
		for (int i; same && i < rows.Count(); i++)
			same = rows[i] == m_aRows[i];
		if (same)
			return;
		float scrollX, scrollY;
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("EventScroll"));
		scroll.GetSliderPosPixels(scrollX, scrollY);
		ClearRows();
		m_aRows.Copy(rows);
		AppendRows(0);
		if (!m_bEveryone || m_bFocused)
			scroll.SetSliderPosPixels(scrollX, scrollY);
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
		if (m_Controller)
		{
			m_Controller.m_OnGC_SpectatorEvents.Remove(OnRows);
			m_Controller.m_OnGC_SpectatorFocus.Remove(OnFocusChanged);
		}
		super.HandlerDeattached(w);
	}
}
