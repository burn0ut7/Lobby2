class GC_SessionLogComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Bounded current-session view plus permanent, append-only server batches.
class GC_SessionLogComponent : SCR_BaseGameModeComponent
{
	protected ref GC_MissionHistory m_History;
	protected ref GC_TelemetryRecorder m_Telemetry;
	protected string m_sSessionId;
	protected int m_iStartedAt;
	protected float m_fStarted;
	protected int m_iSequence;
	protected int m_iRevision;
	protected int m_iDropped;
	protected int m_iRingCursor;
	protected int m_iPendingCount;
	protected string m_sPendingPayload;
	protected bool m_bWriteFailure;
	protected bool m_bClosed;
	protected bool m_bPendingClosed;
	protected ref array<ref GC_SessionEvent> m_aRecent = {};
	protected ref array<ref GC_SessionEvent> m_aQueued = {};
	static const int MAX_RECENT = 4096;
	static const int MAX_QUEUED = 8192;
	static const int BATCH_SIZE = 256;
	static const string OUTBOX = "$profile:Lobby2SessionLogs";

	//------------------------------------------------------------------------------------------------
	static GC_SessionLogComponent GetInstance()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode)
			return null;
		return GC_SessionLogComponent.Cast(mode.FindComponent(GC_SessionLogComponent));
	}

	//------------------------------------------------------------------------------------------------
	string GetSessionId()
	{
		return m_sSessionId;
	}

	//------------------------------------------------------------------------------------------------
	float GetSessionMilliseconds()
	{
		return Math.Max(0, GetGame().GetWorld().GetWorldTime() - m_fStarted * 1000);
	}

	//------------------------------------------------------------------------------------------------
	int GetSessionStartedAt()
	{
		return m_iStartedAt;
	}

	//------------------------------------------------------------------------------------------------
	string GetTelemetryReport()
	{
		if (m_Telemetry)
			return m_Telemetry.Report();
		return "Telemetry disabled; no recording created.";
	}

	//------------------------------------------------------------------------------------------------
	//! Authority entry point; callers must supply the authenticated requesting player.
	bool SetTelemetry(int requester, bool enabled, int quotaMiB = 1024)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !manager.AuthorizeAdminAction(requester, "telemetry", enabled.ToString()))
			return false;
		if (GetInstance() != this || m_bClosed || m_sSessionId.IsEmpty() || quotaMiB < 64 || quotaMiB > 8192)
		{
			GC_AdminActionLogger.Log(requester, "telemetry", enabled.ToString(), "rejected-inactive-session");
			return false;
		}
		if (enabled)
		{
			if (m_Telemetry && !m_Telemetry.IsSealed())
			{
				GC_AdminActionLogger.Log(requester, "telemetry", enabled.ToString(), "rejected-unsealed-recording");
				return false;
			}
			m_Telemetry = new GC_TelemetryRecorder();
			m_Telemetry.Start(this, quotaMiB);
		}
		else if (m_Telemetry)
			m_Telemetry.Stop("disabled");
		GC_AdminActionLogger.Log(requester, "telemetry", enabled.ToString(), "accepted");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (!Replication.IsServer() || GetInstance() != this || !m_sSessionId.IsEmpty())
			return;
		m_sSessionId = PersistenceIdUtils.Generate();
		m_iStartedAt = System.GetUnixTime();
		m_fStarted = world.GetWorldTime() * 0.001;
		m_History = new GC_MissionHistory();
		if (!m_History.Start(m_sSessionId))
		{
			Print("GC | Mission history | unavailable: cannot read/create profile server identity", LogLevel.ERROR);
			m_History = null;
		}
		FileIO.MakeDirectory(OUTBOX);
		GC_SessionEvent entry = CreateEvent("session", 0, 0);
		entry.m_sDetail = "Session opened";
		Record(entry);
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	GC_SessionEvent CreateEvent(string type, int actorId, int targetId)
	{
		GC_SessionEvent entry = new GC_SessionEvent();
		entry.m_sType = type;
		PlayerManager players = GetGame().GetPlayerManager();
		if (actorId > 0)
		{
			entry.m_sActor = SCR_PlayerIdentityUtils.GetPlayerIdentityId(actorId);
			entry.m_sActorName = players.GetPlayerName(actorId);
		}
		if (targetId > 0)
		{
			entry.m_sTarget = SCR_PlayerIdentityUtils.GetPlayerIdentityId(targetId);
			entry.m_sTargetName = players.GetPlayerName(targetId);
		}
		return entry;
	}

	//------------------------------------------------------------------------------------------------
	//! Snapshot event-time identity and world origins before entities can move or disappear.
	int RecordDamage(BaseDamageContext context, IEntity victim, GC_MissionStatsComponent stats, GC_StatsPlayer player = null)
	{
		int actorId;
		IEntity actor;
		if (context.instigator)
		{
			actorId = context.instigator.GetInstigatorPlayerID();
			actor = context.instigator.GetInstigatorEntity();
		}
		GC_SessionEvent entry = CreateEvent("damage", actorId, 0);
		entry.m_sTargetKind = "ai";
		entry.m_sTargetName = "AI";
		entry.m_sTarget = "ai:" + victim.GetID().ToString();
		if (player)
		{
			entry.m_sTargetKind = "player";
			entry.m_sTargetName = player.m_sName;
			entry.m_sTarget = player.m_sIdentity;
		}
		entry.m_sTargetEntity = victim.GetID().ToString();
		vector targetPosition = victim.GetOrigin();
		entry.m_aTargetPosition = {targetPosition[0], targetPosition[1], targetPosition[2]};
		if (victim.GetPrefabData())
			entry.m_sTargetPrefab = victim.GetPrefabData().GetPrefabName();
		entry.m_sActorKind = "other";
		if (context.instigator && context.instigator.GetInstigatorType() == InstigatorType.INSTIGATOR_AI)
		{
			entry.m_sActorKind = "ai";
			entry.m_sActorName = "AI";
			if (actor)
				entry.m_sActor = "ai:" + actor.GetID().ToString();
		}
		else if (actorId > 0)
			entry.m_sActorKind = "player";
		if (actor)
		{
			entry.m_sActorEntity = actor.GetID().ToString();
			vector actorPosition = actor.GetOrigin();
			entry.m_aActorPosition = {actorPosition[0], actorPosition[1], actorPosition[2]};
			entry.m_fDistance = vector.Distance(actorPosition, targetPosition);
			if (actor.GetPrefabData())
				entry.m_sActorPrefab = actor.GetPrefabData().GetPrefabName();
		}
		entry.m_fDamage = context.damageValue;
		entry.m_iDamageType = context.damageType;
		entry.m_sWeapon = stats.GetProjectileWeapon(context.damageSource);
		if (context.damageSource && !context.damageSource.FindComponent(BaseWeaponComponent) && context.damageSource.GetPrefabData())
			entry.m_sProjectile = context.damageSource.GetPrefabData().GetPrefabName();
		Record(entry);
		if (m_Telemetry && entry.m_iSequence > 0)
			m_Telemetry.RecordDamage(context, victim, entry);
		return entry.m_iSequence;
	}

	//------------------------------------------------------------------------------------------------
	//! Every accepted callback gets its own sequence, even within the same engine tick.
	void Record(GC_SessionEvent entry)
	{
		if (!entry || !Replication.IsServer() || GetInstance() != this || m_sSessionId.IsEmpty() || m_bClosed)
			return;
		entry.m_iSequence = ++m_iSequence;
		entry.m_fSeconds = Math.Max(0, GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted);
		if (m_aRecent.Count() < MAX_RECENT)
			m_aRecent.Insert(entry);
		else
		{
			m_aRecent[m_iRingCursor] = entry;
			m_iRingCursor = (m_iRingCursor + 1) % MAX_RECENT;
		}
		if (m_aQueued.Count() < MAX_QUEUED)
			m_aQueued.Insert(entry);
		else
		{
			if (m_iDropped == 0)
				Print("GC | Session logs | persistence queue full; history incomplete", LogLevel.ERROR);
			m_iDropped++;
		}
	}

	//------------------------------------------------------------------------------------------------
	void RecordDeath(GC_SessionEvent entry, IEntity victim, IEntity killer)
	{
		if (victim)
		{
			vector position = victim.GetOrigin();
			entry.m_aTargetPosition = {position[0], position[1], position[2]};
		}
		if (killer && entry.m_sCreditSource != "lastPlayerDamage")
		{
			vector origin = killer.GetOrigin();
			entry.m_aActorPosition = {origin[0], origin[1], origin[2]};
		}
		Record(entry);
		if (m_Telemetry && entry.m_iSequence > 0)
			m_Telemetry.RecordDeath(entry, victim, killer);
	}

	//------------------------------------------------------------------------------------------------
	void RecordShot(string type, int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		GC_SessionEvent entry = CreateEvent(type, playerId, 0);
		if (projectile && projectile.GetPrefabData())
			entry.m_sProjectile = projectile.GetPrefabData().GetPrefabName();
		if (weapon)
		{
			if (weapon.GetOwner() && weapon.GetOwner().GetPrefabData())
				entry.m_sWeapon = weapon.GetOwner().GetPrefabData().GetPrefabName();
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			if (magazine && magazine.GetOwner() && magazine.GetOwner().GetPrefabData())
				entry.m_sMagazine = magazine.GetOwner().GetPrefabData().GetPrefabName();
		}
		Record(entry);
		if (m_Telemetry && entry.m_iSequence > 0)
			m_Telemetry.RecordShot(type, playerId, weapon, projectile, entry.m_iSequence);
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (GetInstance() != this || !Replication.IsServer() || m_bClosed)
			return;
		if (m_History)
			m_History.Tick();
		// Bounded catch-up for bursts; callbacks themselves never perform disk I/O.
		for (int batch; batch < 4 && (!m_aQueued.IsEmpty() || !m_sPendingPayload.IsEmpty()); batch++)
		{
			if (!Flush())
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Retry the identical payload until published; new callbacks remain queued behind it.
	protected bool Flush()
	{
		if (m_sPendingPayload.IsEmpty())
		{
			m_iPendingCount = Math.Min(BATCH_SIZE, m_aQueued.Count());
			JsonSaveContext context = new JsonSaveContext();
			context.WriteValue("kind", "sessionEvents");
			context.WriteValue("schemaVersion", 1);
			context.WriteValue("sessionId", m_sSessionId);
			context.WriteValue("world", GetGame().GetWorldFile());
			context.WriteValue("startedAt", m_iStartedAt);
			context.WriteValue("revision", m_iRevision);
			bool closed = m_bClosed && m_iPendingCount == m_aQueued.Count();
			m_bPendingClosed = closed;
			context.WriteValue("closed", closed);
			context.WriteValue("droppedRecords", m_iDropped);
			context.StartArray("events", m_iPendingCount);
			for (int i; i < m_iPendingCount; i++)
			{
				context.StartObject();
				m_aQueued[i].Write(context);
				context.EndObject();
			}
			context.EndArray();
			if (!context.IsValid())
				return false;
			m_sPendingPayload = context.SaveToString();
			if (m_sPendingPayload.Length() <= 2)
				return false;
		}
		string path = string.Format("%1/%2-events-%3.json", OUTBOX, m_sSessionId, m_iRevision);
		if (!GC_StatsOutbox.WriteVerified(path, m_sPendingPayload) || !GC_StatsOutbox.WriteReady(path + ".ready"))
		{
			if (!m_bWriteFailure)
				PrintFormat("GC | Session logs | write failed path=%1; pending records retained", path, level: LogLevel.ERROR);
			m_bWriteFailure = true;
			return false;
		}
		m_bWriteFailure = false;
		m_iRevision++;
		for (int index; index < m_iPendingCount; index++)
			m_aQueued.RemoveOrdered(0);
		m_sPendingPayload = string.Empty;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Display elapsed mission time with optional milliseconds; hours appear only when needed.
	static string FormatEventTime(float seconds, bool precise = true)
	{
		seconds = Math.Max(0, seconds);
		int wholeSeconds = Math.Floor(seconds);
		int milliseconds = Math.Round((seconds - wholeSeconds) * 1000);
		if (precise && milliseconds == 1000)
		{
			wholeSeconds++;
			milliseconds = 0;
		}
		int hours = wholeSeconds / 3600;
		int minutes = (wholeSeconds / 60) % 60;
		int remainder = wholeSeconds % 60;
		string minuteText = minutes.ToString();
		string secondText = remainder.ToString();
		string millisecondText = milliseconds.ToString();
		if (minutes < 10)
			minuteText = "0" + minuteText;
		if (remainder < 10)
			secondText = "0" + secondText;
		if (milliseconds < 100)
			millisecondText = "0" + millisecondText;
		if (milliseconds < 10)
			millisecondText = "0" + millisecondText;
		string result = minuteText + ":" + secondText;
		if (precise)
			result += ":" + millisecondText;
		if (hours == 0)
			return result;
		string hourText = hours.ToString();
		if (hours < 10)
			hourText = "0" + hourText;
		return hourText + ":" + result;
	}

	//------------------------------------------------------------------------------------------------
	//! Server-filtered bounded page. Cursor excludes that sequence and all newer records.
	void ReadPage(int requester, string query, string type, int before, out array<string> rows, out int next, out string status)
	{
		rows = {};
		next = 0;
		status = "Access denied.";
		if (!Replication.IsServer() || GetInstance() != this || !SCR_Global.IsAdmin(requester))
			return;
		status = "Showing latest events.";
		if (before > 0)
			status = "Showing earlier events.";
		if (query.Length() > 128 || type.Length() > 32 || before < 0)
		{
			status = "Invalid filter.";
			return;
		}
		query.TrimInPlace();
		query.ToLower();
		array<string> identities = {};
		GC_LobbyManager.GetManager().GetAdminLogSearchIdentities(query, identities);
		bool searchType = type.StartsWith("~");
		string eventQuery;
		if (searchType)
		{
			eventQuery = type.Substring(1, type.Length() - 1);
			eventQuery.ToLower();
		}
		int count = m_aRecent.Count();
		int start = count - 1;
		if (count == MAX_RECENT)
			start = (m_iRingCursor + count - 1) % count;
		int matched;
		bool hasMore;
		for (int offset; offset < count; offset++)
		{
			GC_SessionEvent entry = m_aRecent[(start - offset + count) % count];
			if (before > 0 && entry.m_iSequence >= before)
				continue;
			bool death = entry.m_sType == "kill" || entry.m_sType == "aiKill" || entry.m_sType == "teamKill" || entry.m_sType == "death";
			if (!searchType && !type.IsEmpty() && entry.m_sType != type && !(type == "death" && death))
				continue;
			if (searchType)
			{
				string eventTerms = entry.m_sType;
				if (eventTerms == "kill")
					eventTerms = "kill player kills deaths (all)";
				else if (eventTerms == "teamKill")
					eventTerms = "teamKill team kills deaths (all)";
				else if (eventTerms == "aiKill")
					eventTerms = "aiKill AI kills deaths (all)";
				else if (eventTerms == "death")
					eventTerms = "death deaths (all)";
				else if (eventTerms == "slot")
					eventTerms = "slot slotting";
				else
					eventTerms += "s";
				eventTerms.ToLower();
				if (!eventTerms.Contains(eventQuery))
					continue;
			}
			string detail = entry.m_sDetail;
			if (!entry.m_sWeapon.IsEmpty())
				detail += " " + FilePath.StripExtension(FilePath.StripPath(entry.m_sWeapon));
			if (entry.m_sType == "damage")
				detail += string.Format(" | reported damage %1", entry.m_fDamage);
			if (entry.m_fDistance >= 0)
				detail += string.Format(" | %1 m", Math.Round(entry.m_fDistance));
			string eventName = entry.m_sType;
			if (eventName == "teamKill")
				eventName = "Team kill";
			else if (eventName == "aiKill")
				eventName = "AI kill";
			else if (eventName == "slot")
				eventName = "Slotting";
			string search = FormatEventTime(entry.m_fSeconds) + " " + eventName + " " + entry.m_sType
				+ " " + entry.m_sActor + " " + entry.m_sActorName + " " + entry.m_sTarget + " " + entry.m_sTargetName + " " + detail;
			search.ToLower();
			if (!query.IsEmpty() && !search.Contains(query)
				&& !identities.Contains(entry.m_sActor) && !identities.Contains(entry.m_sTarget))
				continue;
			if (matched == 20)
			{
				hasMore = true;
				break;
			}
			rows.Insert(entry.m_iSequence.ToString());
			rows.Insert(FormatEventTime(entry.m_fSeconds));
			rows.Insert(entry.m_sType);
			string people = entry.m_sActorName;
			if (!entry.m_sTargetName.IsEmpty())
				people += " > " + entry.m_sTargetName;
			rows.Insert(people);
			rows.Insert(detail);
			next = entry.m_iSequence;
			matched++;
		}
		if (!hasMore)
			next = 0;
		if (matched == 0)
			status = "No matching events.";
		if (m_iSequence > MAX_RECENT)
			status += " Earlier events are in the server archive.";
		if (m_iDropped > 0 || m_bWriteFailure)
			status += " Server log storage is incomplete or delayed.";
	}

	//------------------------------------------------------------------------------------------------
	//! Spectator-only combat projection. Never exposes identities or administrative events.
	void ReadSpectatorPage(int requester, bool everyone, int before, out array<string> rows, out int next, out string status)
	{
		rows = {};
		next = 0;
		status = "Spectator access required.";
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requester));
		if (!Replication.IsServer() || GetInstance() != this || !controller || controller.GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(requester);
		if (identity.IsEmpty() || before < 0)
		{
			status = "Player identity or cursor unavailable.";
			return;
		}
		status = string.Empty;
		int count = m_aRecent.Count();
		int start = count - 1;
		if (count == MAX_RECENT)
			start = (m_iRingCursor + count - 1) % count;
		// Combine same-tick fatal hits only in this view; retain canonical records unchanged.
		map<string, GC_SessionEvent> lastHits = new map<string, GC_SessionEvent>();
		map<int, GC_SessionEvent> fatalHits = new map<int, GC_SessionEvent>();
		set<int> combinedHits = new set<int>();
		for (int scan = count - 1; scan >= 0; scan--)
		{
			GC_SessionEvent candidate = m_aRecent[(start - scan + count) % count];
			if (candidate.m_sType == "damage")
				lastHits.Set(candidate.m_sTargetEntity, candidate);
			else if (candidate.m_sType == "kill" || candidate.m_sType == "teamKill" || candidate.m_sType == "aiKill" || candidate.m_sType == "death")
			{
				GC_SessionEvent hit;
				if (candidate.m_sDeathCause != "bleeding" && candidate.m_sDeathCause != "respawn"
					&& !candidate.m_sTargetEntity.IsEmpty() && lastHits.Find(candidate.m_sTargetEntity, hit)
					&& candidate.m_sActor == hit.m_sActor && candidate.m_fSeconds == hit.m_fSeconds)
				{
					fatalHits.Set(candidate.m_iSequence, hit);
					combinedHits.Insert(hit.m_iSequence);
				}
			}
		}
		int matched;
		for (int offset; offset < count; offset++)
		{
			GC_SessionEvent entry = m_aRecent[(start - offset + count) % count];
			if ((before > 0 && entry.m_iSequence >= before) || combinedHits.Contains(entry.m_iSequence))
				continue;
			bool damage = entry.m_sType == "damage";
			if (!damage && entry.m_sType != "kill" && entry.m_sType != "teamKill" && entry.m_sType != "aiKill" && entry.m_sType != "death")
				continue;
			if (!everyone && (entry.m_sTargetKind == "ai" || (entry.m_sActor != identity && entry.m_sTarget != identity)))
				continue;
			if (matched == 20)
			{
				next = rows[rows.Count() - 7].ToInt();
				break;
			}
			string actor = entry.m_sActorName;
			string target = entry.m_sTargetName;
			if (actor.IsEmpty())
				actor = "Environment / unknown";
			if (target.IsEmpty())
				target = "Unknown";
			if (entry.m_sActor == identity)
				actor = "You";
			if (entry.m_sTarget == identity)
				target = "You";
			GC_SessionEvent hitEntry = entry;
			bool fatal = fatalHits.Find(entry.m_iSequence, hitEntry);
			if (!fatal)
				hitEntry = entry;
			string eventName = "Killed";
			string detail;
			if (damage || fatal)
			{
				eventName = string.Format("%1 damage", Math.Round(hitEntry.m_fDamage * 10) * 0.1);
				if (fatal)
					eventName += " (killed)";
				detail = SCR_Enum.GetEnumName(EDamageType, hitEntry.m_iDamageType);
				detail.ToLower();
				if (hitEntry.m_iDamageType == EDamageType.KINETIC)
					detail = "bullet";
			}
			if (entry.m_sType == "teamKill")
			{
				if (fatal)
					eventName.Replace("killed", "team kill");
				else
					eventName = "Team kill";
			}
			if (entry.m_sDeathCause == "bleeding")
				eventName = "Bled out";
			else if (entry.m_sDeathCause == "respawn")
				eventName = "Respawn";
			if (!entry.m_sWeapon.IsEmpty())
			{
				if (!detail.IsEmpty())
					detail += " | ";
				string weapon = FilePath.StripExtension(FilePath.StripPath(entry.m_sWeapon));
				weapon.Replace("_", " ");
				detail += weapon;
			}
			if (entry.m_fDistance >= 0)
			{
				if (!detail.IsEmpty())
					detail += " | ";
				detail += string.Format("%1 m", Math.Round(entry.m_fDistance));
			}
			string positionText;
			string location = "Location unavailable";
			if (entry.m_aTargetPosition.Count() == 3)
			{
				vector position = {entry.m_aTargetPosition[0], entry.m_aTargetPosition[1], entry.m_aTargetPosition[2]};
				positionText = position.ToString(false);
				location = string.Format("X %1 / Z %2 m", Math.Round(position[0]), Math.Round(position[2]));
			}
			rows.Insert(entry.m_iSequence.ToString());
			rows.Insert(FormatEventTime(entry.m_fSeconds, false));
			rows.Insert(eventName);
			rows.Insert(actor + " > " + target);
			rows.Insert(detail);
			rows.Insert(location);
			rows.Insert(positionText);
			matched++;
		}
		if (matched == 0)
			status = "No events";
	}

	//------------------------------------------------------------------------------------------------
	void RecordPhase()
	{
		if (Replication.IsServer() && GetInstance() == this && m_History)
			m_History.RecordPhase();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		if (Replication.IsServer() && GetInstance() == this && m_History)
			m_History.EndMission(data);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		if (m_Telemetry)
			m_Telemetry.Close();
		GetGame().GetCallqueue().Remove(Tick);
		if (m_History)
			m_History.Close();
		if (Replication.IsServer() && !m_sSessionId.IsEmpty() && !m_bClosed)
		{
			m_bClosed = true;
			// At most 32 data batches plus a seal after an already encoded retry.
			while (Flush() && !m_bPendingClosed)
			{
			}
		}
		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_SessionLogComponent()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
	}
}
