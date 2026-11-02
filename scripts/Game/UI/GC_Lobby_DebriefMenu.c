//! Cached final-mission views; UI filtering never calls gameplay collectors.
class GC_DebriefPlayerRow
{
	string m_sId;
	string m_sName;
	string m_sFaction;
	ref array<string> m_aValues = {};
}

class GC_Lobby_DebriefMenu : ChimeraMenuBase
{
	protected GC_LobbyPlayerController m_Controller;
	protected ref map<SCR_ButtonBaseComponent, string> m_mSelections = new map<SCR_ButtonBaseComponent, string>();
	protected ref map<SCR_ButtonBaseComponent, string> m_mRows = new map<SCR_ButtonBaseComponent, string>();
	protected ref array<SCR_ButtonBaseComponent> m_aActions = {};
	protected ref array<ref GC_DebriefPlayerRow> m_aPlayers = {};
	protected ref set<int> m_PlayerPages = new set<int>();
	protected ref set<int> m_CombatPages = new set<int>();
	protected ref array<string> m_aCombat = {};
	protected ref array<string> m_aValues = {};
	protected ref array<string> m_aLabels = {"Play time", "Shots fired", "Grenades", "Bandages given", "Player kills", "AI kills", "Team kills", "Deaths", "Deaths by players", "Deaths by AI", "Other deaths", "On foot", "Driving / piloting", "Radio time (estimated)", "Command"};
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected GC_AdminSearchFilter m_Sort;
	protected GC_AdminSearchFilter m_EventFilter;
	protected string m_sFaction;
	protected string m_sSubject;
	protected string m_sTitle = "Private";
	protected string m_sView = "overview";
	protected string m_sReportKey;
	protected string m_sCombatPrefix;
	protected string m_sPlayerStatus;
	protected int m_iCombatFilter;
	protected int m_iPlayerNext;
	protected int m_iCombatNext;
	protected bool m_bDescending;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_HeaderComponent header = GC_HeaderComponent.Cast(root.FindAnyWidget("MenuBar").FindHandler(GC_HeaderComponent));
		if (header)
			header.SetActiveState(4);
		AddSelection("Private", "");
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			foreach (Faction faction : manager.GetRegisteredFactions())
				AddSelection(WidgetManager.Translate(faction.GetFactionName()), faction.GetFactionKey());
		}
		AddSelection("Other players", "players");
		Bind("DebriefBack", "Back (Esc)");
		Bind("ReportOverview", "Overview");
		Bind("ReportCombat", "Activity");
		Bind("ReportPlayers", "Players");
		Bind("ReportDirection", "");
		m_Search = EditBoxWidget.Cast(root.FindAnyWidget("ReportSearch"));
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearch);
		m_Sort = GC_AdminSearchFilter.Cast(root.FindAnyWidget("ReportSort").FindHandler(GC_AdminSearchFilter));
		array<string> labels = {"Name"};
		array<string> values = {""};
		array<string> terms = {""};
		for (int i; i < 14; i++)
		{
			labels.Insert(m_aLabels[i]);
			values.Insert(i.ToString());
			terms.Insert(m_aLabels[i]);
		}
		m_Sort.SetChoices(labels, values, terms);
		EditBoxWidget.Cast(m_Sort.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("Sort: Name");
		m_Sort.m_OnFilterChanged.Insert(OnSort);
		m_EventFilter = GC_AdminSearchFilter.Cast(root.FindAnyWidget("ReportFilter").FindHandler(GC_AdminSearchFilter));
		m_EventFilter.SetChoices({"All events", "Kills", "Deaths", "Damage received", "Damage dealt"}, {"", "1", "2", "3", "4"}, {"", "", "", "", ""});
		EditBoxWidget.Cast(m_EventFilter.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("All events");
		m_EventFilter.m_OnFilterChanged.Insert(OnEventFilter);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		if (m_Controller)
			m_Controller.m_GC_OnReportPage.Insert(OnPage);
		ShowOverview();
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, GetRootWidget());
		button.SetText(label);
		button.m_OnClicked.Insert(OnAction);
		m_aActions.Insert(button);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddSelection(string label, string key)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets("{E37A21004D940001}UI/layouts/Lobby_DebriefRow.layout", GetRootWidget().FindAnyWidget("DebriefSelections"));
		row.SetName("DebriefSelection_" + key);
		TextWidget caption = TextWidget.Cast(row.FindAnyWidget("Cell1"));
		caption.SetText(label);
		FrameSlot.SetAnchorMax(caption.GetParent(), 1, 1);
		for (int cell = 2; cell <= 6; cell++)
			row.FindAnyWidget("Cell" + cell).GetParent().SetVisible(false);
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
		button.m_OnClicked.Insert(OnSelection);
		m_mSelections.Set(button, key);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelection(SCR_ButtonBaseComponent selected)
	{
		m_sFaction = m_mSelections.Get(selected);
		m_sSubject = "";
		m_sTitle = TextWidget.Cast(selected.GetRootWidget().FindAnyWidget("Cell1")).GetText();
		m_Search.SetText("");
		if (m_sFaction == "players")
			ShowPlayers();
		else
			ShowOverview();
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowOverview()
	{
		m_sView = "overview";
		m_aValues.Clear();
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo")).SetText("");
		SetStatus("Loading…");
		string kind = "player";
		string subject = m_sSubject;
		if (!m_sFaction.IsEmpty() && m_sFaction != "players")
		{
			kind = "faction";
			subject = m_sFaction;
		}
		m_sReportKey = string.Format("%1|%2|0|0", kind, subject);
		Refresh();
		if (m_Controller)
			m_Controller.GC_RequestReport(kind, subject);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowPlayers()
	{
		m_sView = "players";
		m_sTitle = "Players";
		m_sFaction = "players";
		Refresh();
		if (m_iPlayerNext >= 0 && m_Controller)
			m_Controller.GC_RequestReport("players", "", m_iPlayerNext);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowCombat()
	{
		m_sView = "combat";
		m_aCombat.Clear();
		m_CombatPages.Clear();
		m_iCombatNext = 0;
		m_sCombatPrefix = "combat|" + m_sSubject + "|";
		SetStatus("Loading…");
		Refresh();
		if (m_Controller)
			m_Controller.GC_RequestReport("combat", m_sSubject, 0, m_iCombatFilter);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPage(string key, GC_DebriefPage page)
	{
		if (key.IndexOf("players|") == 0)
		{
			m_sPlayerStatus = page.m_sStatus;
			// Request keys contain an empty subject; extract cursor from the stable suffix.
			array<string> fields = {};
			key.Split("|", fields, false);
			int offset = fields[2].ToInt();
			if (!m_PlayerPages.Contains(offset))
			{
				m_PlayerPages.Insert(offset);
				for (int i; i + 3 < page.m_aRows.Count(); i += 4)
				{
					GC_DebriefPlayerRow player = new GC_DebriefPlayerRow();
					player.m_sId = page.m_aRows[i];
					player.m_sName = page.m_aRows[i + 1];
					player.m_sFaction = page.m_aRows[i + 2];
					page.m_aRows[i + 3].Split("\n", player.m_aValues, false);
					m_aPlayers.Insert(player);
				}
			}
			m_iPlayerNext = page.m_iNext;
			if (m_sView == "players")
			{
				SetStatus(page.m_sStatus);
				QueueRefresh();
				if (m_iPlayerNext >= 0)
					GetGame().GetCallqueue().CallLater(ContinuePlayers, 0);
			}
		}
		else if (key == m_sReportKey && m_sView != "players")
		{
			m_aValues.Clear();
			if (page.m_aRows.Count() == 2)
			{
				page.m_aRows[0].Split("\n", m_aValues, false);
				TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo")).SetText(page.m_aRows[1]);
			}
			SetStatus(page.m_sStatus);
			QueueRefresh();
		}
		else if (m_sView == "combat" && key.IndexOf(m_sCombatPrefix) == 0)
		{
			array<string> fields = {};
			key.Split("|", fields, false);
			if (fields.Count() != 4 || fields[3].ToInt() != m_iCombatFilter)
				return;
			int cursor = fields[2].ToInt();
			if (!m_CombatPages.Contains(cursor))
			{
				m_CombatPages.Insert(cursor);
				m_aCombat.InsertAll(page.m_aRows);
			}
			m_iCombatNext = page.m_iNext;
			SetStatus(page.m_sStatus);
			QueueRefresh();
			if (m_iCombatNext >= 0)
				GetGame().GetCallqueue().CallLater(ContinueCombat, 0);
		}
	}

	protected void ContinuePlayers()
	{
		if (m_sView == "players" && m_iPlayerNext >= 0 && m_Controller)
			m_Controller.GC_RequestReport("players", "", m_iPlayerNext);
	}

	protected void ContinueCombat()
	{
		if (m_sView == "combat" && m_iCombatNext >= 0 && m_Controller)
			m_Controller.GC_RequestReport("combat", m_sSubject, m_iCombatNext, m_iCombatFilter);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		string name = button.GetRootWidget().GetName();
		if (name == "DebriefBack")
			OnBack();
		else if (name == "ReportOverview")
			ShowOverview();
		else if (name == "ReportCombat")
			ShowCombat();
		else if (name == "ReportPlayers")
			ShowPlayers();
		else if (name == "ReportDirection")
		{
			m_bDescending = !m_bDescending;
			Refresh();
		}
	}

	protected void OnSearch()
	{
		QueueRefresh();
	}

	protected void OnSort()
	{
		m_bDescending = m_Sort.HasChoice() && !m_Sort.GetFilterValue().IsEmpty();
		QueueRefresh();
	}

	protected void OnEventFilter()
	{
		if (!m_EventFilter.HasChoice())
			return;
		m_iCombatFilter = Math.Max(0, GC_DebriefText.Ordinal(m_EventFilter.GetFilterValue()));
		ShowCombat();
	}

	protected void QueueRefresh()
	{
		GetGame().GetCallqueue().Remove(Refresh);
		GetGame().GetCallqueue().CallLater(Refresh, 100);
	}

	protected void SetStatus(string status)
	{
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefStatus")).SetText(status);
	}

	//------------------------------------------------------------------------------------------------
	protected bool Matches(string text)
	{
		string query = m_Search.GetText();
		query.ToLower();
		text.ToLower();
		array<string> terms = {};
		query.Split(" ", terms, true);
		foreach (string term : terms)
		{
			if (!text.Contains(term))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		Widget root = GetRootWidget();
		Widget focused = GetGame().GetWorkspace().GetFocusedWidget();
		string focusedRow;
		while (focused && focused != root)
		{
			if (focused.GetName().StartsWith("ReportPlayer_"))
				focusedRow = focused.GetName();
			focused = focused.GetParent();
		}
		bool overview = m_sView == "overview";
		bool players = m_sView == "players";
		bool combat = m_sView == "combat";
		foreach (SCR_ButtonBaseComponent selection, string key : m_mSelections)
			selection.SetToggled(key == m_sFaction, false, false);
		TextWidget.Cast(root.FindAnyWidget("DebriefPlayer")).SetText(m_sTitle);
		root.FindAnyWidget("DebriefPlayer").SetVisible(!players && m_sFaction == "players");
		root.FindAnyWidget("StatsScroll").SetVisible(overview);
		root.FindAnyWidget("AmmoHeading").SetVisible(overview);
		root.FindAnyWidget("AmmoScroll").SetVisible(overview);
		root.FindAnyWidget("ReportTable").SetVisible(!overview);
		root.FindAnyWidget("ReportTableHeader").SetVisible(!overview);
		root.FindAnyWidget("ReportSort").SetVisible(players);
		root.FindAnyWidget("ReportDirection").SetVisible(players);
		root.FindAnyWidget("ReportSortArrow").SetVisible(players);
		root.FindAnyWidget("ReportFilter").SetVisible(combat);
		root.FindAnyWidget("ReportOverview").SetVisible(!players);
		root.FindAnyWidget("ReportCombat").SetVisible(!players && (m_sFaction.IsEmpty() || m_sFaction == "players"));
		root.FindAnyWidget("ReportPlayers").SetVisible(false);
		SCR_ButtonTextComponent.GetButtonText("ReportOverview", root).SetToggled(overview, false, false);
		SCR_ButtonTextComponent.GetButtonText("ReportCombat", root).SetToggled(combat, false, false);
		string direction = "sortArrowUp";
		if (m_bDescending)
			direction = "sortArrowDown";
		ImageWidget.Cast(root.FindAnyWidget("ReportSortArrow")).LoadImageFromSet(0, "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset", direction);
		float searchTop = 0.10;
		if (combat)
			searchTop = 0.17;
		if (overview)
			searchTop = 0.435;
		FrameSlot.SetAnchorMin(root.FindAnyWidget("SearchSurface"), 0.22, searchTop);
		FrameSlot.SetAnchorMax(root.FindAnyWidget("SearchSurface"), 0.71, searchTop + 0.05);
		FrameSlot.SetAnchorMin(m_Search, 0.23, searchTop + 0.005);
		FrameSlot.SetAnchorMax(m_Search, 0.70, searchTop + 0.048);
		float headerTop = 0.175;
		float rowsTop = 0.23;
		if (combat)
		{
			headerTop = 0.245;
			rowsTop = 0.30;
		}
		FrameSlot.SetAnchorMin(root.FindAnyWidget("ReportTableHeader"), 0.22, headerTop);
		FrameSlot.SetAnchorMax(root.FindAnyWidget("ReportTableHeader"), 0.97, headerTop + 0.045);
		FrameSlot.SetAnchorMin(root.FindAnyWidget("ReportTable"), 0.22, rowsTop);
		FrameSlot.SetAnchorMax(root.FindAnyWidget("ReportTable"), 0.97, 0.94);
		for (int hero; hero < 6; hero++)
		{
			root.FindAnyWidget("HeroValue" + hero).SetVisible(overview);
			root.FindAnyWidget("HeroLabel" + hero).SetVisible(overview);
		}
		ClearRows();
		if (players)
			RenderPlayers();
		else if (combat)
			RenderCombat();
		else
			RenderOverview();
		RenderHeroes();
		if (!focusedRow.IsEmpty())
		{
			Widget restored = root.FindAnyWidget(focusedRow);
			if (restored)
				GetGame().GetWorkspace().SetFocusedWidget(restored);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RenderOverview()
	{
		int shown;
		for (int i; i < 15; i++)
		{
			TextWidget label = TextWidget.Cast(GetRootWidget().FindAnyWidget("StatLabel" + i));
			TextWidget value = TextWidget.Cast(GetRootWidget().FindAnyWidget("StatValue" + i));
			string text = "—";
			if (m_aValues.Count() == 15)
				text = m_aValues[i];
			string caption = m_aLabels[i];
			if (i == 14 && !m_sFaction.IsEmpty() && m_sFaction != "players")
				caption = "Participants";
			label.SetText(caption);
			value.SetText(text);
			bool visible = i != 1 && i != 2 && i != 3 && i != 4 && i != 5 && i != 7 && Matches(caption + " " + text);
			label.SetVisible(visible);
			value.SetVisible(visible);
			if (visible)
			{
				int column = shown / 5;
				int line = shown % 5;
				float left = column * 0.52;
				FrameSlot.SetAnchorMin(label, left, 0);
				FrameSlot.SetAnchorMax(label, left + 0.31, 0);
				FrameSlot.SetAnchorMin(value, left + 0.32, 0);
				FrameSlot.SetAnchorMax(value, left + 0.48, 0);
				FrameSlot.SetOffsets(label, 0, line * 58, 0, -(line * 58 + 54));
				FrameSlot.SetOffsets(value, 0, line * 58, 0, -(line * 58 + 54));
				shown++;
			}
		}
		SizeLayoutWidget.Cast(GetRootWidget().FindAnyWidget("StatsSize")).SetHeightOverride(Math.Min(shown, 5) * 58);
		GetGame().GetCallqueue().CallLater(ResizeAmmo, 0);
	}

	protected void ResizeAmmo()
	{
		GetRootWidget().Update();
		float width, height;
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefAmmo")).GetTextSize(width, height);
		SizeLayoutWidget.Cast(GetRootWidget().FindAnyWidget("AmmoSize")).SetHeightOverride(height + 16);
	}

	//------------------------------------------------------------------------------------------------
	protected void RenderHeroes()
	{
		array<int> indexes = {1, 4, 7, 5, 2, 3};
		for (int i; i < 6; i++)
		{
			string value = "—";
			if (m_sView == "players")
			{
				int total;
				foreach (GC_DebriefPlayerRow player : m_aPlayers)
				{
					if (player.m_aValues.Count() == 15)
						total += player.m_aValues[indexes[i]].ToInt();
				}
				value = total.ToString();
			}
			else if (m_aValues.Count() == 15)
				value = m_aValues[indexes[i]];
			TextWidget.Cast(GetRootWidget().FindAnyWidget("HeroValue" + i)).SetText(value);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float SortValue(GC_DebriefPlayerRow player, int index)
	{
		string value = player.m_aValues[index];
		if (index == 0 || index == 13)
		{
			array<string> parts = {};
			value.Split("m", parts, false);
			if (parts.Count() == 2)
			{
				string seconds = parts[1];
				seconds.Replace("s", "");
				seconds.TrimInPlace();
				return parts[0].ToFloat() * 60 + seconds.ToFloat();
			}
		}
		value.Replace(" m", "");
		if (index == 12)
		{
			array<string> distances = {};
			value.Split("/", distances, false);
			if (distances.Count() == 2)
			{
				string first = distances[0];
				string second = distances[1];
				first.TrimInPlace();
				second.TrimInPlace();
				return first.ToFloat() + second.ToFloat();
			}
		}
		return value.ToFloat();
	}

	protected void RenderPlayers()
	{
		string sort = m_Sort.GetFilterValue();
		int index = -1;
		if (m_Sort.HasChoice() && !sort.IsEmpty())
			index = sort.ToInt();
		bool numeric = m_Sort.HasChoice() && !sort.IsEmpty() && index >= 0 && index < 14;
		array<GC_DebriefPlayerRow> ordered = {};
		foreach (GC_DebriefPlayerRow player : m_aPlayers)
		{
			if (player.m_aValues.Count() != 15)
				continue;
			string text = player.m_sName + " " + player.m_sFaction;
			for (int field; field < 15; field++)
				text += " " + m_aLabels[field] + " " + player.m_aValues[field];
			if (!Matches(text))
				continue;
			int position = ordered.Count();
			for (int j; j < ordered.Count(); j++)
			{
				string left = player.m_sName;
				string right = ordered[j].m_sName;
				left.ToLower();
				right.ToLower();
				bool before = left.Compare(right) < 0;
				if (numeric)
				{
					float difference = SortValue(player, index) - SortValue(ordered[j], index);
					if (difference != 0)
						before = (m_bDescending && difference > 0) || (!m_bDescending && difference < 0);
				}
				else if (m_bDescending)
					before = left.Compare(right) > 0;
				if (before)
				{
					position = j;
					break;
				}
			}
			if (position == ordered.Count())
				ordered.Insert(player);
			else
				ordered.InsertAt(player, position);
		}
		string extra = "Shots";
		int extraIndex = 1;
		if (numeric && index != 4 && index != 5 && index != 7)
		{
			extraIndex = index;
			extra = m_aLabels[index];
		}
		MakeRow({"Player", "Player kills", "AI kills", "Deaths", extra}, "", true);
		foreach (GC_DebriefPlayerRow row : ordered)
			MakeRow({row.m_sName, row.m_aValues[4], row.m_aValues[5], row.m_aValues[7], row.m_aValues[extraIndex]}, row.m_sId, false, row.m_sFaction);
		string status = ordered.Count().ToString() + " players";
		if (ordered.Count() == 1)
			status = "1 player";
		if (m_iPlayerNext >= 0)
			status += " · Loading…";
		if (!m_sPlayerStatus.IsEmpty())
			status += " · " + m_sPlayerStatus;
		SetStatus(status);
	}

	protected void RenderCombat()
	{
		MakeRow({"Time", "Event", "From", "To", "Damage", "Weapon / range"}, "", true);
		for (int i; i + 7 < m_aCombat.Count(); i += 8)
		{
			array<string> cells = {};
			string search;
			for (int j; j < 6; j++)
			{
				cells.Insert(m_aCombat[i + j]);
				search += " " + m_aCombat[i + j];
			}
			if (Matches(search))
				MakeRow(cells, "", false, m_aCombat[i + 6], m_aCombat[i + 7]);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void MakeRow(array<string> cells, string id, bool heading = false, string actorFaction = "", string targetFaction = "")
	{
		Widget parent = GetRootWidget().FindAnyWidget("ReportRows");
		if (heading)
			parent = GetRootWidget().FindAnyWidget("ReportTableHeader");
		Widget row = GetGame().GetWorkspace().CreateWidgets("{E37A21004D940001}UI/layouts/Lobby_DebriefRow.layout", parent);
		SizeLayoutWidget.Cast(row.GetChildren()).SetHeightOverride(48);
		array<float> widths = {0, 0.40, 0.55, 0.70, 0.85, 1};
		if (m_sView == "combat")
			widths = {0, 0.08, 0.20, 0.43, 0.66, 0.76, 1};
		for (int i; i < 6; i++)
		{
			TextWidget cell = TextWidget.Cast(row.FindAnyWidget("Cell" + (i + 1)));
			cell.GetParent().SetVisible(i < cells.Count());
			if (i >= cells.Count())
				continue;
			cell.SetText(cells[i]);
			FrameSlot.SetAnchorMin(cell.GetParent(), widths[i], 0);
			FrameSlot.SetAnchorMax(cell.GetParent(), widths[i + 1], 1);
		}
		if (!heading)
		{
			if (m_sView == "players")
				ColorName(row, "Cell1", actorFaction);
			else
			{
				ColorName(row, "Cell3", actorFaction);
				ColorName(row, "Cell4", targetFaction);
			}
		}
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
		if (heading)
		{
			row.SetEnabled(false);
			row.SetOpacity(0.65);
		}
		else if (!id.IsEmpty())
		{
			row.SetName("ReportPlayer_" + id);
			m_mRows.Set(button, id);
			button.m_OnClicked.Insert(OnPlayer);
		}
	}

	protected void ColorName(Widget row, string cellName, string key)
	{
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(key);
		if (faction)
			row.FindAnyWidget(cellName).SetColor(faction.GetFactionColor());
	}

	protected void OnPlayer(SCR_ButtonBaseComponent button)
	{
		m_sSubject = m_mRows.Get(button);
		foreach (GC_DebriefPlayerRow player : m_aPlayers)
		{
			if (player.m_sId == m_sSubject)
				m_sTitle = player.m_sName;
		}
		m_Search.SetText("");
		ShowOverview();
	}

	protected void ClearRows()
	{
		foreach (SCR_ButtonBaseComponent button, string id : m_mRows)
			button.m_OnClicked.Remove(OnPlayer);
		m_mRows.Clear();
		Widget parent = GetRootWidget().FindAnyWidget("ReportRows");
		while (parent.GetChildren())
			parent.GetChildren().RemoveFromHierarchy();
		parent = GetRootWidget().FindAnyWidget("ReportTableHeader");
		while (parent.GetChildren())
			parent.GetChildren().RemoveFromHierarchy();
	}

	protected void OnBack()
	{
		if (!IsFocused())
			return;
		if (m_sFaction == "players" && m_sView != "players")
			ShowPlayers();
		else
			GetGame().GetCallqueue().CallLater(Close, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_Controller)
			m_Controller.m_GC_OnReportPage.Remove(OnPage);
		m_SearchEvents.GetOnChange().Remove(OnSearch);
		m_Sort.m_OnFilterChanged.Remove(OnSort);
		m_EventFilter.m_OnFilterChanged.Remove(OnEventFilter);
		foreach (SCR_ButtonBaseComponent button : m_aActions)
			button.m_OnClicked.Remove(OnAction);
		foreach (SCR_ButtonBaseComponent selection, string key : m_mSelections)
			selection.m_OnClicked.Remove(OnSelection);
		ClearRows();
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		GetGame().GetCallqueue().Remove(Close);
		GetGame().GetCallqueue().Remove(Refresh);
		GetGame().GetCallqueue().Remove(ResizeAmmo);
		GetGame().GetCallqueue().Remove(ContinuePlayers);
		GetGame().GetCallqueue().Remove(ContinueCombat);
		super.OnMenuClose();
	}
}
