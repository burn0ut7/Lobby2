//! Read-only projections of the final backend batch, delivered through one paced queue.
modded class GC_MissionStatsComponent
{
	protected ref array<GC_LobbyPlayerController> m_aGC_ReportQueue = {};
	protected bool m_bGC_ReportPump;
	static const int GC_REPORTS_PER_TICK = 8;
	static const int GC_REPORT_PAYLOAD_LIMIT = 3800;
	static const int GC_REPORT_TICK_BUDGET = 8000;

	//------------------------------------------------------------------------------------------------
	void QueueReport(GC_LobbyPlayerController controller)
	{
		if (!Replication.IsServer() || GetInstance() != this || !controller)
			return;
		if (m_aGC_ReportQueue.Find(controller) < 0)
			m_aGC_ReportQueue.Insert(controller);
		if (!m_bGC_ReportPump)
		{
			m_bGC_ReportPump = true;
			GetGame().GetCallqueue().CallLater(PumpReports, 50);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PumpReports()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || GetInstance() != this || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME)
		{
			m_bGC_ReportPump = false;
			m_aGC_ReportQueue.Clear();
			return;
		}
		if (IsFinalized())
		{
			int remaining = GC_REPORT_TICK_BUDGET;
			for (int i; i < GC_REPORTS_PER_TICK && remaining >= GC_REPORT_PAYLOAD_LIMIT && !m_aGC_ReportQueue.IsEmpty(); i++)
			{
				GC_LobbyPlayerController controller = m_aGC_ReportQueue[0];
				m_aGC_ReportQueue.RemoveOrdered(0);
				if (controller && GetGame().GetPlayerManager().GetPlayerController(controller.GetPlayerId()) == controller)
					remaining -= controller.GC_DeliverReport(this);
			}
		}
		if (!m_aGC_ReportQueue.IsEmpty())
		{
			m_bGC_ReportPump = true;
			GetGame().GetCallqueue().CallLater(PumpReports, 50);
		}
		else
			m_bGC_ReportPump = false;
	}

	//------------------------------------------------------------------------------------------------
	//! No caller-provided account identity is accepted. Ordinals are fixed by the sealed batch.
	void ReadReport(int requester, string kind, string subject, int cursor, int filter, out array<string> rows, out int next, out string status)
	{
		rows = {};
		next = -1;
		status = "Unavailable";
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || GetInstance() != this || !IsFinalized() || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME)
			return;
		if (!GetGame().GetPlayerManager().GetPlayerController(requester) || cursor < 0)
			return;
		status = string.Empty;
		if (kind == "players")
		{
			if (cursor > m_Batch.m_aPlayers.Count())
			{
				status = "Page unavailable";
				return;
			}
			int until = Math.Min(cursor + 16, m_Batch.m_aPlayers.Count());
			int payloadSize = 128;
			for (int i = cursor; i < until; i++)
			{
				GC_StatsPlayer player = m_Batch.m_aPlayers[i];
				GC_StatsDebrief cached = m_mPersonalReports.Get(player.m_sIdentity);
				string name = cached.GetPlayerName();
				string factions = cached.GetFactions();
				int rowSize = name.Length() + factions.Length() + cached.GetSummary().Length() + 40;
				if (payloadSize + rowSize > GC_REPORT_PAYLOAD_LIMIT)
				{
					until = i;
					break;
				}
				payloadSize += rowSize;
				rows.Insert(i.ToString());
				rows.Insert(name);
				rows.Insert(factions);
				rows.Insert(cached.GetSummary());
			}
			if (until < m_Batch.m_aPlayers.Count())
				next = until;
			return;
		}
		if (kind == "missionCombat")
		{
			if (!subject.IsEmpty() || filter != 0)
			{
				status = "Unavailable";
				return;
			}
			GC_SessionLogComponent missionEvents = GC_SessionLogComponent.GetInstance();
			if (missionEvents)
				missionEvents.ReadDebriefCombat("", cursor, 0, rows, next, status, true);
			else
				status = "Combat log unavailable";
			return;
		}
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(requester);
		if (kind == "player" || kind == "combat")
		{
			if (!subject.IsEmpty())
			{
				int ordinal = GC_DebriefText.Ordinal(subject);
				if (ordinal < 0 || ordinal >= m_Batch.m_aPlayers.Count() || ordinal.ToString() != subject)
				{
					status = "Player unavailable";
					return;
				}
				identity = m_Batch.m_aPlayers[ordinal].m_sIdentity;
			}
			if (kind == "combat")
			{
				GC_SessionLogComponent events = GC_SessionLogComponent.GetInstance();
				if (events)
					events.ReadDebriefCombat(identity, cursor, filter, rows, next, status);
				else
					status = "Combat log unavailable";
				return;
			}
		}
		GC_StatsDebrief report;
		if (kind == "all")
			report = m_AllReport;
		else if (kind == "faction")
			report = m_mFactionReports.Get(subject);
		else if (kind == "player")
			report = m_mPersonalReports.Get(identity);
		if (!report)
		{
			status = "No statistics";
			return;
		}
		rows.Insert(report.GetSummary());
		rows.Insert(report.GetAmmunition());
		if (!IsHistoryComplete() || ((kind == "all" || kind == "faction") && m_AIStats.IsPartial()))
			status = "Partial report";
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve event-time affiliation from the backend's existing membership intervals.
	string GetReportFactionAt(string identity, float worldSeconds)
	{
		GC_StatsPlayer player = m_mPlayers.Get(identity);
		if (!player)
			return string.Empty;
		float seconds = worldSeconds - m_fStarted;
		for (int i = player.m_aAssignments.Count() - 1; i >= 0; i--)
		{
			GC_StatsAssignment assignment = player.m_aAssignments[i];
			if (seconds >= assignment.m_fFrom && seconds <= assignment.m_fUntil)
				return GC_DebriefText.Short(assignment.m_sFaction);
		}
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MissionStatsComponent()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(PumpReports);
	}
}

