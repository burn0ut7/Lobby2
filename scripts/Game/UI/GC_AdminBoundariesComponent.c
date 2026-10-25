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
	protected bool m_bActive;
	protected int m_iTab;
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
		Bind("BoundaryAOs", "AO limits");
		Bind("BoundarySetups", "Setup timers");
		Bind("BoundaryNotice", "Announcement timer");
		Bind("BoundaryMap", "All zones on map: Off");
		Bind("BoundaryPanic", "Stop all boundaries");
		Bind("NoticeStart", "Start timer");
		Bind("NoticeStop", "Stop timer");
		EditBoxWidget.Cast(w.FindAnyWidget("NoticeTitle")).SetText("HOLD");
		EditBoxWidget.Cast(w.FindAnyWidget("NoticeMinutes")).SetText("5");
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
		if (name == "BoundaryAOs" || name == "BoundarySetups" || name == "BoundaryNotice")
		{
			m_iTab = 0;
			if (name == "BoundarySetups")
				m_iTab = 1;
			else if (name == "BoundaryNotice")
				m_iTab = 2;
			ClearRows();
			Refresh();
			return;
		}
		if (m_iPending >= 0)
			return;
		string key;
		if (m_mActions.Find(button, key))
		{
			if (name == "ZoneExtend")
				Request("extend", key);
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
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		m_wRoot.FindAnyWidget("BoundaryScroll").SetVisible(m_iTab != 2);
		m_wRoot.FindAnyWidget("AnnouncementFields").SetVisible(m_iTab == 2);
		m_wRoot.FindAnyWidget("BoundaryMap").SetVisible(m_iTab != 2);
		SCR_ButtonTextComponent.GetButtonText("BoundaryAOs", m_wRoot).SetToggled(m_iTab == 0, false, false);
		SCR_ButtonTextComponent.GetButtonText("BoundarySetups", m_wRoot).SetToggled(m_iTab == 1, false, false);
		SCR_ButtonTextComponent.GetButtonText("BoundaryNotice", m_wRoot).SetToggled(m_iTab == 2, false, false);
		string mapText = "All zones on map: Off";
		if (m_bShowMap)
			mapText = "All zones on map: On";
		SCR_ButtonTextComponent.GetButtonText("BoundaryMap", m_wRoot).SetText(mapText);
		if (m_iTab == 2)
		{
			m_wRoot.FindAnyWidget("BoundaryEmpty").SetVisible(false);
			RebuildFactions();
			string current = "No announcement running. Display only; no movement or combat restrictions.";
			if (m_fAnnouncementRemaining > 0)
				current = m_sAnnouncement + " — " + Math.Ceil(m_fAnnouncementRemaining).ToString() + "s remaining\nApplies to: " + m_sAnnouncementFactions + "\nDisplay only; no movement or combat restrictions.";
			TextWidget.Cast(m_wRoot.FindAnyWidget("NoticeCurrent")).SetText(current);
			string start = "Start timer";
			if (m_fAnnouncementRemaining > 0)
				start = "Replace timer";
			SCR_ButtonTextComponent.GetButtonText("NoticeStart", m_wRoot).SetText(start);
			return;
		}
		array<string> currentKeys = {};
		foreach (GC_BoundaryAdminRow zone : m_aZones)
		{
			if ((m_iTab == 0 && zone.Kind != "AO") || (m_iTab == 1 && zone.Kind != "Setup"))
				continue;
			currentKeys.Insert(zone.Key);
			Widget row;
			if (!m_mRows.Find(zone.Key, row))
			{
				row = GetGame().GetWorkspace().CreateWidgets("{6A5A00000000B002}UI/layouts/Admin/Lobby_BoundaryRow.layout", m_wRoot.FindAnyWidget("BoundaryRows"));
				if (!row)
					continue;
				m_mRows.Set(zone.Key, row);
				array<string> actions = {"ZoneAction", "ZoneExtend"};
				foreach (string actionName : actions)
				{
					SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(actionName, row);
					button.m_OnClicked.Insert(OnAction);
					m_mActions.Set(button, zone.Key);
				}
			}
			TextWidget heading = TextWidget.Cast(row.FindAnyWidget("ZoneName"));
			heading.SetText(zone.Name);
			WrapZoneText(heading, row, zone.Name);
			float headingWidth, headingHeight;
			heading.GetTextSize(headingWidth, headingHeight);
			float bodyTop = Math.Max(40, headingHeight + 12);
			FrameSlot.SetSizeY(heading, Math.Max(30, headingHeight));
			TextWidget body = TextWidget.Cast(row.FindAnyWidget("ZoneBody"));
			string details = "Affected factions: " + zone.Factions + "\nStatus: " + zone.State;
			if (zone.Kind == "Setup" && zone.Enabled)
			{
				int remaining = Math.Ceil(zone.Remaining);
				string seconds = (remaining % 60).ToString();
				if (remaining % 60 < 10)
					seconds = "0" + seconds;
				details += " | " + (remaining / 60).ToString() + ":" + seconds + " remaining";
			}
			details += "\nMode: " + zone.Behavior + "\n" + zone.Details;
			body.SetText(details);
			WrapZoneText(body, row, details);
			float width, height;
			body.GetTextSize(width, height);
			FrameSlot.SetPosY(body, bodyTop);
			FrameSlot.SetSizeY(body, Math.Max(80, height));
			SizeLayoutWidget.Cast(row).SetHeightOverride(Math.Max(132, height + bodyTop + 14));
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
			extend.SetText("+5 minutes");
			extend.GetRootWidget().SetVisible(zone.Kind == "Setup");
			extend.SetEnabled(zone.Valid && zone.Enabled);
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
	}


	//------------------------------------------------------------------------------------------------
	//! Native text measurement excludes automatic wrapping. Explicit line breaks
	//! let the row reserve the full height without clipping names or faction lists.
	protected void WrapZoneText(TextWidget text, Widget row, string value)
	{
		GC_AdminTextWrapping.Apply(text, row, value);
		float screenWidth, screenHeight;
		text.GetScreenSize(screenWidth, screenHeight);
		float available = GetGame().GetWorkspace().DPIUnscale(screenWidth) - 4;
		if (available <= 0)
			return;

		array<string> lines = {};
		text.GetText().Split("\n", lines, false);
		string display;
		text.SetTextWrapping(false);
		foreach (int lineIndex, string line : lines)
		{
			if (lineIndex > 0)
				display += "\n";
			array<string> words = {};
			line.Split(" ", words, false);
			string current;
			foreach (string word : words)
			{
				string candidate = word;
				if (!current.IsEmpty())
					candidate = current + " " + word;
				text.SetText(candidate);
				float width, height;
				text.GetTextSize(width, height);
				if (width > available && !current.IsEmpty())
				{
					display += current + "\n";
					current = word;
				}
				else
					current = candidate;
			}
			display += current;
		}
		text.SetText(display);
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
		if (m_Controller)
			m_Controller.m_GC_OnBoundaryResult.Remove(OnResult);
		super.HandlerDeattached(w);
	}
}
