//! Explicit fixtures; never publish synthetic players to the backend or live mission.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestReport(string action, string value)
	{
		GC_Lobby_DebriefMenu menu = GC_Lobby_DebriefMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (action == "contract")
			return GC_MissionStatsComponent.GetInstance().GC_TestReportContract(GetGame().GetPlayerController().GetPlayerId());
		if (!menu)
			return "UNAVAILABLE debrief";
		return menu.GC_TestReportUI(action, value);
	}
}

modded class GC_Lobby_DebriefMenu
{
	string GC_TestReportUI(string action, string value)
	{
		if (action == "search")
		{
			m_Search.SetText(value);
			m_SearchEvents.GetOnChange().Invoke(m_Search);
		}
		else if (action == "sort")
			{
			m_Sort.SetCurrentItem(value.ToInt());
			m_Sort.m_OnChanged.Invoke(m_Sort, value.ToInt());
		}
		else if (action == "filter")
		{
			m_EventFilter.SetCurrentItem(value.ToInt());
			m_EventFilter.m_OnChanged.Invoke(m_EventFilter, value.ToInt());
		}
		else if (action == "error")
		{
			GC_DebriefPage failed = new GC_DebriefPage();
			failed.m_sStatus = "Unavailable";
			OnPage("players||0|0", failed);
		}
		else if (action == "restore")
		{
			m_aPlayers.Clear();
			m_PlayerPages.Clear();
			m_iPlayerNext = 0;
			m_sPlayerStatus = "";
			m_Search.SetText("");
			ShowPlayers();
		}
		else if (action == "fixture")
		{
			m_aPlayers.Clear();
			m_PlayerPages.Clear();
			m_iPlayerNext = -1;
			for (int i; i < 128; i++)
			{
				GC_DebriefPlayerRow player = new GC_DebriefPlayerRow();
				player.m_sId = i.ToString();
				player.m_sName = "Player " + i;
				if (i == 0)
					player.m_sName = "Jim Bob — Long player name for clipping verification";
				player.m_sFaction = "US";
				player.m_aValues = {"21m 4s", "96", "2", "1", i.ToString(), "3", "0", "1", "1", "0", "0", "1234 m", "100 m / 50 m", "2m 8s", "None"};
				m_aPlayers.Insert(player);
			}
			ShowPlayers();
			return "SYNTHETIC UI fixture only; reload restores real participants";
		}
		Widget rows = GetRootWidget().FindAnyWidget("ReportRows");
		int count;
		string first;
		for (Widget row = rows.GetChildren(); row; row = row.GetSibling())
		{
			if (count == 0)
				first = TextWidget.Cast(row.FindAnyWidget("Cell1")).GetText();
			count++;
		}
		return string.Format("view=%1 players=%2 rows=%3 first=%4 combatRecords=%5 playerNext=%6 combatNext=%7 status=%8", m_sView, m_aPlayers.Count(), count, first, m_aCombat.Count() / 8, m_iPlayerNext, m_iCombatNext, TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefStatus")).GetText());
	}
}

modded class GC_MissionStatsComponent
{
	string GC_TestReportContract(int requester)
	{
		array<string> rows;
		int next;
		string status;
		ReadReport(-1, "player", "", 0, 0, rows, next, status);
		if (!rows.IsEmpty())
			return "FAIL unresolved requester";
		if (!IsFinalized())
		{
			ReadReport(requester, "players", "", 0, 0, rows, next, status);
			return string.Format("pre-final rows=%1 status=%2", rows.Count(), status);
		}
		ReadReport(requester, "player", "garbage", 0, 0, rows, next, status);
		if (!rows.IsEmpty() || status != "Player unavailable")
			return "FAIL malformed ordinal";
		GC_MissionStatsBatch original = m_Batch;
		map<string, ref GC_StatsDebrief> originalPersonal = m_mPersonalReports;
		map<string, ref GC_StatsDebrief> originalFaction = m_mFactionReports;
		m_Batch = new GC_MissionStatsBatch();
		m_Batch.m_bFinal = true;
		m_mPersonalReports = new map<string, ref GC_StatsDebrief>();
		m_mFactionReports = new map<string, ref GC_StatsDebrief>();
		for (int i; i < 128; i++)
		{
			GC_StatsPlayer player = new GC_StatsPlayer();
			player.m_sIdentity = "fixture-account-" + i;
			player.m_sName = "Jim Bob — Long name with Unicode é and quote \" test player " + i;
			player.m_iKills = i;
			player.m_iAIKills = 3;
			player.m_iShots = 1234567;
			player.m_fPlaySeconds = 7200;
			GC_StatsAssignment assignment = new GC_StatsAssignment();
			assignment.m_sFaction = "USSR";
			player.m_aAssignments.Insert(assignment);
			m_Batch.m_aPlayers.Insert(player);
		}
		int started = System.GetTickCount();
		BuildDebriefs();
		int buildMs = System.GetTickCount(started);
		int total;
		int pages;
		int maxLength;
		int offset;
		bool valid = true;
		while (offset >= 0)
		{
			ReadReport(requester, "players", "", offset, 0, rows, next, status);
			int length;
			foreach (string field : rows)
			{
				length += field.Length();
				if (field.Contains("fixture-account"))
					valid = false;
			}
			maxLength = Math.Max(maxLength, length);
			total += rows.Count() / 4;
			pages++;
			offset = next;
			if (pages > 40)
			{
				valid = false;
				break;
			}
		}
		m_Batch = original;
		m_mPersonalReports = originalPersonal;
		m_mFactionReports = originalFaction;
		return string.Format("valid=%1 players=%2 pages=%3 maxPayloadCharacters=%4 buildMs=%5 totalMs=%6; original batch restored", valid && total == 128 && pages == 8, total, pages, maxLength, buildMs, System.GetTickCount(started));
	}
}
modded class GC_SessionLogComponent
{
	string GC_TestReportEvents()
	{
		string identity = "debrief-retention-fixture";
		GC_SpectatorEventBuffer original = m_mPersonalEvents.Get(identity);
		GC_SpectatorEventBuffer fixture = new GC_SpectatorEventBuffer(256);
		for (int i; i < 260; i++)
		{
			GC_SessionEvent entry = new GC_SessionEvent();
			entry.m_iSequence = i + 1;
			entry.m_fSeconds = i;
			entry.m_sType = "damage";
			entry.m_sActor = identity;
			entry.m_sActorName = "Jim Bob";
			entry.m_sActorKind = "player";
			entry.m_sTarget = "fixture-other";
			entry.m_sTargetName = "Other";
			entry.m_sTargetKind = "ai";
			if (i % 4 == 0 || i % 4 == 3)
			{
				entry.m_sActor = "fixture-other";
				entry.m_sTarget = identity;
			}
			if (i % 4 == 2)
				entry.m_sType = "aiKill";
			if (i % 4 == 3)
				entry.m_sType = "death";
			fixture.Insert(entry);
		}
		m_mPersonalEvents.Set(identity, fixture);
		string result;
		bool valid = true;
		for (int filter; filter < 5; filter++)
		{
			int cursor = 0;
			int count = 0;
			int pages = 0;
			while (cursor >= 0 && pages < 50)
			{
				array<string> rows;
				int next;
				string status;
				ReadDebriefCombat(identity, cursor, filter, rows, next, status);
				count += rows.Count() / 8;
				if (rows.Count() > 48 || status != "Recent 256 events" || (next >= 0 && cursor > 0 && next >= cursor))
					valid = false;
				cursor = next;
				pages++;
			}
			int expected = 64;
			if (filter == 0)
				expected = 256;
			if (count != expected)
				valid = false;
			result += string.Format("filter%1=%2/%3pages ", filter, count, pages);
		}
		if (original)
			m_mPersonalEvents.Set(identity, original);
		else
			m_mPersonalEvents.Remove(identity);
		return "valid=" + valid.ToString() + " " + result;
	}
}
#endif
