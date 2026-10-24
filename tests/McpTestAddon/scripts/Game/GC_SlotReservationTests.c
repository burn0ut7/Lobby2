#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected string m_sGCReservationOutcome;
	protected string m_sGCReservationOutput;
	protected int m_iGCReservationRequest = -1;

	//------------------------------------------------------------------------------------------------
	//! Exercise the owner-controller RPC and record its correlated result.
	string GC_TestReservationRequest(string action, int slotId, string expected, string identity, string playerName, string input)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller || m_iGCReservationRequest >= 0)
			return "UNAVAILABLE local controller or request pending";
		m_sGCReservationOutcome = "";
		m_sGCReservationOutput = "";
		m_iGCReservationRequest = controller.ReserveAdminPlayerToolRequest();
		controller.m_OnSlotReservationResult.Insert(GC_TestReservationResult);
		controller.RequestSlotReservation(action, slotId, expected, identity, playerName, input, m_iGCReservationRequest);
		return "DISPATCHED request=" + m_iGCReservationRequest.ToString();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestReservationResult(int requestId, string outcome, string output, string currentClaim)
	{
		if (requestId != m_iGCReservationRequest)
			return;
		m_sGCReservationOutcome = outcome;
		m_sGCReservationOutput = output;
		m_iGCReservationRequest = -1;
		GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).m_OnSlotReservationResult.Remove(GC_TestReservationResult);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestReservationRead(string kind)
	{
		if (kind == "output")
			return m_sGCReservationOutput;
		if (kind == "outcome")
			return m_sGCReservationOutcome;
		if (kind == "ui-data")
		{
			Widget root = GC_TestMenuRoot();
			if (!root || !root.FindAnyWidget("ReservationData"))
				return "UNAVAILABLE transfer field";
			return MultilineEditBoxWidget.Cast(root.FindAnyWidget("ReservationData")).GetText();
		}
		string result = string.Format("phase=%1 pending=%2 outcome=%3\n", GetPhaseName(GetLobbyPhase()), m_iGCReservationRequest, m_sGCReservationOutcome);
		foreach (GC_Slot slot : GetSlots())
		{
			string faction;
			int nativeGroup = -1;
			if (slot.GetGroup() && slot.GetGroup().GetFaction())
				faction = slot.GetGroup().GetFaction().GetFactionKey();
			if (slot.GetPlayerId() > 0 && GetPlayerGroup(slot.GetPlayerId()))
				nativeGroup = GetPlayerGroup(slot.GetPlayerId()).GetGroupID();
			result += string.Format("slot=%1 uid=%2 player=%3 initial=%4 state=%5 name=%6 faction=%7 nativeGroup=%8\n",
				slot.GetID(), slot.GetUUID(), slot.GetPlayerId(), slot.IsAwaitingInitialJoin(), slot.GetState(),
				slot.GetCachedPlayerName(), faction, nativeGroup);
		}
		Widget menu = GC_TestMenuRoot();
		if (menu)
		{
			result += "menu=" + menu.GetName() + "\n";
			array<string> labels = {"ReservationTarget", "ReservationTeam", "ReservationCurrent", "ReservationStatus"};
			foreach (string label : labels)
			{
				TextWidget text = TextWidget.Cast(menu.FindAnyWidget(label));
				if (text && GC_TestVisible(text))
					result += label + "=" + text.GetText() + "\n";
			}
			array<string> fields = {"ReservationUID", "ReservationName"};
			foreach (string fieldName : fields)
			{
				EditBoxWidget field = EditBoxWidget.Cast(menu.FindAnyWidget(fieldName));
				if (field)
					result += fieldName + "=" + field.GetText() + "\n";
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestReservationUI(string action, int slotId, string fieldName, string value)
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		if (action == "context")
		{
			array<Widget> widgets = {};
			GC_TestCollect(root, widgets);
			foreach (Widget widget : widgets)
			{
				GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
				if (slotButton && slotButton.GetSlot() && slotButton.GetSlot().GetID() == slotId)
					return "rightClick=" + slotButton.OnClick(widget, 0, 0, 1).ToString();
			}
			return "UNAVAILABLE visible slot";
		}
		if (action == "reserve-action")
		{
			Widget actions = GetGame().GetWorkspace().FindAnyWidget("ContextActions");
			if (!actions)
				return "UNAVAILABLE context menu";
			Widget row = actions.GetChildren();
			while (row)
			{
				TextWidget label = TextWidget.Cast(row.FindAnyWidget("ContextActionLabel"));
				if (label && label.GetText() == "Reserve slot...")
					return GC_TestDispatchClick(row);
				row = row.GetSibling();
			}
			return "UNAVAILABLE reservation action";
		}
		if (action == "field")
		{
			if (fieldName != "ReservationUID" && fieldName != "ReservationName" && fieldName != "ReservationData")
				return "REJECTED field";
			Widget field = root.FindAnyWidget(fieldName);
			if (!GC_TestVisible(field) || !field.IsEnabled())
				return "UNAVAILABLE field";
			if (fieldName == "ReservationData")
				MultilineEditBoxWidget.Cast(field).SetText(value);
			else
			{
				EditBoxWidget.Cast(field).SetText(value);
				SCR_EventHandlerComponent.Cast(field.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(field);
			}
			return "EDITED " + fieldName;
		}
		Widget pickerRoot = root.FindAnyWidget("ReservationPlayer");
		if (!GC_TestVisible(pickerRoot) || !pickerRoot.IsEnabled())
			return "UNAVAILABLE picker";
		GC_AdminPlayerPicker picker = GC_AdminPlayerPicker.Cast(pickerRoot.FindHandler(GC_AdminPlayerPicker));
		if (action == "picker-open")
			return GC_TestDispatchClick(pickerRoot.FindAnyWidget("PickerField"));
		if (action == "picker-select")
			return GC_TestDispatchClick(picker.GC_TestRosterWidget().FindAnyWidget("AdminPlayer_" + value));
		return "REJECTED action";
	}

	//------------------------------------------------------------------------------------------------
	//! Remove host roles only; restarting disposable Play restores the normal host login.
	string GC_TestReservationLogout()
	{
		if (!Replication.IsServer() || RplSession.Mode() == RplMode.Dedicated)
			return "UNSUPPORTED embedded host only";
		int requester = GetGame().GetPlayerController().GetPlayerId();
		PlayerManager players = GetGame().GetPlayerManager();
		players.ClearPlayerRole(requester, EPlayerRole.ADMINISTRATOR);
		players.ClearPlayerRole(requester, EPlayerRole.SESSION_ADMINISTRATOR);
		return "admin=" + SCR_Global.IsAdmin(requester).ToString();
	}
}
#endif

