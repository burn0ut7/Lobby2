//! The slotting screen supplies the sole target; this dialog edits its reservation.
class GC_SlotReservationMenu : ChimeraMenuBase
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_Controller;
	protected GC_Slot m_Slot;
	protected GC_AdminPlayerPicker m_Players;
	protected EditBoxWidget m_Identity;
	protected EditBoxWidget m_PlayerName;
	protected TextWidget m_Status;
	protected SCR_EventHandlerComponent m_IdentityEvents;
	protected UUID m_ExpectedClaim;
	protected int m_iPending = -1;

	//------------------------------------------------------------------------------------------------
	static void OpenForSlot(GC_Slot slot)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		PlayerController controller = GetGame().GetPlayerController();
		if (!manager || !controller || !slot || manager.GetSlot(slot.GetID()) != slot
			|| !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return;
		GC_SlotReservationMenu menu = GC_SlotReservationMenu.Cast(GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_SlotReservationMenu, 0, true, false));
		if (!menu)
			return;
		menu.m_Slot = slot;
		menu.m_ExpectedClaim = slot.GetUUID();
		if (slot.HasClaim())
			menu.m_Identity.SetText(slot.GetUUID());
		menu.m_PlayerName.SetText(slot.GetCachedPlayerName());
		menu.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		m_Manager = GC_LobbyManager.GetManager();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Players = GC_AdminPlayerPicker.Cast(root.FindAnyWidget("ReservationPlayer").FindHandler(GC_AdminPlayerPicker));
		m_Players.SetMultiple(false);
		m_Players.m_OnChanged.Insert(OnPlayerSelected);
		m_Identity = EditBoxWidget.Cast(root.FindAnyWidget("ReservationUID"));
		m_PlayerName = EditBoxWidget.Cast(root.FindAnyWidget("ReservationName"));
		m_Identity.SetPlaceholderText("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
		m_PlayerName.SetPlaceholderText("Optional display name");
		m_IdentityEvents = SCR_EventHandlerComponent.Cast(m_Identity.FindHandler(SCR_EventHandlerComponent));
		m_IdentityEvents.GetOnChange().Insert(OnIdentityChanged);
		m_Status = TextWidget.Cast(root.FindAnyWidget("ReservationStatus"));
		Bind("ReservationClose", "Close");
		Bind("ReservationApply", "Reserve slot");
		Bind("ReservationClear", "Clear reservation");
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Insert(OnSlotChanged);
			m_Manager.m_OnPlayerRoleChanged.Insert(OnRoleChanged);
			m_Manager.m_OnLobbyPhaseChanged.Insert(OnPhaseChanged);
		}
		if (m_Controller)
			m_Controller.m_OnSlotReservationResult.Insert(OnResult);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, GetRootWidget());
		button.SetText(label);
		button.m_OnClicked.Insert(OnAction);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Manager && m_Manager == GC_LobbyManager.GetManager() && m_Controller
			&& SCR_Global.IsAdmin(m_Controller.GetPlayerId())
			&& !GC_LobbyPhase.IsAtOrAfter(m_Manager.GetLobbyPhase(), SCR_EGameModeState.GAME);
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!HasAccess())
		{
			GetGame().GetCallqueue().CallLater(Close, 0);
			return;
		}
		Widget root = GetRootWidget();
		bool current = m_Slot && m_Manager.GetSlot(m_Slot.GetID()) == m_Slot;
		string target = "Slot no longer exists";
		string detail;
		string state;
		if (current)
		{
			SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(m_Slot.GetAuthoredPrefab());
			string role = "Unit " + (m_Slot.GetGroupSlotIndex() + 1).ToString();
			if (info)
				role = WidgetManager.Translate(info.GetName());
			target = string.Format("Slot %1 - %2", m_Slot.GetID(), role);
			SCR_AIGroup group = m_Slot.GetGroup();
			if (group && group.GetFaction())
				detail = WidgetManager.Translate(group.GetFaction().GetFactionName()) + " / " + GC_GroupCallsignHelper.GetDisplayName(group);
			state = "Available";
			if (m_Slot.IsKIA())
				state = "KIA - this character is unavailable";
			else if (m_Slot.IsAwaitingInitialJoin())
				state = "Reserved for " + m_Slot.GetUUID();
			else if (m_Slot.HasClaim())
				state = "Claimed by " + m_Slot.GetCachedPlayerName();
			else if (m_Slot.IsKIA() || m_Slot.IsAdminLocked())
				state = "Slot unavailable";
			if (m_Slot.GetUUID() != m_ExpectedClaim && m_iPending < 0)
				state = "Claim changed. Close and reopen to review this slot.";
		}
		TextWidget.Cast(root.FindAnyWidget("ReservationTarget")).SetText(target);
		TextWidget.Cast(root.FindAnyWidget("ReservationTeam")).SetText(detail);
		TextWidget.Cast(root.FindAnyWidget("ReservationCurrent")).SetText(state);
		bool idle = m_iPending < 0;
		root.FindAnyWidget("ReservationFields").SetEnabled(idle);
		bool editable = idle && current && m_Slot.GetUUID() == m_ExpectedClaim && !m_Slot.IsKIA() && !m_Slot.IsAdminLocked();
		string identity = m_Identity.GetText();
		identity.TrimInPlace();
		SCR_ButtonTextComponent.GetButtonText("ReservationApply", root).SetEnabled(editable && UUID.IsUUID(identity));
		SCR_ButtonTextComponent.GetButtonText("ReservationClear", root).SetEnabled(editable && m_Slot.IsAwaitingInitialJoin());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerSelected()
	{
		array<int> players;
		array<string> identities;
		if (m_Players.GetSelection(players, identities))
		{
			m_Identity.SetText(identities[0]);
			m_PlayerName.SetText(GetGame().GetPlayerManager().GetPlayerName(players[0]));
		}
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnIdentityChanged(Widget w)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		string name = button.GetRootWidget().GetName();
		if (name == "ReservationClose")
		{
			GetGame().GetCallqueue().CallLater(Close, 0);
			return;
		}
		if (!HasAccess() || !m_Slot || m_iPending >= 0 || !button.GetRootWidget().IsEnabled())
			return;
		string action = "reserve";
		if (name == "ReservationClear")
			action = "clear";
		m_Players.CloseRoster();
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		m_Status.SetText("Waiting for server...");
		Refresh();
		m_Controller.RequestSlotReservation(action, m_Slot.GetID(), m_ExpectedClaim, m_Identity.GetText(), m_PlayerName.GetText(), "", m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome, string output, string currentClaim)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		m_Status.SetText(outcome);
		// The reply and slot replication travel on different replication items.
		// Bind the next action to the server snapshot, not a possibly older local claim.
		m_ExpectedClaim = currentClaim;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlotChanged(GC_Slot slot)
	{
		if (slot == m_Slot)
			Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRoleChanged(int playerId)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPhaseChanged(SCR_EGameModeState phase)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (IsFocused())
			GetGame().GetCallqueue().CallLater(Close, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(Close);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		if (m_Players)
		{
			m_Players.m_OnChanged.Remove(OnPlayerSelected);
			m_Players.CloseRoster();
		}
		if (m_IdentityEvents)
			m_IdentityEvents.GetOnChange().Remove(OnIdentityChanged);
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlotChanged);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnRoleChanged);
			m_Manager.m_OnLobbyPhaseChanged.Remove(OnPhaseChanged);
		}
		if (m_Controller)
			m_Controller.m_OnSlotReservationResult.Remove(OnResult);
		super.OnMenuClose();
	}
}

