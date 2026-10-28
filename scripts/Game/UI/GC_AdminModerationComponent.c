//! One identity-bound moderation form; authority and persistence remain on the server.
class GC_AdminModerationComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_AdminSearchFilter m_Players;
	protected GC_AdminSearchFilter m_BanStatus;
	protected EditBoxWidget m_Identity;
	protected EditBoxWidget m_Reason;
	protected EditBoxWidget m_Amount;
	protected SCR_ComboBoxComponent m_Expiry;
	protected TextWidget m_Status;
	protected ref array<string> m_aRows = {};
	protected ref map<SCR_ButtonBaseComponent, int> m_mRows = new map<SCR_ButtonBaseComponent, int>();
	protected int m_iSelected = -1;
	protected int m_iPending = -1;
	protected int m_iRosterRequest;
	protected ref array<SCR_EventHandlerComponent> m_aEditEvents = {};

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Players = GC_AdminSearchFilter.Cast(w.FindAnyWidget("ModerationPlayers").FindHandler(GC_AdminSearchFilter));
		m_BanStatus = GC_AdminSearchFilter.Cast(w.FindAnyWidget("ModerationBanStatus").FindHandler(GC_AdminSearchFilter));
		EditBoxWidget.Cast(m_Players.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("All players");
		EditBoxWidget.Cast(m_BanStatus.GetRootWidget().FindAnyWidget("FilterInput")).SetPlaceholderText("Any ban status");
		m_Players.SetChoices({"All players"}, {""}, {""});
		m_BanStatus.SetChoices({"Any ban status", "Banned", "Not banned"}, {"", "Banned", "Not banned"}, {"", "", ""});
		m_Players.m_OnFilterChanged.Insert(OnFilterChanged);
		m_BanStatus.m_OnFilterChanged.Insert(OnFilterChanged);
		m_Identity = EditBoxWidget.Cast(w.FindAnyWidget("ModerationIdentity"));
		m_Reason = EditBoxWidget.Cast(w.FindAnyWidget("ModerationReason"));
		m_Amount = EditBoxWidget.Cast(w.FindAnyWidget("ModerationAmount"));
		m_Status = TextWidget.Cast(w.FindAnyWidget("ModerationStatus"));
		m_Expiry = SCR_ComboBoxComponent.Cast(w.FindAnyWidget("ModerationExpiry").FindHandler(SCR_ComboBoxComponent));
		array<string> choices = {"Permanent", "Minutes", "Hours", "Days", "Until date (UTC)"};
		foreach (string choice : choices)
			m_Expiry.AddItem(choice);
		m_Expiry.SetCurrentItem(1, false, false, false);
		m_Expiry.m_OnChanged.Insert(OnExpiry);
		SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("ModerationExpiry").FindAnyWidget("ComboButton").FindHandler(SCR_ButtonBaseComponent)).m_OnClicked.Insert(OnOpenExpiry);
		m_Amount.SetText("30");
		array<EditBoxWidget> edits = {m_Identity, m_Reason, m_Amount};
		foreach (EditBoxWidget edit : edits)
		{
			SCR_EventHandlerComponent events = SCR_EventHandlerComponent.Cast(edit.FindHandler(SCR_EventHandlerComponent));
			events.GetOnChange().Insert(OnEdit);
			m_aEditEvents.Insert(events);
		}
		Bind("ModerationKick", "Kick player");
		Bind("ModerationBan", "Ban player");
		if (m_Controller)
		{
			m_Controller.m_OnModerationRows.Insert(OnRows);
			m_Controller.m_OnModerationChanged.Insert(OnDataChanged);
			m_Controller.m_OnAdminModerationResult.Insert(OnResult);
			m_Controller.m_OnPlayerDetailsCopied.Insert(OnCopyResult);
		}
		Refresh();
		GetGame().GetCallqueue().CallLater(UpdateExpiries, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
		button.SetText(label);
		button.m_OnClicked.Insert(OnAction);
	}

	//------------------------------------------------------------------------------------------------
	bool IsBusy()
	{
		return m_iPending >= 0;
	}

	//------------------------------------------------------------------------------------------------
	void Open()
	{
		if (!IsBusy())
			LoadRoster();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Controller && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadRoster()
	{
		if (!HasAccess() || IsBusy())
			return;
		m_iRosterRequest++;
		m_Controller.RequestModerationSession(m_iRosterRequest);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDataChanged()
	{
		if (!HasAccess() || IsBusy() || !m_wRoot.IsVisible())
			return;
		GetGame().GetCallqueue().Remove(LoadRoster);
		GetGame().GetCallqueue().CallLater(LoadRoster, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRows(array<string> rows, int requestId)
	{
		if (!HasAccess() || requestId != m_iRosterRequest || rows.Count() % 7 != 0)
			return;
		bool hadSelection = m_iSelected >= 0;
		string selectedIdentity = m_Identity.GetText();
		m_aRows.Copy(rows);
		array<string> labels = {"All players"};
		array<string> values = {""};
		array<string> terms = {""};
		for (int playerIndex; playerIndex < rows.Count(); playerIndex += 7)
		{
			labels.Insert(rows[playerIndex + 1]);
			values.Insert(rows[playerIndex + 2]);
			terms.Insert(rows[playerIndex + 2] + " " + rows[playerIndex + 3] + " " + rows[playerIndex + 4]
				+ " " + rows[playerIndex + 5] + " " + ExpiryText(rows[playerIndex + 6]));
		}
		m_Players.SetChoices(labels, values, terms);
		m_iSelected = -1;
		if (hadSelection)
		{
			for (int index; index < rows.Count(); index += 7)
			{
				if (rows[index + 2] == selectedIdentity)
					m_iSelected = index;
			}
		}
		RebuildRows();
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildRows()
	{
		Widget root = m_wRoot.FindAnyWidget("ModerationRows");
		m_mRows.Clear();
		while (root.GetChildren())
			root.GetChildren().RemoveFromHierarchy();
		string query = m_Players.GetFilterValue();
		if (m_Players.HasChoice())
			query = "";
		query.TrimInPlace();
		query.ToLower();
		for (int index; index < m_aRows.Count(); index += 7)
		{
			if (!MatchesQuery(index, query))
				continue;
			Widget row = GetGame().GetWorkspace().CreateWidgets("{6A5A000000000071}UI/layouts/Admin/Lobby_AdminModerationRow.layout", root);
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Insert(OnRow);
			button.SetToggled(index == m_iSelected, false, false);
			m_mRows.Set(button, index);
			TextWidget.Cast(row.FindAnyWidget("Cell1")).SetText(m_aRows[index + 1]);
			TextWidget.Cast(row.FindAnyWidget("Cell2")).SetText(m_aRows[index + 4]);
			string steam = m_aRows[index + 3];
			if (steam.IsEmpty())
				steam = "Unavailable";
			TextWidget.Cast(row.FindAnyWidget("Cell3")).SetText(steam);
			TextWidget.Cast(row.FindAnyWidget("Cell4")).SetText(m_aRows[index + 2]);
			TextWidget.Cast(row.FindAnyWidget("Cell5")).SetText(m_aRows[index + 5]);
		}
		m_wRoot.FindAnyWidget("ModerationEmpty").SetVisible(m_mRows.Count() == 0);
		UpdateExpiries();
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesQuery(int index, string query)
	{
		string player = m_Players.GetFilterValue();
		if (m_Players.HasChoice() && !player.IsEmpty())
		{
			if (m_aRows[index + 2] != player)
				return false;
		}

		// This roster reports session-observed bans, not the backend's complete ban list.
		string expiry = m_aRows[index + 6];
		bool banned = !expiry.IsEmpty() && (expiry == "0" || expiry.ToInt() > System.GetUnixTime());
		string status = "Not banned";
		if (banned)
			status = "Banned";
		string banFilter = m_BanStatus.GetFilterValue();
		if (m_BanStatus.HasChoice() && !banFilter.IsEmpty() && status != banFilter)
			return false;
		string search = m_aRows[index + 1] + " " + m_aRows[index + 2] + " " + m_aRows[index + 3] + " " + m_aRows[index + 4] + " " + m_aRows[index + 5] + " " + ExpiryText(m_aRows[index + 6]);
		if (m_aRows[index + 3].IsEmpty())
			search += " Unavailable";
		if (ExpiryText(m_aRows[index + 6]) == "Expired")
			search += " Ban expired (session record)";
		search.ToLower();
		banFilter.ToLower();
		return search.Contains(query) && (m_BanStatus.HasChoice() || search.Contains(banFilter));
	}

	//------------------------------------------------------------------------------------------------
	protected string ExpiryText(string value)
	{
		if (value == "0")
			return "Permanent";
		if (value.IsEmpty())
			return string.Empty;
		int remaining = value.ToInt() - System.GetUnixTime();
		if (remaining <= 0)
			return "Expired";
		int minutes = (remaining + 59) / 60;
		return minutes.ToString() + " min";
	}

	//------------------------------------------------------------------------------------------------
	//! Presentation clock only; roster changes still arrive through server events.
	protected void UpdateExpiries()
	{
		if (!m_wRoot || !m_wRoot.IsVisible())
			return;
		string query = m_Players.GetFilterValue();
		if (m_Players.HasChoice())
			query = "";
		query.TrimInPlace();
		query.ToLower();
		if (!query.IsEmpty() || !m_BanStatus.GetFilterValue().IsEmpty() || !m_Players.GetFilterValue().IsEmpty())
		{
			array<int> visible = {};
			foreach (SCR_ButtonBaseComponent rowButton, int rowIndex : m_mRows)
				visible.Insert(rowIndex);
			for (int rowIndex; rowIndex < m_aRows.Count(); rowIndex += 7)
			{
				if (MatchesQuery(rowIndex, query) == visible.Contains(rowIndex))
					continue;
				RebuildRows();
				return;
			}
		}
		foreach (SCR_ButtonBaseComponent button, int index : m_mRows)
		{
			string expiry = ExpiryText(m_aRows[index + 6]);
			if (expiry == "Expired")
				TextWidget.Cast(button.GetRootWidget().FindAnyWidget("Cell2")).SetText("Ban expired (session record)");
			TextWidget.Cast(button.GetRootWidget().FindAnyWidget("Cell6")).SetText(expiry);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRow(SCR_ButtonBaseComponent button)
	{
		int selected;
		if (!HasAccess() || IsBusy() || !m_mRows.Find(button, selected))
			return;
		m_Identity.SetText(m_aRows[selected + 2]);
		// The native edit field exposes the full saved note, including clipped table text.
		m_Reason.SetText(m_aRows[selected + 5]);
		m_iSelected = selected;
		m_Status.SetText("");
		RebuildRows();
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	bool OpenPlayerContext(SCR_ButtonBaseComponent button)
	{
		int index;
		if (!HasAccess() || IsBusy() || !m_mRows.Find(button, index))
			return false;
		return GC_ContextMenuComponent.Open(
			GC_ContextMenuContext.ForModerationPlayer(m_aRows[index + 2], m_aRows[index + 1], this), button.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCopyResult(bool copied)
	{
		if (IsBusy())
			return;
		if (copied)
			m_Status.SetText("Player details copied to clipboard.");
		else
			m_Status.SetText("Copy requested. Paste to verify the player details.");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFilterChanged()
	{
		if (IsBusy())
			return;
		RebuildRows();
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEdit(Widget w)
	{
		if (IsBusy())
			return;
		if (w == m_Identity)
			m_iSelected = -1;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnExpiry(SCR_SelectionWidgetComponent selection, int index)
	{
		if (index == 4)
			m_Amount.SetText("");
		else
			m_Amount.SetText("30");
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenExpiry(SCR_ButtonBaseComponent button)
	{
		if (HasAccess() && !IsBusy())
			m_Expiry.OpenList();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		m_wRoot.FindAnyWidget("ModerationForm").SetEnabled(!IsBusy());
		m_Identity.SetEnabled(!IsBusy());
		m_Amount.SetVisible(m_Expiry.GetCurrentIndex() != 0);
		m_wRoot.FindAnyWidget("ModerationAmountLabel").SetVisible(m_Expiry.GetCurrentIndex() != 0);
		m_wRoot.FindAnyWidget("ModerationAmountBackground").SetVisible(m_Expiry.GetCurrentIndex() != 0);
		m_Amount.SetPlaceholderText("Amount");
		if (m_Expiry.GetCurrentIndex() == 4)
			m_Amount.SetPlaceholderText("YYYY-MM-DD HH:MM (UTC)");
		int playerId;
		if (m_iSelected >= 0 && m_iSelected + 6 < m_aRows.Count())
			playerId = m_aRows[m_iSelected].ToInt();
		m_wRoot.FindAnyWidget("ModerationKick").SetEnabled(!IsBusy() && playerId > 0);
		m_wRoot.FindAnyWidget("ModerationBan").SetEnabled(!IsBusy());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || IsBusy())
			return;
		string name = button.GetRootWidget().GetName();
		if (name != "ModerationKick" && name != "ModerationBan")
			return;
		string identity = m_Identity.GetText();
		identity.TrimInPlace();
		string reason = m_Reason.GetText();
		reason.TrimInPlace();
		if (identity.Length() != 36)
		{
			m_Status.SetText("Enter an Arma UID, or search a known Steam ID and select its row. Unknown Steam IDs cannot be resolved here.");
			return;
		}
		if (reason.IsEmpty() || reason.Length() > 256)
		{
			m_Status.SetText("Enter a reason / note (1-256 characters).");
			return;
		}
		int playerId;
		if (m_iSelected >= 0 && m_aRows[m_iSelected + 2] == identity)
			playerId = m_aRows[m_iSelected].ToInt();
		int duration;
		int expiresAt;
		string action;
		if (name == "ModerationBan")
		{
			int mode = m_Expiry.GetCurrentIndex();
			if (mode == 4)
			{
				if (!GC_AdminBanExpiry.ParseDate(m_Amount.GetText(), expiresAt) || expiresAt <= System.GetUnixTime())
				{
					m_Status.SetText("Enter a future UTC date: YYYY-MM-DD HH:MM, through 2037.");
					return;
				}
			}
			else if (mode > 0)
			{
				array<int> units = {0, 60, 3600, 86400};
				if (!GC_AdminBanExpiry.ParseDuration(m_Amount.GetText(), units[mode], duration))
				{
					m_Status.SetText("Enter a positive whole-number duration, up to 10 years.");
					return;
				}
			}
			action = "player-ban";
		}
		else
		{
			if (playerId <= 0)
				return;
			action = "player-kick";
		}
		Submit(action, playerId, identity, reason, duration, expiresAt);
	}

	//------------------------------------------------------------------------------------------------
	void RemoveBan(UUID identity)
	{
		if (HasAccess() && !IsBusy())
			Submit("player-unban", 0, identity, "", 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Submit(string action, int playerId, UUID identity, string reason, int duration, int expiresAt)
	{
		m_iPending = m_Controller.ReserveAdminActionRequest();
		m_Status.SetText("Waiting for the server...");
		Refresh();
		m_Controller.RequestAdminPlayerAction(action, playerId, identity, reason, duration, 0, m_iPending, expiresAt);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome)
	{
		if (requestId != m_iPending)
			return;
		if (!outcome.StartsWith("pending-"))
			m_iPending = -1;
		m_Status.SetText(GC_AdminActionResultText.Get(outcome));
		Refresh();
		if (!IsBusy())
			OnDataChanged();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(LoadRoster);
		GetGame().GetCallqueue().Remove(UpdateExpiries);
		foreach (SCR_EventHandlerComponent events : m_aEditEvents)
		{
			// Child handlers can already be destroyed when the menu closes.
			if (events)
				events.GetOnChange().Remove(OnEdit);
		}
		if (m_Players)
			m_Players.m_OnFilterChanged.Remove(OnFilterChanged);
		if (m_BanStatus)
			m_BanStatus.m_OnFilterChanged.Remove(OnFilterChanged);
		if (m_Expiry)
			m_Expiry.m_OnChanged.Remove(OnExpiry);
		if (m_Controller)
		{
			m_Controller.m_OnModerationRows.Remove(OnRows);
			m_Controller.m_OnModerationChanged.Remove(OnDataChanged);
			m_Controller.m_OnAdminModerationResult.Remove(OnResult);
			m_Controller.m_OnPlayerDetailsCopied.Remove(OnCopyResult);
		}
		super.HandlerDeattached(w);
	}
}
