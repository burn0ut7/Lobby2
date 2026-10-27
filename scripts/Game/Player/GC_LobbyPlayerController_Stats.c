modded class GC_LobbyPlayerController
{
	ref ScriptInvoker<string, string, string, bool> m_GC_OnDebrief = new ScriptInvoker<string, string, string, bool>();
	protected string m_sGC_DebriefFaction;
	protected float m_fGC_LastDebriefSent = -1;
	protected bool m_bGC_DebriefQueued;

	//------------------------------------------------------------------------------------------------
	void GC_RequestDebrief(string factionKey = string.Empty)
	{
		if (GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_Debrief, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_Debrief(string factionKey)
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		if (factionKey.Length() > 128 || GC_LobbyGameMode.GetGameMode().GetState() != SCR_EGameModeState.POSTGAME)
			return;
		m_sGC_DebriefFaction = factionKey;
		GC_SendDebrief();
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendDebrief()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!Replication.IsServer() || !stats)
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
		GC_StatsDebrief report = stats.GetDebrief(GetPlayerId(), m_sGC_DebriefFaction);
		if (report)
			Rpc(GC_RpcDo_Debrief, m_sGC_DebriefFaction, report.GetSummary(), report.GetAmmunition(), stats.IsHistoryComplete());
		else
			Rpc(GC_RpcDo_Debrief, m_sGC_DebriefFaction, "No mission statistics recorded for this selection.", string.Empty, stats.IsHistoryComplete());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_Debrief(string factionKey, string summary, string ammunition, bool complete)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (mode && mode.GetState() == SCR_EGameModeState.POSTGAME && GetGame().GetPlayerController() == this)
			m_GC_OnDebrief.Invoke(factionKey, summary, ammunition, complete);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyPlayerController()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(GC_SendDebrief);
	}
}
