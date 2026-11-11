//! Presentation for the existing authoritative mission-ending owner.
class GC_AdminEndingComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_AdminSearchFilter m_Result;
	protected GC_AdminSearchFilter m_Faction;
	protected int m_iRevision;
	protected int m_iPending = -1;
	protected bool m_bActive;
	protected bool m_bEnding;
	protected bool m_bReceived;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Result = GC_AdminSearchFilter.Cast(w.FindAnyWidget("EndResult").FindHandler(GC_AdminSearchFilter));
		m_Faction = GC_AdminSearchFilter.Cast(w.FindAnyWidget("EndFaction").FindHandler(GC_AdminSearchFilter));
		EditBoxWidget.Cast(m_Result.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("Choose result");
		EditBoxWidget.Cast(m_Faction.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("Choose faction");
		m_Result.SetChoices({"Choose result", "Mission complete", "Faction victory", "Faction defeat", "Faction draw", "Mission failed"}, {"", "0", "1", "2", "3", "4"}, {"", "", "", "", "", ""});
		Bind("EndStart", "Start / replace timer");
		Bind("EndCancel", "Cancel ending");
		Bind("EndNow", "End match now");
		EditBoxWidget.Cast(w.FindAnyWidget("EndSeconds")).SetText("120");
		if (m_Controller)
			m_Controller.m_GC_OnBoundaryResult.Insert(OnResult);
		GetGame().GetCallqueue().CallLater(Poll, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
		button.SetText(label);
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
		{
			m_Result.CloseList();
			m_Faction.CloseList();
		}
		if (!opening)
			return;
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		array<string> labels = {"Choose faction"}, values = {""}, terms = {""};
		foreach (Faction faction : factions)
		{
			labels.Insert(WidgetManager.Translate(faction.GetFactionName()));
			values.Insert(faction.GetFactionKey());
			terms.Insert(faction.GetFactionKey());
		}
		m_Faction.SetChoices(labels, values, terms);
		Poll();
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!m_bActive || !HasAccess() || m_iPending >= 0)
			return;
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		array<string> factions = {};
		m_Controller.GC_RequestBoundary("status", "", "", 0, factions, m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void Status(string text)
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("EndStatus")).SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome, string snapshot)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		if (!outcome.IsEmpty())
			Status(outcome);
		JsonLoadContext json = new JsonLoadContext(false);
		float remaining;
		m_bReceived = !snapshot.IsEmpty() && json.LoadFromString(snapshot);
		if (m_bReceived)
		{
			json.ReadValue("endingRevision", m_iRevision);
			json.ReadValue("ending", m_bEnding);
			json.ReadValue("remaining", remaining);
		}
		string timer = "No mission ending scheduled";
		if (m_bEnding)
			timer = "Mission ends in " + Math.Ceil(remaining).ToString() + " seconds";
		TextWidget.Cast(m_wRoot.FindAnyWidget("EndCountdown")).SetText(timer);
		bool enabled = HasAccess() && m_bReceived && GC_LobbyGameMode.GetGameMode().GetState() == SCR_EGameModeState.GAME;
		SCR_ButtonTextComponent.GetButtonText("EndStart", m_wRoot).SetEnabled(enabled);
		SCR_ButtonTextComponent.GetButtonText("EndNow", m_wRoot).SetEnabled(enabled);
		SCR_ButtonTextComponent.GetButtonText("EndCancel", m_wRoot).SetEnabled(enabled && m_bEnding);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || !m_bReceived || m_iPending >= 0)
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "EndCancel")
		{
			m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
			array<string> factions = {};
			m_Controller.GC_RequestBoundary("ending-cancel", m_iRevision.ToString(), "", 0, factions, m_iPending);
			return;
		}
		string result = m_Result.GetFilterValue();
		if (!m_Result.HasChoice() || result.IsEmpty())
		{
			Status("Choose a mission result.");
			return;
		}
		string faction = m_Faction.GetFilterValue();
		if (result.ToInt() > 0 && result.ToInt() < 4 && (!m_Faction.HasChoice() || faction.IsEmpty()))
		{
			Status("Choose the faction for this result.");
			return;
		}
		string seconds = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("EndSeconds")).GetText();
		int duration = seconds.ToInt();
		if (seconds != duration.ToString() || duration < 1 || duration > 3600)
		{
			Status("Enter whole seconds from 1 to 3600.");
			return;
		}
		string message = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("EndMessage")).GetText();
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		Status("Waiting for server...");
		m_Controller.GC_RequestMissionEnding(m_iRevision, result.ToInt(), faction, message, duration, name == "EndNow", m_iPending);
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
