//! Optional Lobby2-only test addon. Nothing runs automatically.
//! MCP calls these methods on the existing named GC_LobbyManager component.
#ifdef WORKBENCH
modded class GC_ContextMenuContext
{
	//------------------------------------------------------------------------------------------------
	//! Corrupt only the captured target, without changing any live identity or role.
	void GC_TestInvalidateCapturedPlayer()
	{
		m_PlayerIdentity = "stale-context-fixture";
	}
}

modded class GC_AdminToolsMenu
{
	//------------------------------------------------------------------------------------------------
	void GC_TestBackHandler()
	{
		OnBack();
	}
}

modded class GC_AdminPlayerPicker
{
	//------------------------------------------------------------------------------------------------
	Widget GC_TestRosterWidget()
	{
		return m_Menu;
	}
}

modded class GC_AdminCellTooltip
{
	protected Widget m_wGC_TestCell;

	//------------------------------------------------------------------------------------------------
	void GC_TestConfigure(Widget cell)
	{
		m_sPresetsConfig = "{D271A640B8000100}Configs/UI/AdminTableTooltips.conf";
		m_sPresetTag = "Cell";
		m_wGC_TestCell = cell;
		GetOnTooltipShowInit().Insert(GC_TestHoverContext);
	}

	//------------------------------------------------------------------------------------------------
	//! Supply the hovered-cell observation for a background callback test.
	void GC_TestHoverContext(SCR_ScriptedWidgetTooltip tooltip)
	{
		if (tooltip != this)
			return;

		m_wHoverWidget = m_wGC_TestCell;
		GetOnTooltipShowInit().Remove(GC_TestHoverContext);
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestRelease()
	{
		Hide(GetGame().GetWorkspace(), m_wTooltipProxy);
		GetOnTooltipShowInit().Remove(GC_TestHoverContext);
		m_Preset = null;
		m_wHoverWidget = null;
		m_wGC_TestCell = null;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestTooltipAllocation()
	{
		GC_AdminCellTooltip sample = new GC_AdminCellTooltip();
		delete sample;
		return "ALLOCATED and deleted tooltip without Show/config/widgets";
	}

	protected ref GC_AdminCellTooltip m_GC_TestCellTooltip;
	protected Widget m_wGC_TestTooltipProxy;

	//------------------------------------------------------------------------------------------------
	protected void GC_TestClearCellTooltip()
	{
		if (m_GC_TestCellTooltip)
			m_GC_TestCellTooltip.GC_TestRelease();
		if (m_wGC_TestTooltipProxy)
			m_wGC_TestTooltipProxy.RemoveFromHierarchy();
		m_wGC_TestTooltipProxy = null;
		delete m_GC_TestCellTooltip;
		m_GC_TestCellTooltip = null;
	}

	//------------------------------------------------------------------------------------------------
	override void Shutdown()
	{
		GC_TestClearCellTooltip();
		super.Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	//! Presentation fixture only. Native hover uses the actual visible cell bounds.
	string GC_TestAdminCell(string tableName, int cellIndex, string action, string value)
	{
		if (action == "hide")
		{
			GC_TestClearCellTooltip();
			GetGame().GetInputManager().SetLastUsedInputDevice(EInputDeviceType.MOUSE);
			return "HIDDEN test tooltip";
		}
		if (tableName != "LogRows" && tableName != "ModerationRows" && tableName != "BoundaryRows")
			return "REJECTED table";
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget table = root.FindAnyWidget(tableName);
		if (!GC_TestVisible(table) || !table.GetChildren())
			return "UNAVAILABLE rows";
		TextWidget cell = TextWidget.Cast(table.GetChildren().FindAnyWidget("Cell" + cellIndex.ToString()));
		if (!GC_TestVisible(cell))
			return "UNAVAILABLE cell";

		float x, y, width, height, textWidth, textHeight;
		cell.GetScreenPos(x, y);
		cell.GetScreenSize(width, height);
		if (action == "text")
			cell.SetText(value);
		else if (action == "hover")
		{
			GetGame().GetInputManager().SetLastUsedInputDevice(EInputDeviceType.MOUSE);
			GetGame().GetInputManager().GetMouseDeviceHandler().SetCursorPosition(x + width * 0.5, y + height * 0.5);
		}
		else if (action == "report-hover" || action == "report-leave")
		{
			GetGame().GetInputManager().SetLastUsedInputDevice(EInputDeviceType.MOUSE);
			if (action == "report-hover")
				WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, root);
			else
				WidgetManager.ReportMouse(-1, -1, root);
		}
		else if (action == "focus" || action == "preview")
		{
			GetGame().GetInputManager().SetLastUsedInputDevice(EInputDeviceType.KEYBOARD);
			GetGame().GetWorkspace().SetFocusedWidget(cell);
			if (action == "preview")
			{
				GC_TestClearCellTooltip();
				m_GC_TestCellTooltip = new GC_AdminCellTooltip();
				m_GC_TestCellTooltip.GC_TestConfigure(cell);
				m_wGC_TestTooltipProxy = SCR_ScriptedWidgetTooltip.CreateTooltipWidget();
				m_wGC_TestTooltipProxy.SetVisible(true);
				m_wGC_TestTooltipProxy.SetZOrder(10000);
				m_GC_TestCellTooltip.Show(GetGame().GetWorkspace(), m_wGC_TestTooltipProxy, x, y);
			}
		}
		else if (action != "inspect")
			return "REJECTED action";

		cell.GetTextSize(textWidth, textHeight);
		Widget hovered = WidgetManager.GetWidgetUnderCursor();
		string hoverName;
		if (hovered)
			hoverName = hovered.GetName();
		string tooltipText;
		SCR_ScriptedWidgetTooltip tooltip = SCR_ScriptedWidgetTooltip.GetCurrentTooltip();
		if (tooltip && tooltip.GetContent())
		{
			tooltipText = tooltip.GetContent().GetMessage();
			Widget content = tooltip.GetContentRoot();
			if (content)
			{
				float tx, ty, tw, th;
				content.GetScreenPos(tx, ty);
				content.GetScreenSize(tw, th);
				tooltipText += string.Format(" [visible=%1 x=%2 y=%3 w=%4 h=%5]", tooltip.IsVisible(), tx, ty, tw, th);
			}
		}
		return string.Format("cell=%1 width=%2 textWidth=%3 textHeight=%4 hovered=%5 text=%6 tooltip=%7", cell.GetName(), GetGame().GetWorkspace().DPIUnscale(width), textWidth, textHeight, hoverName, cell.GetText(), tooltipText);
	}

	//------------------------------------------------------------------------------------------------
	//! Inspect resolved prefab timing, including inherited buttons and open popup rows.
	string GC_TestAdminAnimation(string widgetName)
	{
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "UNAVAILABLE admin menu";
		array<Widget> widgets = {};
		GC_TestCollect(GetGame().GetWorkspace(), widgets);
		string result;
		foreach (Widget widget : widgets)
		{
			if (!GC_TestVisible(widget) || (!widgetName.IsEmpty() && widget.GetName() != widgetName))
				continue;
			SCR_WLibComponentBase component = SCR_WLibComponentBase.Cast(widget.FindHandler(SCR_WLibComponentBase));
			if (!component)
				continue;
			result += widget.GetName() + " " + component.GC_TestAnimationTiming() + "\n";
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Exercise production hover/focus presentation without activating the action.
	string GC_TestAdminButtonState(string widgetName, string state)
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget target = root.FindAnyWidget(widgetName);
		if (!GC_TestVisible(target) || !target.IsEnabled())
			return "UNAVAILABLE button";
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
		if (!button)
			return "UNAVAILABLE native handler";
		if (state == "hover")
			button.OnMouseEnter(target, 0, 0);
		else if (state == "focus")
			button.OnFocus(target, 0, 0);
		else if (state == "idle")
		{
			button.OnMouseLeave(target, null, 0, 0);
			button.OnFocusLost(target, 0, 0);
		}
		else
			return "REJECTED state";
		return "native handler invoked: " + widgetName + " " + state;
	}

	//------------------------------------------------------------------------------------------------
	//! Inspect resolved production rows; a nonnegative index selects through the native handler.
	string GC_TestAdminTable(string tableName, int selectIndex)
	{
		if (tableName != "LogRows" && tableName != "ModerationRows" && tableName != "BoundaryRows")
			return "REJECTED table";
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget table = root.FindAnyWidget(tableName);
		if (!GC_TestVisible(table))
			return "UNAVAILABLE table";

		string result;
		if (selectIndex >= 0)
		{
			Widget target = table.GetChildren();
			for (int index; target && index < selectIndex; index++)
				target = target.GetSibling();
			result = GC_TestDispatchClick(target) + "\n";
		}

		int rowCount;
		int missingCells;
		int selectedCount;
		int cellCount = 4;
		if (tableName == "BoundaryRows")
			cellCount = 5;
		if (tableName == "ModerationRows")
			cellCount = 6;
		Widget row = table.GetChildren();
		while (row)
		{
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			if (!button)
				return "FAILED native row handler";
			if (button.IsToggled())
				selectedCount++;
			for (int cellIndex = 1; cellIndex <= cellCount; cellIndex++)
			{
				TextWidget cell = TextWidget.Cast(row.FindAnyWidget("Cell" + cellIndex.ToString()));
				if (!cell)
					missingCells++;
				else if (rowCount < 3)
					result += string.Format("row=%1 cell=%2 text=%3\n", rowCount, cellIndex, cell.GetText());
			}
			rowCount++;
			row = row.GetSibling();
		}
		return string.Format("table=%1 rows=%2 missingCells=%3 selected=%4\n", tableName, rowCount, missingCells, selectedCount) + result;
	}

	//------------------------------------------------------------------------------------------------
	//! Layout-only summary; does not change native player names or selected identities.
	string GC_TestAdminLongSummary()
	{
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "UNAVAILABLE admin menu";
		TextWidget.Cast(root.FindAnyWidget("ToolSummary")).SetText(
			"Alexandra Montgomery-Sutherland [UI layout fixture] -> Christopher Richardson-Wellington [UI layout fixture]");
		return "ARRANGED summary text only; native selection and identities unchanged";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminWidgetCount()
	{
		array<Widget> widgets = {};
		GC_TestCollect(GetGame().GetWorkspace(), widgets);
		int menus;
		foreach (Widget widget : widgets)
		{
			if (widget.GetName() == "PickerMenu")
				menus++;
		}
		return string.Format("workspaceWidgets=%1 rosterPopupsIncludingHidden=%2", widgets.Count(), menus);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminRosterLoad(string pickerName, string action, string query, float value)
	{
		if (pickerName != "HealPlayers" && pickerName != "FromPlayer" && pickerName != "ToPlayer")
			return "REJECTED picker";
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "UNAVAILABLE admin menu";
		Widget field = root.FindAnyWidget(pickerName);
		GC_AdminPlayerPicker picker = GC_AdminPlayerPicker.Cast(field.FindHandler(GC_AdminPlayerPicker));
		if (!picker || !GC_TestVisible(field))
			return "UNAVAILABLE visible picker";
		if (action == "load")
		{
			if (value != 0 && value != 128)
				return "REJECTED expected 0 or 128 display rows";
			return picker.GC_TestRosterLoad(value);
		}
		if (action == "probe")
			return picker.GC_TestRosterProbe(query, value);
		if (action == "long-text")
			return picker.GC_TestLongFieldText();
		return "REJECTED action";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminStaleContext(int targetId, bool slotContext)
	{
		if (GetManager() != this)
			return "UNAVAILABLE manager";
		GC_ContextMenuContext context;
		if (slotContext)
			context = GC_ContextMenuContext.ForSlot(GetSlot(targetId));
		else
			context = GC_ContextMenuContext.ForPlayer(targetId);
		if (!context)
			return "UNAVAILABLE context";
		GC_AdminOpenToolsAction action = new GC_AdminOpenToolsAction();
		bool before = action.CanExecute(context);
		context.GC_TestInvalidateCapturedPlayer();
		bool stale = action.CanExecute(context);
		action.Execute(context);
		return string.Format("current-enabled=%1 stale-enabled=%2; inspect menu absence", before, stale);
	}

	//------------------------------------------------------------------------------------------------
	//! Engine action-value route; does not invoke a menu callback or activate a context.
	string GC_TestAdminInputValue(float value)
	{
		if (GetManager() != this || !GetGame().GetPlayerController() || (value != 0 && value != 1))
			return "UNAVAILABLE active local session or invalid pulse";
		InputManager input = GetGame().GetInputManager();
		bool active = input.IsContextActive("GC_AdminTools");
		float before = input.GetActionValue("MenuBack");
		input.SetActionValue("MenuBack", value);
		return string.Format("context=%1 before=%2 after=%3; inspect next frame", active, before, input.GetActionValue("MenuBack"));
	}

	//------------------------------------------------------------------------------------------------
	//! Uses the production context action or existing selection plus normal pause-menu path.
	string GC_TestAdminContextOpen(int playerId, bool useSelected)
	{
		if (GetManager() != this || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return "UNAVAILABLE connected player";
		if (useSelected)
		{
			SelectAdminPlayer(playerId);
			ArmaReforgerScripted.OpenPauseMenu(false, true);
			return "DISPATCHED pause; use GC_TestPauseAction tools next";
		}
		GC_AdminOpenToolsAction action = new GC_AdminOpenToolsAction();
		GC_ContextMenuContext context = GC_ContextMenuContext.ForPlayer(playerId);
		bool visible = action.IsVisible(context);
		bool enabled = action.CanExecute(context);
		action.Execute(context);
		return string.Format("context-visible=%1 enabled=%2 selected=%3; inspect fields", visible, enabled, GetAdminSelectedPlayer());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminBack()
	{
		GC_AdminToolsMenu menu = GC_AdminToolsMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!menu)
			return "UNAVAILABLE admin menu";
		menu.GC_TestBackHandler();
		return "DISPATCHED production MenuBack handler; inspect next tick";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminPopupCount()
	{
		array<Widget> widgets = {};
		GC_TestCollect(GetGame().GetWorkspace(), widgets);
		int count;
		foreach (Widget widget : widgets)
		{
			if (widget.GetName() == "PickerMenu" && GC_TestVisible(widget))
				count++;
		}
		return string.Format("visible-rosters=%1", count);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminPicker(string pickerName, string action, string value)
	{
		if (pickerName != "HealPlayers" && pickerName != "FromPlayer" && pickerName != "ToPlayer")
			return "REJECTED picker";
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget field = root.FindAnyWidget(pickerName);
		if (!GC_TestVisible(field) || !field.IsEnabled())
			return "UNAVAILABLE picker";
		GC_AdminPlayerPicker picker = GC_AdminPlayerPicker.Cast(field.FindHandler(GC_AdminPlayerPicker));
		Widget popup = picker.GC_TestRosterWidget();
		EditBoxWidget header = EditBoxWidget.Cast(field.FindAnyWidget("PickerSearch"));
		if (action == "focus")
		{
			GetGame().GetWorkspace().SetFocusedWidget(header);
			return "DISPATCHED native header focus";
		}
		if (action == "close")
		{
			picker.CloseRoster();
			return "DISPATCHED close";
		}
		if (action == "input")
		{
			SCR_EventHandlerComponent.Cast(header.FindHandler(SCR_EventHandlerComponent)).OnClick(header, 0, 0, 0);
			return "DISPATCHED header click";
		}
		if (action == "state")
		{
			array<int> selected;
			array<string> identities;
			bool valid = picker.GetSelection(selected, identities);
			return string.Format("open=%1 headerFocused=%2 writing=%3 query=%4 selected=%5 valid=%6", popup.IsVisible(),
				GetGame().GetWorkspace().GetFocusedWidget() == header, header.IsInWriteMode(), header.GetText(), selected.Count(), valid);
		}
		if (action == "open")
			return GC_TestDispatchClick(field.FindAnyWidget("PickerField"));
		if (action == "search")
		{
			EditBoxWidget search = EditBoxWidget.Cast(field.FindAnyWidget("PickerSearch"));
			if (!GC_TestVisible(search))
				return "REJECTED hidden search";
			search.SetText(value);
			SCR_EventHandlerComponent.Cast(search.FindHandler(SCR_EventHandlerComponent)).OnChange(search, false);
			return "DISPATCHED search=" + search.GetText();
		}
		if (action == "select")
			return GC_TestDispatchClick(popup.FindAnyWidget("AdminPlayer_" + value));
		if (action == "commit-select")
		{
			Widget row = popup.FindAnyWidget("AdminPlayer_" + value);
			// Native edit commit precedes the click on the already hit-tested row.
			SCR_EventHandlerComponent.Cast(header.FindHandler(SCR_EventHandlerComponent)).OnChange(header, true);
			if (!row || row != popup.FindAnyWidget("AdminPlayer_" + value))
				return "FAIL edit commit replaced the clicked row";
			return GC_TestDispatchClick(row);
		}
		return "REJECTED action";
	}


	//------------------------------------------------------------------------------------------------
	string GC_TestAdminRadiusSlider(int meters)
	{
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "UNAVAILABLE admin menu";
		Widget field = root.FindAnyWidget("HealRadius");
		if (!GC_TestVisible(field) || !field.IsEnabled())
			return "REJECTED hidden or disabled slider";
		SCR_SliderComponent slider = SCR_SliderComponent.Cast(field.FindHandler(SCR_SliderComponent));
		slider.SetValue(meters);
		return GC_TestAdminRadiusState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminRadiusState()
	{
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "UNAVAILABLE admin menu";
		Widget field = root.FindAnyWidget("HealRadius");
		SCR_SliderComponent slider = SCR_SliderComponent.Cast(field.FindHandler(SCR_SliderComponent));
		return string.Format("visible=%1 enabled=%2 value=%3 label=%4 action=%5 summary=%6", GC_TestVisible(field), field.IsEnabled(), slider.GetValue(),
			TextWidget.Cast(field.FindAnyWidget("SliderText")).GetText(), TextWidget.Cast(root.FindAnyWidget("ToolApply").FindAnyWidget("Text")).GetText(),
			TextWidget.Cast(root.FindAnyWidget("ToolSummary")).GetText());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminToolsSnapshot()
	{
		Widget root = GC_TestMenuRoot();
		if (!root || root.GetName() != "AdminToolsPanel")
			return "panel=absent";
		string result;
		array<string> names = {"ToolTitle", "ToolDescription", "ToolSummary", "ToolStatus"};
		foreach (string name : names)
		{
			TextWidget text = TextWidget.Cast(root.FindAnyWidget(name));
			result += string.Format("%1 visible=%2 text=%3\n", name, GC_TestVisible(text), text.GetText());
		}
		result += string.Format("apply-enabled=%1\n", root.FindAnyWidget("ToolApply").IsEnabled());
		array<string> pickers = {"HealPlayers", "FromPlayer", "ToPlayer"};
		foreach (string pickerName : pickers)
		{
			Widget field = root.FindAnyWidget(pickerName);
			if (!GC_TestVisible(field))
				continue;
			GC_AdminPlayerPicker picker = GC_AdminPlayerPicker.Cast(field.FindHandler(GC_AdminPlayerPicker));
			array<int> players;
			array<string> identities;
			bool valid = picker.GetSelection(players, identities);
			Widget popup = picker.GC_TestRosterWidget();
			result += string.Format("picker=%1 valid=%2 count=%3 roster=%4\n", pickerName, valid, players.Count(), GC_TestVisible(popup));
			foreach (int index, int playerId : players)
				result += string.Format("selected=%1 identity=%2\n", playerId, identities[index]);
			Widget row = popup.FindAnyWidget("PickerRows").GetChildren();
			while (row)
			{
				if (GC_TestVisible(row))
					result += string.Format("row=%1 name=%2\n", row.GetName(), TextWidget.Cast(row.FindAnyWidget("Text")).GetText());
				row = row.GetSibling();
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminSearchRanking()
	{
		array<string> names = {"Harris", "Harper", "Chris Harris", "Harrison", "Miller", "Harris"};
		array<string> queries = {"", "HAR", "harris", "hrsn", "zzzz", "harris"};
		array<int> expected = {0, 1, 2, 3, -1, 0};
		string result;
		foreach (int index, string name : names)
		{
			int actual = GC_AdminPlayerPicker.MatchRank(name, queries[index]);
			result += string.Format("case=%1 actual=%2 expected=%3 pass=%4\n", index, actual, expected[index], actual == expected[index]);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminTeleport(int fromId, string fromIdentity, int toId, string toIdentity)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE local controller";
		int requestId = controller.ReserveAdminPlayerToolRequest();
		controller.GC_RequestAdminTeleport(fromId, fromIdentity, toId, toIdentity, requestId);
		return string.Format("DISPATCHED request=%1; inspect owner/server positions and audit", requestId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminMessageUi()
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		string result;
		array<string> names = {"AdminChatButton", "AdminChatPanel", "AdminChatHistory", "AdminChatStatus", "AdminChatInput", "AdminMessagePopup", "AdminMessageText"};
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			if (!widget)
			{
				result += name + " absent\n";
				continue;
			}
			result += string.Format("%1 visible=%2 enabled=%3", name, GC_TestVisible(widget), widget.IsEnabled());
			TextWidget text = TextWidget.Cast(widget);
			EditBoxWidget edit = EditBoxWidget.Cast(widget);
			if (text)
				result += " text=" + text.GetText();
			if (edit)
				result += " draft=" + edit.GetText();
			result += "\n";
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected Widget GC_TestMenuRoot()
	{
		if (SCR_Global.IsEditMode() || GC_LobbyManager.GetManager() != this)
			return null;

		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return null;

		return menu.GetRootWidget();
	}

	//------------------------------------------------------------------------------------------------
	//! Includes ancestor visibility; hidden controls cannot be test-clicked.
	protected bool GC_TestVisible(Widget widget)
	{
		if (!widget)
			return false;
		while (widget)
		{
			if (!widget.IsVisible())
				return false;
			widget = widget.GetParent();
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestCollect(Widget widget, array<Widget> widgets)
	{
		if (!widget || !widget.IsVisible())
			return;
		widgets.Insert(widget);
		Widget child = widget.GetChildren();
		while (child)
		{
			GC_TestCollect(child, widgets);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Dispatch is not acceptance: read the next snapshot and receipt logs.
	string GC_TestClick(string widgetName)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		Widget target;
		foreach (Widget widget : widgets)
		{
			if (widget.GetName() != widgetName)
				continue;
			if (target)
				return "rejected: ambiguous visible widget name";
			target = widget;
		}
		return GC_TestDispatchClick(target);
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestDispatchClick(Widget target)
	{
		if (!GC_TestVisible(target))
			return "rejected: widget missing or hidden";
		Widget ancestor = target;
		while (ancestor)
		{
			if (!ancestor.IsEnabled())
				return "rejected: widget or ancestor disabled";
			ancestor = ancestor.GetParent();
		}
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
		if (!button)
			return "rejected: not a supported button";

		string name = target.GetName();
		button.OnClick(target, 0, 0, 0);
		return "dispatched: " + name;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelectFaction(string factionKey)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		Widget target;
		foreach (Widget widget : widgets)
		{
			GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(widget.FindHandler(GC_FactionButtonComponent));
			if (!button || button.GetFactionKey() != factionKey)
				continue;
			if (target)
				return "rejected: ambiguous visible faction";
			target = widget;
		}
		return GC_TestDispatchClick(target);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelectSlot(int slotId)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		foreach (Widget widget : widgets)
		{
			GC_SlotButtonComponent button = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
			if (button && button.GetSlot() && button.GetSlot().GetID() == slotId)
				return GC_TestDispatchClick(widget);
		}
		return "rejected: slot is not in the visible view";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestOpenBriefing(int displayOrder)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		foreach (Widget widget : widgets)
		{
			GC_BriefingTitleRowComponent row = GC_BriefingTitleRowComponent.Cast(widget.FindHandler(GC_BriefingTitleRowComponent));
			if (row && row.GetDisplayOrder() == displayOrder)
				return GC_TestDispatchClick(widget);
		}
		return "rejected: briefing row not found";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestOpenView(string view)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "rejected: no local Lobby2 controller";
		return controller.GC_TestOpenView(view);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestScroll(string widgetName, float position)
	{
		Widget root = GC_TestMenuRoot();
		if (!root || position < 0 || position > 1)
			return "rejected: no menu or position outside 0..1";
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(root.FindAnyWidget(widgetName));
		if (!GC_TestVisible(scroll))
			return "rejected: no visible scroll layout";
		scroll.SetSliderPos(0, position);
		return "dispatched: scroll";
	}

	//------------------------------------------------------------------------------------------------
	//! Layout observations are separate from IsVisible and click dispatch.
	string GC_TestWidgetGeometry(string widgetName)
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "rejected: no menu";
		Widget widget = root.FindAnyWidget(widgetName);
		if (!widget)
			return "rejected: no widget";
		string result;
		while (widget)
		{
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("%1 x=%2 y=%3 width=%4 height=%5 visible=%6 enabled=%7\n", widget.GetName(), x, y, width, height, widget.IsVisible(), widget.IsEnabled());
			widget = widget.GetParent();
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Uses native widget setters and the production row event handlers.
	string GC_TestEditParameter(int definitionIndex, string action, string value)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		foreach (Widget widget : widgets)
		{
			GC_BriefingParameterRowComponent row = GC_BriefingParameterRowComponent.Cast(widget.FindHandler(GC_BriefingParameterRowComponent));
			if (row && row.GetDefinitionIndex() == definitionIndex)
				return row.GC_TestEdit(action, value);
		}
		return "rejected: parameter is not in the visible view";
	}

	//------------------------------------------------------------------------------------------------
	//! Exercise the production right-click and action buttons, without granting roles.
	string GC_TestFactionInfo(string action, string factionKey)
	{
		if (GetManager() != this)
			return "UNAVAILABLE manager";

		SCR_HintManagerComponent hints = SCR_HintManagerComponent.GetInstance();
		SCR_HintUIInfo hint;
		if (hints)
			hint = hints.GetCurrentHint();
		if (action == "snapshot")
		{
			if (!hint)
				return "hint=absent";
			Widget hintWidget = GC_TestMenuRoot().FindAnyWidget("Hint");
			PrintFormat("GC | Hint widget exists=%1 visible=%2", hintWidget != null, hintWidget && hintWidget.IsVisibleInHierarchy());
			return string.Format("dialog=%1 title=%2\n%3", SCR_ConfigurableDialogUi.GetCurrentDialog() != null,
				hint.GetName(), hint.GetDescription());
		}
		if (action == "stale")
		{
			GC_FactionInfoAction info = new GC_FactionInfoAction();
			GC_ContextMenuContext stale = GC_ContextMenuContext.ForFaction("missing-faction-test");
			info.Execute(stale);
			return string.Format("eligible=%1 hintUnchanged=%2", info.CanExecute(stale), hints && hint == hints.GetCurrentHint());
		}

		array<Widget> widgets = {};
		GC_TestCollect(GetGame().GetWorkspace(), widgets);
		foreach (Widget widget : widgets)
		{
			if (action == "open")
			{
				GC_FactionButtonComponent faction = GC_FactionButtonComponent.Cast(widget.FindHandler(GC_FactionButtonComponent));
				if (faction && faction.GetFactionKey() == factionKey)
				{
					faction.OnClick(widget, 0, 0, 1);
					return "DISPATCHED faction right-click handler";
				}
			}
			else if (action == "info")
			{
				GC_ContextMenuActionButtonComponent button = GC_ContextMenuActionButtonComponent.Cast(widget.FindHandler(GC_ContextMenuActionButtonComponent));
				TextWidget label = TextWidget.Cast(widget.FindAnyWidget("ContextActionLabel"));
				if (button && label && label.GetText() == "Get info")
					return GC_TestDispatchClick(widget);
			}
		}
		return "UNAVAILABLE visible target";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFactionInfoTargets()
	{
		array<int> targets;
		GC_FactionBalancePolicy.Calculate({20, 40}, 30, targets);
		if (targets.Count() != 2 || targets[0] != 10 || targets[1] != 20)
			return "FAIL 20:40 slots and 30 connected players";
		GC_FactionBalancePolicy.Calculate({20, 40}, 0, targets);
		if (targets[0] != 0 || targets[1] != 0)
			return "FAIL empty server";
		GC_FactionBalancePolicy.Calculate({0, 0}, 30, targets);
		if (targets[0] != 0 || targets[1] != 0)
			return "FAIL no capacity";
		GC_FactionBalancePolicy.Calculate({20}, 30, targets);
		if (targets[0] != 30)
			return "FAIL single faction target";
		return "PASS proportional needed targets, empty server, no capacity and single faction";
	}

	//------------------------------------------------------------------------------------------------
	//! Read-only, bounded visible UI inventory plus authoritative lobby state.
	string GC_TestSnapshot()
	{
		PlayerController controller = GetGame().GetPlayerController();
		int playerId = -1;
		if (controller)
			playerId = controller.GetPlayerId();
		string result = string.Format("phase=%1 groups=%2 slots=%3 briefings=%4 revision=%5 player=%6 admin=%7\n", GetPhaseName(GetLobbyPhase()), GetRegisteredGroups().Count(), GetSlots().Count(), GetBriefings().Count(), GetScenarioParameterRevision(), playerId, controller && SCR_Global.IsAdmin(playerId));
		foreach (GC_LobbyParameterValue value : GetScenarioParameterValues())
			result += string.Format("canonical=%1 value=%2\n", value.GetDefinitionIndex(), value.GetNormalizedValue());
		Widget root = GC_TestMenuRoot();
		if (!root)
			return result + "menu=none";
		result += "menu=" + root.GetName() + "\n";
		array<Widget> widgets = {};
		GC_TestCollect(root, widgets);
		int emitted;
		foreach (Widget widget : widgets)
		{
			GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
			if (slotButton && slotButton.GetSlot())
				result += string.Format("slot=%1 player=%2\n", slotButton.GetSlot().GetID(), slotButton.GetSlot().GetPlayerId());
			GC_HeaderComponent header = GC_HeaderComponent.Cast(widget.FindHandler(GC_HeaderComponent));
			if (header)
				result += string.Format("activeView=%1\n", header.GetActiveState());
			GC_LobbyParametersComponent parameters = GC_LobbyParametersComponent.Cast(widget.FindHandler(GC_LobbyParametersComponent));
			if (parameters)
				result += parameters.GC_TestState() + "\n";
			GC_BriefingParameterRowComponent row = GC_BriefingParameterRowComponent.Cast(widget.FindHandler(GC_BriefingParameterRowComponent));
			if (row)
			{
				GC_LobbyParameterValue draft = row.BuildValue();
				string normalized = "INVALID";
				if (draft)
					normalized = draft.GetNormalizedValue();
				result += string.Format("parameter=%1 editable=%2 value=%3\n", row.GetDefinitionIndex(), row.IsEditable(), normalized);
			}
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(widget.FindHandler(SCR_ButtonBaseComponent));
			TextWidget textWidget = TextWidget.Cast(widget);
			if (button)
				result += "button=" + widget.GetName() + "\n";
			else if (textWidget && !textWidget.GetText().IsEmpty())
				result += widget.GetName() + "=" + textWidget.GetText() + "\n";
			else
				continue;
			emitted++;
			if (emitted >= 100)
				return result + "TRUNCATED: first 100 visible text/button entries";
		}
		return result;
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestOpenView(string view)
	{
		if (GetGame().GetPlayerController() != this)
			return "rejected: not the local controller";
		if (view == "lobby")
			RequestOpenSlottingFromPause();
		else if (view == "briefing")
			RequestOpenBriefingFromPause();
		else
			return "rejected: expected lobby or briefing";
		return "dispatched: production pause-menu action";
	}
}
#endif
