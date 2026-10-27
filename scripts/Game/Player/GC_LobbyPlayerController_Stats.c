//! Owner-local presentation data; never contains another player's personal report.
class GC_StatsCachedReport
{
	string m_sSummary;
	string m_sAmmunition;
	bool m_bComplete;
}

modded class GC_LobbyPlayerController
{
	ref ScriptInvoker<string, string, string, bool> m_GC_OnDebrief = new ScriptInvoker<string, string, string, bool>();
	protected ref map<string, ref GC_StatsCachedReport> m_mGC_DebriefCache = new map<string, ref GC_StatsCachedReport>();
	protected bool m_bGC_DebriefRequested;
	protected float m_fGC_LastDebriefSent = -1;
	protected bool m_bGC_DebriefQueued;

	//------------------------------------------------------------------------------------------------
	void GC_RequestDebrief(string factionKey = string.Empty)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() != SCR_EGameModeState.POSTGAME || GetGame().GetPlayerController() != this)
			return;
		GC_StatsCachedReport cached = m_mGC_DebriefCache.Get(factionKey);
		if (cached)
		{
			m_GC_OnDebrief.Invoke(factionKey, cached.m_sSummary, cached.m_sAmmunition, cached.m_bComplete);
			return;
		}
		// One request obtains every tab. Finalization also pushes them to connected owners.
		if (!m_bGC_DebriefRequested)
		{
			m_bGC_DebriefRequested = true;
			Rpc(GC_RpcAsk_Debrief);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_Debrief()
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		if (GC_LobbyGameMode.GetGameMode().GetState() != SCR_EGameModeState.POSTGAME)
			return;
		GC_SendDebrief();
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendDebrief()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !stats || !manager || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME)
			return;
		if (!stats.IsFinalized())
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (now - m_fGC_LastDebriefSent < 0.5)
		{
			if (!m_bGC_DebriefQueued)
			{
				m_bGC_DebriefQueued = true;
				GetGame().GetCallqueue().CallLater(GC_SendDebrief, 500, false);
			}
			return;
		}
		m_bGC_DebriefQueued = false;
		GetGame().GetCallqueue().Remove(GC_SendDebrief);
		m_fGC_LastDebriefSent = now;
		GC_SendDebriefReport(stats, string.Empty);
		foreach (Faction faction : manager.GetRegisteredFactions())
			GC_SendDebriefReport(stats, faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_SendDebriefReport(GC_MissionStatsComponent stats, string factionKey)
	{
		GC_StatsDebrief report = stats.GetDebrief(GetPlayerId(), factionKey);
		if (report)
			Rpc(GC_RpcDo_Debrief, factionKey, report.GetSummary(), report.GetAmmunition(), stats.IsHistoryComplete());
		else
			Rpc(GC_RpcDo_Debrief, factionKey, "No mission statistics recorded for this selection.", string.Empty, stats.IsHistoryComplete());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_Debrief(string factionKey, string summary, string ammunition, bool complete)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() != SCR_EGameModeState.POSTGAME || GetGame().GetPlayerController() != this)
			return;
		GC_StatsCachedReport cached = new GC_StatsCachedReport();
		cached.m_sSummary = summary;
		cached.m_sAmmunition = ammunition;
		cached.m_bComplete = complete;
		m_mGC_DebriefCache.Set(factionKey, cached);
		m_GC_OnDebrief.Invoke(factionKey, summary, ammunition, complete);
	}

	//------------------------------------------------------------------------------------------------
	void GC_ClearDebriefCache()
	{
		m_mGC_DebriefCache.Clear();
		m_bGC_DebriefRequested = false;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_ClearDebriefCache();
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyPlayerController()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(GC_SendDebrief);
	}
}
