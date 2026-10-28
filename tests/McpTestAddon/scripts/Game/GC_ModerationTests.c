#ifdef WORKBENCH
modded class GC_AdminModerationComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFilters(string field, string query, int choice)
	{
		if (field == "fixture")
		{
			// Presentation only: never creates a backend ban or changes a player.
			array<string> rows = {};
			for (int index; index < 18; index++)
			{
				rows.Insert("0");
				rows.Insert("Fixture " + index.ToString());
				rows.Insert("fixture-uid-" + index.ToString());
				rows.Insert("76561198000000" + index.ToString());
				rows.Insert("Disconnected");
				rows.Insert("Review note " + index.ToString());
				string expiry;
				if (index == 1)
					expiry = "0";
				if (index == 2)
					expiry = (System.GetUnixTime() + 4).ToString();
				rows.Insert(expiry);
			}
			OnRows(rows, m_iRosterRequest);
		}
		else if (field != "state")
		{
			GC_AdminSearchFilter filter = m_Players;
			if (field == "status")
				filter = m_BanStatus;
			if (query == "FOCUS")
				return filter.GC_TestInteraction("focus");
			EditBoxWidget input = EditBoxWidget.Cast(filter.GetRootWidget().FindAnyWidget("FilterInput"));
			input.SetText(query);
			SCR_EventHandlerComponent.Cast(input.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(input);
			array<Widget> options = {};
			filter.GetElementWidgets(options);
			if (choice >= 0 && choice < options.Count() && options[choice].IsVisible())
				SCR_ButtonTextComponent.Cast(options[choice].FindHandler(SCR_ButtonTextComponent)).OnClick(options[choice], 0, 0, 0);
		}
		string names;
		foreach (SCR_ButtonBaseComponent button, int rowIndex : m_mRows)
			names += m_aRows[rowIndex + 1] + ";";
		return string.Format("rows=%1 names=%2 player=%3 status=%4", m_mRows.Count(), names, m_Players.GetFilterValue(), m_BanStatus.GetFilterValue());
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestModerationFilters(string field, string query, int choice)
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget fields = root.FindAnyWidget("ModerationFields");
		return GC_AdminModerationComponent.Cast(fields.FindHandler(GC_AdminModerationComponent)).GC_TestFilters(field, query, choice);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestModeration(string operation, string value)
	{
		if (operation == "expiry")
		{
			int timestamp, duration;
			bool epoch = GC_AdminBanExpiry.ParseDate("1970-01-01 00:00", timestamp) && timestamp == 0;
			bool leap = GC_AdminBanExpiry.ParseDate("2028-02-29 12:30", timestamp) && timestamp == 1835440200;
			bool invalid = !GC_AdminBanExpiry.ParseDate("2027-02-29 12:30", timestamp);
			bool overflow = !GC_AdminBanExpiry.ParseDate("2038-01-20 00:00", timestamp);
			bool minutes = GC_AdminBanExpiry.ParseDuration("30", 60, duration) && duration == 1800;
			bool zero = !GC_AdminBanExpiry.ParseDuration("0", 60, duration);
			bool huge = !GC_AdminBanExpiry.ParseDuration("999999999", 86400, duration);
			return string.Format("epoch=%1 leap=%2 invalid=%3 overflow=%4 minutes=%5 zero=%6 huge=%7", epoch, leap, invalid, overflow, minutes, zero, huge);
		}
		if (operation == "authority")
		{
			if (RplSession.Mode() == RplMode.Dedicated)
				return "UNSUPPORTED destructive tests are embedded-only";
			int requester = GetGame().GetPlayerController().GetPlayerId();
			string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(requester);
			string blank = ExecuteAdminPlayerAction(requester, "player-kick", requester, identity, "", 0, 0);
			string past = ExecuteAdminPlayerAction(requester, "player-ban", 0, identity, "Test", 0, 0, 0, 1);
			string offline = ExecuteAdminPlayerAction(requester, "player-ban", 0, "12345678-1234-1234-1234-123456789abc", "Test offline ban validation", 60, 0);
			string denied = ExecuteAdminPlayerAction(999999, "player-ban", 0, identity, "Test denied", 60, 0);
			return string.Format("blank=%1 past=%2 offline=%3 denied=%4", blank, past, offline, denied);
		}
		if (operation == "logged-out")
		{
			PlayerManager players = GetGame().GetPlayerManager();
			int requester = GetGame().GetPlayerController().GetPlayerId();
			if (!Replication.IsServer() || !SCR_Global.IsAdmin(requester))
				return "UNAVAILABLE authenticated test host";
			string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(requester);
			players.ClearPlayerRole(requester, EPlayerRole.ADMINISTRATOR);
			players.ClearPlayerRole(requester, EPlayerRole.SESSION_ADMINISTRATOR);
			string kick = ExecuteAdminPlayerAction(requester, "player-kick", requester, identity, "Unauthorized test", 0, 0);
			string ban = ExecuteAdminPlayerAction(requester, "player-ban", 0, identity, "Unauthorized test", 60, 0);
			string priorClipboard = System.ImportFromClipboard();
			string copy = CopyAdminPlayerDetails(requester, identity);
			string unban = ExecuteAdminPlayerAction(requester, "player-unban", 0, identity, "", 0, 0);
			return string.Format("admin=%1 connected=%2 kick=%3 ban=%4 copy=%5 clipboardUnchanged=%6 unban=%7; stop disposable Play to restore host role", SCR_Global.IsAdmin(requester), players.IsPlayerConnected(requester), kick, ban, copy, priorClipboard == System.ImportFromClipboard(), unban);
		}
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		if (operation == "presentation")
		{
			// UI-only session-record fixture. This does not create a backend ban.
			if (!Replication.IsServer() || RplSession.Mode() == RplMode.Dedicated)
				return "UNSUPPORTED embedded presentation fixture only";
			string fixtureIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(GetGame().GetPlayerController().GetPlayerId());
			RecordModerationAction(fixtureIdentity, "Banned", value, System.GetUnixTime() + 3);
		}
		if (operation == "copy-context")
		{
			Widget copyRow = root.FindAnyWidget("ModerationRows").GetChildren();
			if (!copyRow)
				return "UNAVAILABLE row";
			GC_AdminModerationRowComponent rowHandler = GC_AdminModerationRowComponent.Cast(copyRow.FindHandler(GC_AdminModerationRowComponent));
			return "rightClick=" + rowHandler.OnClick(copyRow, 0, 0, 1).ToString();
		}
		if (operation == "copy-action" || operation == "unban-action")
		{
			Widget actions = GetGame().GetWorkspace().FindAnyWidget("ContextActions");
			if (!actions || !actions.GetChildren())
				return "UNAVAILABLE context action";
			Widget copyAction = actions.GetChildren();
			if (operation == "unban-action")
				copyAction = copyAction.GetSibling();
			if (!copyAction)
				return "UNAVAILABLE unban action";
			GC_ContextMenuActionButtonComponent actionHandler = GC_ContextMenuActionButtonComponent.Cast(copyAction.FindHandler(GC_ContextMenuActionButtonComponent));
			if (!actionHandler)
				return "UNAVAILABLE action handler";
			actionHandler.OnClick(copyAction, 0, 0, 0);
			return operation + " dispatched";
		}
		if (operation == "copy-check")
		{
			Widget copiedRow = root.FindAnyWidget("ModerationRows").GetChildren();
			if (!copiedRow)
				return "UNAVAILABLE row";
			string expected = "Name: " + TextWidget.Cast(copiedRow.FindAnyWidget("Cell1")).GetText()
				+ "\nSteam ID: " + TextWidget.Cast(copiedRow.FindAnyWidget("Cell3")).GetText()
				+ "\nArma UID: " + TextWidget.Cast(copiedRow.FindAnyWidget("Cell4")).GetText();
			string clipboard = System.ImportFromClipboard();
			int rawLength = clipboard.Length();
			clipboard.Replace("\r", "");
			return "clipboardMatchesRow=" + (clipboard == expected).ToString()
				+ " rawLength=" + rawLength.ToString() + " expectedLength=" + expected.Length().ToString()
				+ " status=" + TextWidget.Cast(root.FindAnyWidget("ModerationStatus")).GetText();
		}
		if (operation == "presentation-state")
		{
			Widget fixtureRow = root.FindAnyWidget("ModerationRows").GetChildren();
			if (!fixtureRow)
				return "UNAVAILABLE row";
			return "reason=" + EditBoxWidget.Cast(root.FindAnyWidget("ModerationReason")).GetText()
				+ " expiry=" + TextWidget.Cast(fixtureRow.FindAnyWidget("Cell6")).GetText()
				+ " status=" + TextWidget.Cast(fixtureRow.FindAnyWidget("Cell2")).GetText();
		}
		if (operation == "field")
		{
			array<string> parts = {};
			value.Split("=", parts, false);
			if (parts.Count() != 2)
				return "INVALID field=value";
			EditBoxWidget field = EditBoxWidget.Cast(root.FindAnyWidget(parts[0]));
			if (!field)
				return "UNAVAILABLE field";
			field.SetText(parts[1]);
			SCR_EventHandlerComponent.Cast(field.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(field);
		}
		if (operation == "expiry-mode")
		{
			SCR_ComboBoxComponent combo = SCR_ComboBoxComponent.Cast(root.FindAnyWidget("ModerationExpiry").FindHandler(SCR_ComboBoxComponent));
			combo.SetCurrentItem(value.ToInt(), false, true, false);
		}
		if (operation == "choose-expiry")
		{
			Widget dropdown = GetGame().GetWorkspace().FindAnyWidget("ModerationComboList");
			if (!dropdown)
				return "UNAVAILABLE dropdown";
			Widget option = dropdown.FindAnyWidget("Content").GetChildren();
			int index;
			while (option)
			{
				SCR_ButtonTextComponent choice = SCR_ButtonTextComponent.Cast(option.FindHandler(SCR_ButtonTextComponent));
				if (choice)
				{
					if (index == value.ToInt())
					{
						choice.OnClick(option, 0, 0, 0);
						break;
					}
					index++;
				}
				option = option.GetSibling();
			}
		}
		if (operation == "row")
		{
			Widget row = root.FindAnyWidget("ModerationRows").GetChildren();
			if (!row)
				return "UNAVAILABLE row";
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Invoke(button);
		}
		int rowCount;
		Widget currentRow = root.FindAnyWidget("ModerationRows").GetChildren();
		while (currentRow)
		{
			rowCount++;
			currentRow = currentRow.GetSibling();
		}
		return "rows=" + rowCount.ToString() + " kick=" + root.FindAnyWidget("ModerationKick").IsEnabled().ToString()
			+ " status=" + TextWidget.Cast(root.FindAnyWidget("ModerationStatus")).GetText()
			+ " identity=" + EditBoxWidget.Cast(root.FindAnyWidget("ModerationIdentity")).GetText();
	}
}
#endif
