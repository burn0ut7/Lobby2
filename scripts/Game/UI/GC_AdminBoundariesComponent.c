//! One row per zone, independent of how many factions share it.
class GC_AdminBoundariesComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected ref array<ref GC_BoundaryAdminRow> m_aZones = {};
	protected ref map<string, Widget> m_mRows = new map<string, Widget>();
	protected ref map<SCR_ButtonBaseComponent, string> m_mActions = new map<SCR_ButtonBaseComponent, string>();
	protected ref map<SCR_ButtonBaseComponent, string> m_mFactions = new map<SCR_ButtonBaseComponent, string>();
	protected ref array<string> m_aSelectedFactions = {};
	protected bool m_bAllFactions = true;
	protected bool m_bShowMap;
	protected string m_sSelectedZone;
	protected bool m_bActive;
	protected bool m_bAnnouncement;
	protected SCR_ComboBoxComponent m_Type;
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected int m_iPending = -1;
	protected int m_iPanicPending = -1;
	protected int m_iLastResult = -1;
	protected string m_sFactionSignature;
	protected string m_sAnnouncement;
	protected string m_sAnnouncementFactions;
	protected float m_fAnnouncementRemaining;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		Bind("BoundaryList", "Boundaries");
		Bind("BoundaryNotice", "Announcement timer");
		Bind("BoundaryMap", "Show all zones on map");
		Bind("BoundaryPanic", "Stop all boundaries");
		Bind("NoticeStart", "Start timer");
		Bind("NoticeStop", "Stop timer");
		EditBoxWidget.Cast(w.FindAnyWidget("NoticeTitle")).SetText("HOLD");
		EditBoxWidget.Cast(w.FindAnyWidget("NoticeMinutes")).SetText("5");
		m_Type = SCR_ComboBoxComponent.Cast(w.FindAnyWidget("BoundaryType").FindHandler(SCR_ComboBoxComponent));
		array<string> types = {"All boundaries", "AO limits", "Setup timers"};
		foreach (string type : types)
			m_Type.AddItem(type);
		m_Type.SetCurrentItem(0, false, false, false);
		m_Type.m_OnChanged.Insert(OnType);
		SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("BoundaryType").FindAnyWidget("ComboButton").FindHandler(SCR_ButtonBaseComponent)).m_OnClicked.Insert(OnOpenType);
		m_Search = EditBoxWidget.Cast(w.FindAnyWidget("BoundarySearch"));
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearch);
		if (m_Controller)
			m_Controller.m_GC_OnBoundaryResult.Insert(OnResult);
		GetGame().GetCallqueue().CallLater(Poll, 1000, true);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string text)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
		button.SetText(text);
		button.m_OnClicked.Insert(OnAction);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Controller && GC_LobbyGameMode.GetGameMode() && SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		bool opening = active && !m_bActive;
		m_bActive = active;
		m_wRoot.SetVisible(active);
		if (!active)
			m_Type.CloseList();
		if (opening)
			Poll();
	}

	//------------------------------------------------------------------------------------------------
	void Panic()
	{
		if (!HasAccess() || m_iPanicPending >= 0)
			return;
		m_iPanicPending = m_Controller.ReserveAdminPlayerToolRequest();
		SetStatus("Stopping all boundaries...");
		array<string> factions = {};
		m_Controller.GC_RequestBoundary("panic", "", "", 0, factions, m_iPanicPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!m_bActive || !HasAccess() || m_iPending >= 0)
			return;
		Request("status");
	}

	//------------------------------------------------------------------------------------------------
	protected void Request(string action, string key = "", string title = "", int seconds = 0)
	{
		if (!HasAccess() || m_iPending >= 0)
			return;
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		array<string> factions = {};
		if (!m_bAllFactions)
			factions.Copy(m_aSelectedFactions);
		if (action != "status")
			SetStatus("Waiting for server...");
		m_Controller.GC_RequestBoundary(action, key, title, seconds, factions, m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text)
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("BoundaryStatus")).SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome, string snapshot)
	{
		if (requestId != m_iPending && requestId != m_iPanicPending)
			return;
		if (!outcome.IsEmpty())
			SetStatus(outcome);
		if (requestId == m_iPending)
			m_iPending = -1;
		if (requestId == m_iPanicPending)
			m_iPanicPending = -1;
		if (requestId < m_iLastResult)
			return;
		m_iLastResult = requestId;
		if (!snapshot.IsEmpty())
		{
			JsonLoadContext json = new JsonLoadContext(false);
			array<ref GC_BoundaryAdminRow> zones = {};
			if (json.LoadFromString(snapshot) && json.ReadValue("zones", zones))
			{
				m_aZones = zones;
				json.ReadValue("showMap", m_bShowMap);
				json.ReadValue("announcement", m_sAnnouncement);
				json.ReadValue("remaining", m_fAnnouncementRemaining);
				json.ReadValue("factions", m_sAnnouncementFactions);
			}
		}
		else
			m_aZones.Clear();
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess())
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "BoundaryPanic")
		{
			Panic();
			return;
		}
		if (name == "BoundaryList" || name == "BoundaryNotice")
		{
			SetStatus(string.Empty);
			m_bAnnouncement = name == "BoundaryNotice";
			m_Type.CloseList();
			Refresh();
			return;
		}
		if (m_iPending >= 0)
			return;
		string key;
		if (m_mActions.Find(button, key))
		{
			if (name == "ZoneExtend")
			{
				Widget row = m_mRows.Get(key);
				int seconds = ParseExtension(EditBoxWidget.Cast(row.FindAnyWidget("ZoneTime")).GetText());
				if (seconds < 1)
				{
					SetStatus("Enter seconds or m:ss, from 1 second to 60 minutes (for example 30 or 2:00).");
					return;
				}
				Request("extend", key, "", seconds);
			}
			else
			{
				foreach (GC_BoundaryAdminRow zone : m_aZones)
				{
					if (zone.Key != key)
						continue;
					string action = "end";
					if (zone.Kind == "AO")
					{
						action = "enable";
						if (zone.Enabled)
							action = "disable";
					}
					Request(action, key);
					break;
				}
			}
		}
		else if (name == "BoundaryMap")
		{
			if (m_bShowMap)
				Request("map-off");
			else
				Request("map-on");
		}
		else if (name == "NoticeStop")
			Request("announce-stop");
		else if (name == "NoticeStart")
		{
			if (!m_bAllFactions && m_aSelectedFactions.IsEmpty())
			{
				SetStatus("Select at least one faction, or All factions.");
				return;
			}
			string title = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("NoticeTitle")).GetText();
			float minutes = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("NoticeMinutes")).GetText().ToFloat();
			Request("announce-start", "", title, Math.Round(minutes * 60));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		foreach (string key, Widget row : m_mRows)
			row.RemoveFromHierarchy();
		m_mRows.Clear();
		m_mActions.Clear();
		m_sSelectedZone = "";
	}

	//------------------------------------------------------------------------------------------------
	//! Accept whole seconds or minutes:seconds without silently truncating malformed input.
	protected int ParseExtension(string value)
	{
		value.TrimInPlace();
		if (value.IsEmpty() || value.Length() > 7)
			return 0;
		array<string> parts = {};
		value.Split(":", parts, false);
		if (parts.Count() < 1 || parts.Count() > 2)
			return 0;
		foreach (string part : parts)
		{
			if (part.IsEmpty())
				return 0;
			for (int i = 0; i < part.Length(); i++)
			{
				int character = part.ToAscii(i);
				if (character < 48 || character > 57)
					return 0;
			}
		}
		int seconds = parts[0].ToInt();
		if (parts.Count() == 2)
		{
			if (parts[1].Length() != 2 || parts[1].ToInt() > 59)
				return 0;
			seconds = seconds * 60 + parts[1].ToInt();
		}
		if (seconds < 1 || seconds > 3600)
			return 0;
		return seconds;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnType(SCR_SelectionWidgetComponent combo, int index)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenType(SCR_ButtonBaseComponent button)
	{
		if (m_bActive && HasAccess())
			m_Type.OpenList();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearch(Widget widget)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected string StateText(GC_BoundaryAdminRow zone)
	{
		string state = zone.State;
		if (zone.Kind == "Setup" && zone.Enabled)
		{
			int remaining = Math.Ceil(zone.Remaining);
			string seconds = (remaining % 60).ToString();
			if (remaining % 60 < 10)
				seconds = "0" + seconds;
			state += " " + (remaining / 60).ToString() + ":" + seconds;
		}
		return state;
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		m_wRoot.FindAnyWidget("BoundaryScroll").SetVisible(!m_bAnnouncement);
		m_wRoot.FindAnyWidget("BoundaryTableHeading").SetVisible(!m_bAnnouncement);
		m_wRoot.FindAnyWidget("BoundaryTimeHeading").SetVisible(!m_bAnnouncement && m_Type.GetCurrentIndex() != 1);
		m_wRoot.FindAnyWidget("BoundarySelection").SetVisible(!m_bAnnouncement);
		m_wRoot.FindAnyWidget("AnnouncementFields").SetVisible(m_bAnnouncement);
		m_wRoot.FindAnyWidget("BoundaryMap").SetVisible(!m_bAnnouncement);
		m_wRoot.FindAnyWidget("BoundaryFilters").SetVisible(!m_bAnnouncement);
		SCR_ButtonTextComponent.GetButtonText("BoundaryList", m_wRoot).SetToggled(!m_bAnnouncement, false, false);
		SCR_ButtonTextComponent.GetButtonText("BoundaryNotice", m_wRoot).SetToggled(m_bAnnouncement, false, false);
		Widget mapToggle = m_wRoot.FindAnyWidget("BoundaryMap");
		mapToggle.FindAnyWidget("Tick").SetVisible(m_bShowMap);
		if (m_bAnnouncement)
		{
			m_wRoot.FindAnyWidget("BoundaryEmpty").SetVisible(false);
			RebuildFactions();
			string current;
			if (m_fAnnouncementRemaining > 0)
				current = m_sAnnouncement + " — " + Math.Ceil(m_fAnnouncementRemaining).ToString() + "s\nFactions: " + m_sAnnouncementFactions;
			TextWidget.Cast(m_wRoot.FindAnyWidget("NoticeCurrent")).SetText(current);
			string start = "Start timer";
			if (m_fAnnouncementRemaining > 0)
				start = "Replace timer";
			SCR_ButtonTextComponent.GetButtonText("NoticeStart", m_wRoot).SetText(start);
			return;
		}
		array<string> currentKeys = {};
		string query = m_Search.GetText();
		query.TrimInPlace();
		query.ToLower();
		int type = m_Type.GetCurrentIndex();
		foreach (GC_BoundaryAdminRow zone : m_aZones)
		{
			if ((type == 1 && zone.Kind != "AO") || (type == 2 && zone.Kind != "Setup"))
				continue;
			string kind = "AO limit";
			if (zone.Kind == "Setup")
				kind = "Setup timer";
			string state = StateText(zone);
			string search = zone.Name + " " + kind + " " + state + " " + zone.Factions + " " + zone.Behavior + " " + zone.Details;
			search.ToLower();
			if (!query.IsEmpty() && !search.Contains(query))
				continue;
			currentKeys.Insert(zone.Key);
			Widget row;
			if (!m_mRows.Find(zone.Key, row))
			{
				row = GetGame().GetWorkspace().CreateWidgets("{6A5A00000000B002}UI/layouts/Admin/Lobby_BoundaryRow.layout", m_wRoot.FindAnyWidget("BoundaryRows"));
				if (!row)
					continue;
				m_mRows.Set(zone.Key, row);
				row.SetName(zone.Key);
				SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent)).m_OnClicked.Insert(OnZoneSelected);
				EditBoxWidget.Cast(row.FindAnyWidget("ZoneTime")).SetText("5:00");
				array<string> actions = {"ZoneAction", "ZoneExtend"};
				foreach (string actionName : actions)
				{
					SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(actionName, row);
					button.m_OnClicked.Insert(OnAction);
					m_mActions.Set(button, zone.Key);
				}
			}
			TextWidget heading = TextWidget.Cast(row.FindAnyWidget("Cell1"));
			heading.SetText(zone.Name);
			TextWidget.Cast(row.FindAnyWidget("Cell2")).SetText(kind);
			TextWidget.Cast(row.FindAnyWidget("Cell3")).SetText(state);
			TextWidget.Cast(row.FindAnyWidget("Cell4")).SetText(zone.Factions);
			TextWidget.Cast(row.FindAnyWidget("Cell5")).SetText(zone.Behavior);
			SCR_ButtonTextComponent action = SCR_ButtonTextComponent.GetButtonText("ZoneAction", row);
			string label = "End setup";
			if (zone.Kind == "AO")
			{
				label = "Enable AO";
				if (zone.Enabled)
					label = "Disable AO";
			}
			action.SetText(label);
			action.SetEnabled(zone.Valid && (zone.Kind == "AO" || zone.Enabled));
			SCR_ButtonTextComponent extend = SCR_ButtonTextComponent.GetButtonText("ZoneExtend", row);
			extend.SetText("Add time");
			extend.GetRootWidget().SetVisible(zone.Kind == "Setup");
			extend.SetEnabled(zone.Valid && zone.Enabled);
			row.FindAnyWidget("ZoneTimeFields").SetVisible(zone.Kind == "Setup");
			row.FindAnyWidget("ZoneTimeFields").SetEnabled(zone.Valid && zone.Enabled);
		}
		array<string> removed = {};
		foreach (string key, Widget row : m_mRows)
		{
			if (!currentKeys.Contains(key))
			{
				m_mActions.Remove(SCR_ButtonTextComponent.GetButtonText("ZoneAction", row));
				m_mActions.Remove(SCR_ButtonTextComponent.GetButtonText("ZoneExtend", row));
				row.RemoveFromHierarchy();
				removed.Insert(key);
			}
		}
		foreach (string key : removed)
			m_mRows.Remove(key);
		m_wRoot.FindAnyWidget("BoundaryEmpty").SetVisible(currentKeys.IsEmpty());
		RefreshSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnZoneSelected(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess())
			return;
		m_sSelectedZone = button.GetRootWidget().GetName();
		RefreshSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshSelection()
	{
		foreach (string key, Widget row : m_mRows)
			SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent)).SetToggled(key == m_sSelectedZone, false, false);
		string detail;
		foreach (GC_BoundaryAdminRow zone : m_aZones)
		{
			if (zone.Key == m_sSelectedZone && m_mRows.Contains(zone.Key))
				detail = zone.Name + " | " + zone.State + "\n" + zone.Factions + " | " + zone.Behavior + "\n" + zone.Details;
		}
		TextWidget.Cast(m_wRoot.FindAnyWidget("BoundarySelectionDetail")).SetText(detail);
	}


	//------------------------------------------------------------------------------------------------
	protected void RebuildFactions()
	{
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		string signature;
		foreach (Faction faction : factions)
		{
			if (faction)
				signature += faction.GetFactionKey() + ";";
		}
		if (signature == m_sFactionSignature && !m_mFactions.IsEmpty())
			return;
		m_sFactionSignature = signature;
		Widget parent = m_wRoot.FindAnyWidget("NoticeFactions");
		while (parent.GetChildren())
			parent.GetChildren().RemoveFromHierarchy();
		m_mFactions.Clear();
		array<string> keys = {""};
		array<string> labels = {"All factions"};
		foreach (Faction faction : factions)
		{
			if (!faction || faction.GetFactionKey().IsEmpty() || keys.Contains(faction.GetFactionKey()))
				continue;
			keys.Insert(faction.GetFactionKey());
			labels.Insert(WidgetManager.Translate(faction.GetFactionName()) + " (" + faction.GetFactionKey() + ")");
		}
		foreach (int index, string key : keys)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets("{6A5A000000000010}UI/layouts/Admin/Lobby_AdminOption.layout", parent);
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
			button.SetText(labels[index]);
			button.m_OnClicked.Insert(OnFaction);
			m_mFactions.Set(button, key);
			button.SetToggled((key.IsEmpty() && m_bAllFactions) || (!m_bAllFactions && m_aSelectedFactions.Contains(key)), false, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFaction(SCR_ButtonBaseComponent button)
	{
		string key;
		if (!m_mFactions.Find(button, key))
			return;
		if (key.IsEmpty())
		{
			m_bAllFactions = true;
			m_aSelectedFactions.Clear();
		}
		else
		{
			m_bAllFactions = false;
			if (m_aSelectedFactions.Contains(key))
				m_aSelectedFactions.RemoveItem(key);
			else
				m_aSelectedFactions.Insert(key);
		}
		foreach (SCR_ButtonBaseComponent choice, string factionKey : m_mFactions)
			choice.SetToggled((factionKey.IsEmpty() && m_bAllFactions) || (!m_bAllFactions && m_aSelectedFactions.Contains(factionKey)), false, false);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Poll);
		if (m_Type)
			m_Type.m_OnChanged.Remove(OnType);
		if (m_SearchEvents)
			m_SearchEvents.GetOnChange().Remove(OnSearch);
		if (m_Controller)
			m_Controller.m_GC_OnBoundaryResult.Remove(OnResult);
		super.HandlerDeattached(w);
	}
}