//! Presentation limits apply before RPC serialization; backend records remain untouched.
class GC_DebriefText
{
	protected static ref map<string, string> s_mRoles = new map<string, string>();

	static int Ordinal(string value)
	{
		if (value.IsEmpty() || value.Length() > 6)
			return -1;
		for (int i; i < value.Length(); i++)
		{
			if ("0123456789".IndexOf(value.Substring(i, 1)) < 0)
				return -1;
		}
		return value.ToInt();
	}

	static string Short(string value, int limit = 80)
	{
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		value.Replace("\t", " ");
		if (value.Length() > limit)
			return value.Substring(0, limit - 3) + "...";
		return value;
	}

	static string Weapon(GC_SessionEvent entry)
	{
		string resource = entry.m_sWeapon;
		if (resource.IsEmpty())
			resource = entry.m_sProjectile;
		string label = FilePath.StripExtension(FilePath.StripPath(resource));
		label.Replace("_", " ");
		return Short(label, 64);
	}

	static string CombatDetail(GC_SessionEvent entry)
	{
		string detail = Weapon(entry);
		if (detail.IsEmpty())
			detail = "Unknown source";
		if (entry.m_fDistance >= 0)
			detail += string.Format(" · %1 m", Math.Round(entry.m_fDistance));
		string cause;
		if (entry.m_sDeathCause == "bleeding")
			cause = "Bled to death";
		else if (entry.m_sDeathCause == "respawn")
			cause = "Respawned";
		else if (entry.m_sDeathCause == "suicide")
			cause = "Suicide";
		if (!cause.IsEmpty())
			detail = cause;
		GC_SessionEvent hit = entry.m_LastDamage;
		if (hit && (!cause.IsEmpty() || entry.m_iDamageEventSequence != hit.m_iSequence))
		{
			string source = Weapon(hit);
			if (source.IsEmpty())
				source = "Unknown source";
			detail += " · Last hit: " + Short(Actor(hit.m_sActorName, hit.m_sActorKind, hit.m_sActorPrefab), 48) + " / " + source;
		}
		if (!cause.IsEmpty() && !hit)
			detail += " · Last hit unavailable";
		return Short(detail, 240);
	}

	static string Actor(string name, string kind, string prefab)
	{
		if (kind == "ai")
		{
			string cached;
			if (s_mRoles.Find(prefab, cached))
				return cached;
			string role = FilePath.StripExtension(FilePath.StripPath(prefab));
			role.Replace("Character_", "");
			role.Replace("_", " ");
			if (!prefab.IsEmpty())
			{
				SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(prefab);
				if (info && !info.GetName().IsEmpty())
					role = WidgetManager.Translate(info.GetName());
			}
			if (!role.IsEmpty())
			{
				cached = Short(role);
				if (s_mRoles.Count() < 256)
					s_mRoles.Set(prefab, cached);
				return Short(cached);
			}
			return "AI";
		}
		if (name.IsEmpty())
			return "Environment / unknown";
		return Short(name);
	}
}
