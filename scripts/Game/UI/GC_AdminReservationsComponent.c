//! Roster transfer only. Select individual slots in the existing slotting screen.
class GC_AdminReservationsComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_Controller;
	protected MultilineEditBoxWidget m_Transfer;
	protected TextWidget m_Status;
	protected int m_iPending = -1;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Manager = GC_LobbyManager.GetManager();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Transfer = MultilineEditBoxWidget.Cast(w.FindAnyWidget("ReservationData"));
		m_Status = TextWidget.Cast(w.FindAnyWidget("ReservationStatus"));
		Bind("ReservationExport", "Export roster");
		Bind("ReservationCopy", "Copy");
		Bind("ReservationPaste", "Paste");
		Bind("ReservationImport", "Import reservations");
		if (m_Controller)
			m_Controller.m_OnSlotReservationResult.Insert(OnResult);
		if (m_Manager)
			m_Manager.m_OnLobbyPhaseChanged.Insert(OnPhaseChanged);
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
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Manager && m_Manager == GC_LobbyManager.GetManager() && m_Controller
			&& SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		bool idle = HasAccess() && m_iPending < 0;
		m_Transfer.SetEnabled(idle);
		SCR_ButtonTextComponent.GetButtonText("ReservationExport", m_wRoot).SetEnabled(idle);
		SCR_ButtonTextComponent.GetButtonText("ReservationCopy", m_wRoot).SetEnabled(idle);
		SCR_ButtonTextComponent.GetButtonText("ReservationPaste", m_wRoot).SetEnabled(idle);
		SCR_ButtonTextComponent.GetButtonText("ReservationImport", m_wRoot).SetEnabled(idle
			&& !GC_LobbyPhase.IsAtOrAfter(m_Manager.GetLobbyPhase(), SCR_EGameModeState.GAME));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || m_iPending >= 0 || !button.GetRootWidget().IsEnabled())
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "ReservationCopy")
		{
			System.ExportToClipboard(m_Transfer.GetText());
			m_Status.SetText("Copied roster text.");
			return;
		}
		if (name == "ReservationPaste")
		{
			string clipboard = System.ImportFromClipboard();
			if (clipboard.Length() > 32768)
				m_Status.SetText("Paste supports 32 KiB or less.");
			else
			{
				m_Transfer.SetText(clipboard);
				m_Status.SetText("Pasted. Review the text, then import.");
			}
			return;
		}
		string action;
		if (name == "ReservationExport")
			action = "export";
		else if (name == "ReservationImport")
			action = "import";
		else
			return;
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		m_Status.SetText("Waiting for server...");
		Refresh();
		m_Controller.RequestSlotReservation(action, -1, UUID.NULL_UUID, "", "", m_Transfer.GetText(), m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome, string output, string currentClaim)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		if (!output.IsEmpty())
			m_Transfer.SetText(output);
		m_Status.SetText(outcome);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPhaseChanged(SCR_EGameModeState phase)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_Controller)
			m_Controller.m_OnSlotReservationResult.Remove(OnResult);
		if (m_Manager)
			m_Manager.m_OnLobbyPhaseChanged.Remove(OnPhaseChanged);
		super.HandlerDeattached(w);
	}
}

