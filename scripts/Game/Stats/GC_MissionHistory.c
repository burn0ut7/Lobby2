//! Session-log-owned cumulative mission record, sharing its ID with player statistics.
class GC_MissionHistory
{
	protected string m_sMissionId;
	protected string m_sServerId;
	protected string m_sServerName;
	protected string m_sMissionName;
	protected string m_sAuthor;
	protected string m_sDescription;
	protected string m_sWorld;
	protected string m_sScenarioPath;
	protected string m_sHostKind;
	protected string m_sStatus = "pregame";
	protected string m_sOutcome = "unknown";
	protected int m_iEndReason = -1;
	protected int m_iStartedAt;
	protected int m_iEndedAt;
	protected int m_iRevision;
	protected float m_fStarted;
	protected float m_fObserved;
	protected float m_fLastSample = -10;
	protected float m_fLastFlush = -30;
	protected bool m_bFinished;
	protected bool m_bClosed;
	protected bool m_bComplete = true;
	protected bool m_bWriteFailure;
	protected string m_sPending;
	protected ref array<ref GC_MissionFactionRecord> m_aFactions = {};
	protected ref array<string> m_aResultFactions = {};
	protected ref array<string> m_aWinningFactions = {};
	protected ref array<ref GC_MissionPhaseRecord> m_aPhases = {};
	protected ref array<ref GC_MissionCommandRecord> m_aCommand = {};
	protected ref GC_MissionPerformance m_SessionPerformance = new GC_MissionPerformance();
	protected ref GC_MissionPerformance m_GamePerformance = new GC_MissionPerformance();
	static const string OUTBOX = "$profile:Lobby2History";

