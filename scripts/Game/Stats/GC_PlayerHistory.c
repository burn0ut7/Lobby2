//! Database-derived prior mission time; current mission time remains owned by statistics.
class GC_PlayerHistory
{
	protected GC_LobbyManager m_Manager;
	protected string m_sToken;
	protected string m_sMission;
	protected float m_fRequestedAt;
	protected float m_fReceivedAt = -1000;
	protected bool m_bUnavailableLogged;
	protected ref map<string, float> m_mSeconds = new map<string, float>();
	protected ref map<string, bool> m_mKnown = new map<string, bool>();
	static const string DIRECTORY = "$profile:Lobby2PlayerHistory";
	static const float NEW_PLAYER_SECONDS = 10800;

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyManager manager)
	{
		m_Manager = manager;
		FileIO.MakeDirectory(DIRECTORY);
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		GetGame().GetCallqueue().Remove(Tick);
		m_Manager = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!Replication.IsServer() || !m_Manager || GC_LobbyManager.GetManager() != m_Manager)
			return;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!logs || logs.GetSessionId().IsEmpty())
			return;
		m_sMission = logs.GetSessionId();
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		ReadResponse(now);
		if (now > 90 && now - m_fReceivedAt > 90 && !m_bUnavailableLogged)
		{
			Print("GC | Player history | unavailable or stale; new-player names withheld", LogLevel.WARNING);
			m_bUnavailableLogged = true;
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<string> identities = {};
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		foreach (int playerId : players)
		{
			string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			if (identity.IsEmpty())
				continue;
			if (identities.Count() < 256)
				identities.Insert(identity);
			float current;
			GC_StatsPlayer record;
			if (stats)
				record = stats.GetRecordedPlayer(playerId);
			if (record)
				current = record.m_fPlaySeconds;
			float prior;
			bool known;
			bool received = m_mSeconds.Find(identity, prior) && m_mKnown.Find(identity, known);
			bool isNew = received && known && now - m_fReceivedAt <= 90 && prior + current < NEW_PLAYER_SECONDS;
			m_Manager.PublishNewPlayer(playerId, identity, isNew);
		}
		if (m_sToken.IsEmpty() && now - m_fRequestedAt >= 15 && !identities.IsEmpty())
		{
			m_sToken = PersistenceIdUtils.Generate();
			m_fRequestedAt = now;
			JsonSaveContext context = new JsonSaveContext(false);
			context.WriteValue("version", 1);
			context.WriteValue("token", m_sToken);
			context.WriteValue("missionId", m_sMission);
			context.WriteValue("identities", identities);
			string path = DIRECTORY + "/" + m_sToken + ".request";
			if (!GC_StatsOutbox.WriteVerified(path, context.SaveToString()) || !GC_StatsOutbox.WriteReady(path + ".ready"))
			{
				Print("GC | Player history | request storage unavailable", LogLevel.ERROR);
				m_sToken = string.Empty;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadResponse(float now)
	{
		if (m_sToken.IsEmpty())
			return;
		string path = DIRECTORY + "/" + m_sToken + ".response";
		if (!FileIO.FileExists(path))
			return;
		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return;
		string payload;
		file.Read(payload, 131072);
		file.Close();
		JsonLoadContext context = new JsonLoadContext(false);
		int version, generatedAt;
		string token, mission;
		array<string> identities = {};
		array<float> seconds = {};
		array<bool> known = {};
		if (!context.LoadFromString(payload) || !context.ReadValue("version", version) || version != 1
			|| !context.ReadValue("token", token) || token != m_sToken
			|| !context.ReadValue("missionId", mission) || mission != m_sMission
			|| !context.ReadValue("identities", identities) || !context.ReadValue("seconds", seconds)
			|| !context.ReadValue("known", known) || identities.Count() != seconds.Count()
			|| !context.ReadValue("generatedAt", generatedAt)
			|| identities.Count() != known.Count() || identities.Count() > 256)
			return;
		if (Math.AbsInt(System.GetUnixTime() - generatedAt) > 90)
		{
			FileIO.DeleteFile(path);
			m_sToken = string.Empty;
			return;
		}
		m_mSeconds.Clear();
		m_mKnown.Clear();
		foreach (int index, string identity : identities)
		{
			if (seconds[index] < 0)
				continue;
			m_mSeconds.Set(identity, seconds[index]);
			m_mKnown.Set(identity, known[index]);
		}
		m_fReceivedAt = now;
		m_bUnavailableLogged = false;
		FileIO.DeleteFile(path);
		m_sToken = string.Empty;
	}
}
