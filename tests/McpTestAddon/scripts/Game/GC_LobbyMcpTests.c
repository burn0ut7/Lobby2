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

modded class GC_LobbyManager
{
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
		if (action == "open")
			return GC_TestDispatchClick(field.FindAnyWidget("PickerField"));
		if (action == "search")
		{
			EditBoxWidget search = EditBoxWidget.Cast(popup.FindAnyWidget("PickerSearch"));
			if (!GC_TestVisible(search))
				return "REJECTED hidden search";
			search.SetText(value);
			SCR_EventHandlerComponent.Cast(search.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(search);
			return "DISPATCHED search=" + search.GetText();
		}
		if (action == "select")
			return GC_TestDispatchClick(popup.FindAnyWidget("AdminPlayer_" + value));
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
