//! Bounded owner-side actions for the explicitly selected dedicated regression.
modded class GC_LobbyPlayerController
{
	protected string m_sGCDedicatedToken;
	protected string m_sGCDedicatedBefore;
	protected int m_iGCDedicatedRequest;

	//------------------------------------------------------------------------------------------------
	protected void GC_TestDedicatedResult(int requestId, string outcome)
	{
		if (m_sGCDedicatedToken.IsEmpty() || (m_iGCDedicatedRequest >= 0 && requestId != m_iGCDedicatedRequest))
			return;
		if (outcome.StartsWith("pending-ban-operation"))
			return;
		m_OnAdminPlayerToolResult.Remove(GC_TestDedicatedResult);
		m_OnAdminMessageResult.Remove(GC_TestDedicatedResult);
		m_OnAdminGroupLockResult.Remove(GC_TestDedicatedResult);
		m_OnAdminFactionRulesResult.Remove(GC_TestDedicatedResult);
		m_OnAdminModerationResult.Remove(GC_TestDedicatedResult);
		m_OnAdminBanPage.Remove(GC_TestDedicatedBanPage);
		Rpc(GC_TestRpcPeerReport, m_sGCDedicatedToken,
			m_sGCDedicatedBefore + string.Format(" request=%1 outcome=%2", requestId, outcome));
		m_sGCDedicatedToken = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestDedicatedBanPage(array<string> identities, array<string> labels, int page, int total)
	{
		m_sGCDedicatedBefore += string.Format(" banPage=%1 banRows=%2 banLabels=%3 banTotal=%4", page, identities.Count(), labels.Count(), total);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestDedicatedReservation(int requestId, string outcome, string output, string claim)
	{
		if (requestId != m_iGCDedicatedRequest)
			return;
		m_OnSlotReservationResult.Remove(GC_TestDedicatedReservation);
		JsonLoadContext data = new JsonLoadContext(false);
		int version;
		array<ref map<string, string>> slots;
		bool parsed = data.LoadFromString(output) && data.ReadValue("version", version) && data.ReadValue("slots", slots);
		int count;
		if (slots)
			count = slots.Count();
		GC_TestDedicatedResult(requestId, outcome + string.Format(" parsed=%1 version=%2 slots=%3", parsed, version, count));
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestDedicatedMapPlan(int requestId, string outcome, string output)
	{
		if (requestId != m_iGCDedicatedRequest)
			return;
		m_GC_OnMapPlanResult.Remove(GC_TestDedicatedMapPlan);
		GC_TestDedicatedResult(requestId, outcome + " bytes=" + output.Length().ToString());
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestDedicatedRows(array<string> rows, int requestId)
	{
		if (requestId != m_iGCDedicatedRequest)
			return;
		m_OnModerationRows.Remove(GC_TestDedicatedRows);
		string result = "rows=" + (rows.Count() / 7).ToString();
		for (int i; i < rows.Count(); i += 7)
			result += string.Format(" player=%1 identity=%2", rows[i], rows[i + 2]);
		GC_TestDedicatedResult(requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	//! Every mutation below calls a production client request or rendered widget handler.
	void GC_TestDedicatedAction(string token, string message)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		array<string> fields = {};
		message.Split("|", fields, false);
		if (!manager || fields.Count() != 2 || !m_sGCDedicatedToken.IsEmpty())
			return;
		string action = fields[0];
		int targetId = fields[1].ToInt();
		m_sGCDedicatedToken = token;
		m_sGCDedicatedBefore = manager.GC_TestAdminScopeSnapshot(targetId);
		m_iGCDedicatedRequest = ReserveAdminPlayerToolRequest();
		array<int> selected = {};
		array<string> identities = {};

		if (action.StartsWith("phase-"))
		{
			SCR_EGameModeState phase = SCR_EGameModeState.SLOTTING;
			if (action == "phase-briefing")
				phase = SCR_EGameModeState.BRIEFING;
			else if (action == "phase-game")
				phase = SCR_EGameModeState.GAME;
			RequestLobbyPhaseAdvance(phase);
			GC_TestDedicatedResult(m_iGCDedicatedRequest, "dispatched-phase");
		}
		else if (action == "group-lock" || action == "group-unlock")
		{
			m_OnAdminGroupLockResult.Insert(GC_TestDedicatedResult);
			RequestAdminGroupLock(RplId.Invalid(), action == "group-lock", m_iGCDedicatedRequest);
		}
		else if (action == "rules-unlock")
		{
			m_OnAdminFactionRulesResult.Insert(GC_TestDedicatedResult);
			RequestAdminFactionRules(manager.GetFactionRulesRevision(), 2, false, m_iGCDedicatedRequest);
		}
		else if (action == "moderation")
		{
			m_OnModerationRows.Insert(GC_TestDedicatedRows);
			RequestModerationSession(m_iGCDedicatedRequest);
		}
		else if (action == "reservation-export")
		{
			m_OnSlotReservationResult.Insert(GC_TestDedicatedReservation);
			RequestSlotReservation("export", -1, "", "", "", "", m_iGCDedicatedRequest);
		}
		else if (action == "map-save" || action == "map-clear")
		{
			m_GC_OnMapPlanResult.Insert(GC_TestDedicatedMapPlan);
			if (action == "map-save")
				GC_RequestMapPlan("save", "$profile:Lobby2MapPlans/dedicated-regression.json", m_iGCDedicatedRequest);
			else
				GC_RequestMapPlan("clear", "", m_iGCDedicatedRequest);
		}
		else if (action == "freeze-start" || action == "freeze-extend" || action == "freeze-end")
		{
			m_iGCDedicatedRequest = -1;
			m_OnAdminModerationResult.Insert(GC_TestDedicatedResult);
			string operation = "start";
			int seconds = 60;
			if (action == "freeze-extend")
			{
				operation = "extend";
				seconds = 30;
			}
			else if (action == "freeze-end")
			{
				operation = "end";
				seconds = 0;
			}
			RequestAdminFreeze(GC_LobbyFreezeComponent.GetInstance().GetRevision(), operation, seconds);
		}
		else if (action == "chat-send" || action == "chat-read")
		{
			string chatAction = "snapshot";
			if (action == "chat-send")
				chatAction = "raw-send";
			GC_TestDedicatedResult(m_iGCDedicatedRequest, manager.GC_TestNativeChat(chatAction, "Admin", "Dedicated admin contact"));
		}
		else if (action == "player-kick" || action == "player-ban" || action == "player-unban" || action == "ban-list")
		{
			m_iGCDedicatedRequest = ReserveAdminActionRequest();
			m_OnAdminModerationResult.Insert(GC_TestDedicatedResult);
			if (action == "ban-list")
				m_OnAdminBanPage.Insert(GC_TestDedicatedBanPage);
			RequestAdminPlayerAction(action, targetId, manager.GetPlayerVoiceIdentity(targetId), "Dedicated regression", 60, 0, m_iGCDedicatedRequest);
		}
		else if (action.StartsWith("message-"))
		{
			m_OnAdminMessageResult.Insert(GC_TestDedicatedResult);
			GC_EAdminMessageAudience audience = GC_EAdminMessageAudience.ALL_PLAYERS;
			if (action == "message-faction")
				audience = GC_EAdminMessageAudience.FACTION;
			else if (action == "message-selected")
			{
				audience = GC_EAdminMessageAudience.SELECTED_PLAYERS;
				identities.Insert(manager.GetPlayerVoiceIdentity(targetId));
			}
			RequestAdminMessage(audience, "Dedicated " + action, identities, m_iGCDedicatedRequest, "USSR");
		}
		else if (action == "ui-everyone" || action == "ui-faction")
		{
			m_iGCDedicatedRequest = -1;
			m_OnAdminPlayerToolResult.Insert(GC_TestDedicatedResult);
			manager.GC_TestAdminScopeUi("open", "");
			manager.GC_TestAdminScopeUi("click", "ToolHeal");
			if (action == "ui-everyone")
				manager.GC_TestAdminScopeUi("click", "ScopeEveryone");
			else
			{
				manager.GC_TestAdminScopeUi("click", "ScopeFaction");
				MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
				GC_AdminToolsComponent panel = GC_AdminToolsComponent.Cast(menu.GetRootWidget().FindHandler(GC_AdminToolsComponent));
				panel.GC_TestDedicatedFaction("USSR");
			}
			manager.GC_TestAdminScopeUi("click", "ToolApply");
		}
		else if (action == "heal-everyone" || action == "heal-nearby" || action == "heal-selected")
		{
			m_OnAdminPlayerToolResult.Insert(GC_TestDedicatedResult);
			GC_EAdminHealScope scope = GC_EAdminHealScope.EVERYONE;
			if (action == "heal-nearby")
				scope = GC_EAdminHealScope.NEARBY;
			else if (action == "heal-selected")
			{
				scope = GC_EAdminHealScope.SELECTED;
				selected.Insert(targetId);
				identities.Insert(manager.GetPlayerVoiceIdentity(targetId));
			}
			RequestAdminHeal(scope, selected, identities, m_iGCDedicatedRequest, 50);
		}
		else if (action == "teleport" || action == "teleport-faction")
		{
			m_OnAdminPlayerToolResult.Insert(GC_TestDedicatedResult);
			if (action == "teleport")
				GC_RequestAdminTeleport(GetPlayerId(), manager.GetPlayerVoiceIdentity(GetPlayerId()), targetId, manager.GetPlayerVoiceIdentity(targetId), m_iGCDedicatedRequest);
			else
				GC_RequestFactionTeleport("US", targetId, manager.GetPlayerVoiceIdentity(targetId), m_iGCDedicatedRequest);
		}
		else
			GC_TestDedicatedResult(m_iGCDedicatedRequest, "unsupported-test-action");
	}
}

modded class GC_AdminToolsComponent
{
	//------------------------------------------------------------------------------------------------
	bool GC_TestDedicatedFaction(string key)
	{
		foreach (SCR_ButtonBaseComponent button, string factionKey : m_mFactionButtons)
		{
			if (key == factionKey)
				return button.OnClick(button.GetRootWidget(), 0, 0, 0);
		}
		return false;
	}
}