	//------------------------------------------------------------------------------------------------
	bool Start(string missionId)
	{
		FileIO.MakeDirectory(OUTBOX);
		string identityPath = OUTBOX + "/server-id.txt";
		if (FileIO.FileExists(identityPath))
		{
			FileHandle identity = FileIO.OpenFile(identityPath, FileMode.READ);
			if (!identity)
				return false;
			identity.Read(m_sServerId, 64);
			identity.Close();
			if (m_sServerId.Length() != 36)
				return false;
		}
		else
		{
			m_sServerId = PersistenceIdUtils.Generate();
			if (!GC_StatsOutbox.WriteVerified(identityPath, m_sServerId))
				return false;
		}
		m_sMissionId = missionId;
		m_iStartedAt = System.GetUnixTime();
		m_fStarted = GetGame().GetWorld().GetWorldTime() * 0.001;
		m_sWorld = GetGame().GetWorldFile();
		m_sHostKind = "listen";
		if (RplSession.Mode() == RplMode.Dedicated)
			m_sHostKind = "dedicated";
#ifdef WORKBENCH
		m_sHostKind = "workbench";
#endif
		ServerConfig server = ServerLobbyApi.GetServerConfig();
		if (server)
			m_sServerName = server.GetName();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (header)
		{
			m_sMissionName = header.m_sName;
			m_sAuthor = header.m_sAuthor;
			m_sDescription = header.m_sDescription;
			m_sScenarioPath = header.m_sPath;
		}
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			GC_MissionFactionRecord record = new GC_MissionFactionRecord();
			record.m_sKey = faction.GetFactionKey();
			record.m_sName = faction.GetFactionName();
			m_aFactions.Insert(record);
		}
		RecordPhase();
		Tick();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void RecordPhase()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !Replication.IsServer() || m_sMissionId.IsEmpty() || m_bClosed)
			return;
		string phase = GC_LobbyManager.GetPhaseName(mode.GetState());
		int count = m_aPhases.Count();
		if (count > 0 && m_aPhases[count - 1].m_sPhase == phase)
			return;
		if (count >= 16)
		{
			m_bComplete = false;
			return;
		}
		GC_MissionPhaseRecord record = new GC_MissionPhaseRecord();
		record.m_sPhase = phase;
		record.m_fSeconds = GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted;
		record.m_iAt = System.GetUnixTime();
		m_aPhases.Insert(record);
		m_fObserved = Math.Max(m_fObserved, record.m_fSeconds);
		if (!m_bFinished && mode.GetState() == SCR_EGameModeState.GAME)
			m_sStatus = "running";
		SampleCommand(record.m_fSeconds);
	}

	//------------------------------------------------------------------------------------------------
	void Tick()
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode() || m_bClosed)
			return;
		float seconds = GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted;
		m_fObserved = seconds;
		if (!m_bFinished)
		{
			SampleCommand(seconds);
			if (seconds - m_fLastSample >= 10)
			{
				m_fLastSample = seconds;
				array<int> players = {};
				GetGame().GetPlayerManager().GetPlayers(players);
				int activeAI;
				if (GetGame().GetAIWorld())
					activeAI = Math.Max(0, GetGame().GetAIWorld().GetCurrentNumOfActiveAIs());
				float fps = Math.Max(0, System.GetFPS());
				m_SessionPerformance.Sample(seconds, fps, players.Count(), activeAI);
				if (GC_LobbyGameMode.GetGameMode().GetState() == SCR_EGameModeState.GAME)
					m_GamePerformance.Sample(seconds, fps, players.Count(), activeAI);
			}
		}
		if (seconds - m_fLastFlush >= 30)
		{
			m_fLastFlush = seconds;
			Flush();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Historical command intervals survive death/disconnect; native ownership is never replaced.
	protected void SampleCommand(float seconds)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || m_bFinished)
			return;
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			string key = faction.GetFactionKey();
			int playerId = manager.GetFactionCommanderPlayerId(key);
			string identity;
			if (playerId > 0)
				identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			GC_MissionCommandRecord last;
			for (int index = m_aCommand.Count() - 1; index >= 0; index--)
			{
				if (m_aCommand[index].m_sFaction == key)
				{
					last = m_aCommand[index];
					break;
				}
			}
			if (last && !last.m_bEnded)
			{
				last.m_fUntil = seconds;
				if (last.m_sIdentity == identity)
					continue;
				last.m_bEnded = true;
			}
			if (identity.IsEmpty())
				continue;
			if (m_aCommand.Count() >= 512)
			{
				m_bComplete = false;
				return;
			}
			GC_MissionCommandRecord record = new GC_MissionCommandRecord();
			record.m_sFaction = key;
			record.m_sFactionName = faction.GetFactionName();
			record.m_sIdentity = identity;
			record.m_sName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			record.m_fFrom = seconds;
			record.m_fUntil = seconds;
			m_aCommand.Insert(record);
		}
	}

	//------------------------------------------------------------------------------------------------
	void EndMission(SCR_GameModeEndData data)
	{
		if (m_bFinished || m_bClosed)
			return;
		Tick();
		m_bFinished = true;
		m_iEndedAt = System.GetUnixTime();
		m_sStatus = "completed";
		if (data)
		{
			m_iEndReason = data.GetEndReason();
			array<int> factionIds = {};
			data.GetFactionWinnerIds(factionIds);
			foreach (int factionId : factionIds)
			{
				Faction faction = GetGame().GetFactionManager().GetFactionByIndex(factionId);
				if (faction)
					m_aResultFactions.Insert(faction.GetFactionKey());
			}
			// Some native endings name a defeated faction. Preserve raw context without inventing a winner.
			if (m_iEndReason == EGameOverTypes.EDITOR_FACTION_VICTORY
				|| m_iEndReason == EGameOverTypes.FACTION_VICTORY_SCORE
				|| m_iEndReason == EGameOverTypes.FACTION_VICTORY_TIME)
			{
				m_aWinningFactions.Copy(m_aResultFactions);
				if (!m_aWinningFactions.IsEmpty())
					m_sOutcome = "victory";
			}
			else if (m_iEndReason == EGameOverTypes.ENDREASON_DRAW || m_iEndReason == EGameOverTypes.FACTION_DRAW
				|| m_iEndReason == EGameOverTypes.EDITOR_FACTION_DRAW || m_iEndReason == EGameOverTypes.COMBATPATROL_DRAW
				|| m_iEndReason == EGameOverTypes.DEATHMATCH_DRAW)
				m_sOutcome = "draw";
			else if (m_iEndReason == EGameOverTypes.SERVER_RESTART)
				m_sStatus = "interrupted";
		}
		CloseCommand();
		Flush();
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseCommand()
	{
		foreach (GC_MissionCommandRecord record : m_aCommand)
		{
			if (!record.m_bEnded)
				record.m_fUntil = m_fObserved;
			record.m_bEnded = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		if (m_bClosed)
			return;
		Tick();
		m_bClosed = true;
		if (!m_bFinished)
		{
			m_sStatus = "interrupted";
			m_iEndedAt = System.GetUnixTime();
		}
		CloseCommand();
		// An earlier immutable retry must publish before the closing snapshot.
		if (!m_sPending.IsEmpty() && !Flush())
			return;
		Flush();
	}

	//------------------------------------------------------------------------------------------------
	string Encode()
	{
		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("kind", "missionHistory");
		context.WriteValue("schemaVersion", 1);
		context.WriteValue("missionId", m_sMissionId);
		context.WriteValue("revision", m_iRevision);
		context.WriteValue("serverId", m_sServerId);
		context.WriteValue("serverName", m_sServerName);
		context.WriteValue("hostKind", m_sHostKind);
		context.WriteValue("missionName", m_sMissionName);
		context.WriteValue("author", m_sAuthor);
		context.WriteValue("description", m_sDescription);
		context.WriteValue("world", m_sWorld);
		context.WriteValue("scenarioPath", m_sScenarioPath);
		context.WriteValue("startedAt", m_iStartedAt);
		context.WriteValue("endedAt", m_iEndedAt);
		context.WriteValue("observedSeconds", m_fObserved);
		context.WriteValue("status", m_sStatus);
		context.WriteValue("outcome", m_sOutcome);
		context.WriteValue("endReason", m_iEndReason);
		context.WriteValue("resultFactions", m_aResultFactions);
		context.WriteValue("winningFactions", m_aWinningFactions);
		context.WriteValue("closed", m_bClosed);
		context.WriteValue("complete", m_bComplete);
		context.StartObject("sessionPerformance");
		m_SessionPerformance.Write(context);
		context.EndObject();
		context.StartObject("gamePerformance");
		m_GamePerformance.Write(context);
		context.EndObject();
		int factionCount = m_aFactions.Count();
		context.StartArray("factions", factionCount);
		foreach (GC_MissionFactionRecord faction : m_aFactions)
		{
			context.StartObject();
			faction.Write(context);
			context.EndObject();
		}
		context.EndArray();
		int phaseCount = m_aPhases.Count();
		context.StartArray("phases", phaseCount);
		foreach (GC_MissionPhaseRecord phase : m_aPhases)
		{
			context.StartObject();
			phase.Write(context);
			context.EndObject();
		}
		context.EndArray();
		int commanderCount = m_aCommand.Count();
		context.StartArray("commanders", commanderCount);
		foreach (GC_MissionCommandRecord commander : m_aCommand)
		{
			context.StartObject();
			commander.Write(context);
			context.EndObject();
		}
		context.EndArray();
		if (!context.IsValid())
			return string.Empty;
		return context.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Flush()
	{
		if (m_sPending.IsEmpty())
			m_sPending = Encode();
		string path = string.Format("%1/%2-history-%3.json", OUTBOX, m_sMissionId, m_iRevision);
		if (m_sPending.Length() <= 2 || !GC_StatsOutbox.WriteVerified(path, m_sPending)
			|| !GC_StatsOutbox.WriteReady(path + ".ready"))
		{
			if (!m_bWriteFailure)
				Print("GC | Mission history | write failed; pending revision retained", LogLevel.ERROR);
			m_bWriteFailure = true;
			return false;
		}
		m_bWriteFailure = false;
		m_sPending = string.Empty;
		m_iRevision++;
		return true;
	}
}

class GC_MissionPhaseRecord
{
	string m_sPhase;
	float m_fSeconds;
	int m_iAt;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("phase", m_sPhase);
		context.WriteValue("sessionSeconds", m_fSeconds);
		context.WriteValue("at", m_iAt);
	}
}

class GC_MissionCommandRecord
{
	string m_sFaction;
	string m_sFactionName;
	string m_sIdentity;
	string m_sName;
	float m_fFrom;
	float m_fUntil;
	bool m_bEnded;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("faction", m_sFaction);
		context.WriteValue("factionName", m_sFactionName);
		context.WriteValue("identity", m_sIdentity);
		context.WriteValue("name", m_sName);
		context.WriteValue("fromSeconds", m_fFrom);
		context.WriteValue("untilSeconds", m_fUntil);
	}
}

class GC_MissionFactionRecord
{
	string m_sKey;
	string m_sName;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("key", m_sKey);
		context.WriteValue("name", m_sName);
	}
}
