modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_GC_OnVoteChanged = new ScriptInvoker();
	ref array<string> m_aGC_VoteCards = {};
	ref array<int> m_aGC_VoteCounts = {};
	string m_sGC_VoteRound;
	string m_sGC_VoteStatus = "#GC-Vote_Loading";
	int m_iGC_VoteSeconds;
	int m_iGC_VoteSelected = -1;
	int m_iGC_VoteWinner = -1;
	bool m_bGC_VoteClosed;
	protected float m_fGC_LastVoteRequest = -1;
	protected float m_fGC_LastVoteChange = -1;

	//------------------------------------------------------------------------------------------------
	void GC_RequestVoteState()
	{
		if (GetGame().GetPlayerController() != this)
			return;
		string knownRound;
		if (!m_aGC_VoteCounts.IsEmpty() && m_aGC_VoteCards.Count() == m_aGC_VoteCounts.Count())
			knownRound = m_sGC_VoteRound;
		Rpc(GC_RpcAsk_Vote, knownRound, -1);
	}

	//------------------------------------------------------------------------------------------------
	void GC_VoteForMission(int choice)
	{
		if (GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAsk_Vote, m_sGC_VoteRound, choice);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_Vote(string round, int choice)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME
			|| !manager.GC_GetMissionVote() || round.Length() > 36 || choice < -1 || choice > 5
			|| GetGame().GetPlayerManager().GetPlayerController(GetPlayerId()) != this)
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (choice >= 0)
		{
			if (now - m_fGC_LastVoteChange < 0.2)
				return;
			m_fGC_LastVoteChange = now;
			manager.GC_GetMissionVote().CastVote(this, round, choice);
		}
		else
		{
			if (now - m_fGC_LastVoteRequest < 0.5)
				return;
			m_fGC_LastVoteRequest = now;
		}
		manager.GC_GetMissionVote().Deliver(this, round);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendVoteCard(string round, int index, string row)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcDo_VoteCard, round, index, row);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendVoteState(string round, string status, int seconds, array<int> counts, int selected, int winner, bool closed)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcDo_VoteState, round, status, seconds, counts, selected, winner, closed);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_VoteCard(string round, int index, string row)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode()
			|| round != m_sGC_VoteRound || index < 0 || index >= 6)
			return;
		while (m_aGC_VoteCards.Count() <= index)
			m_aGC_VoteCards.Insert("");
		m_aGC_VoteCards[index] = row;
		m_GC_OnVoteChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_VoteState(string round, string status, int seconds, array<int> counts, int selected, int winner, bool closed)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (round != m_sGC_VoteRound)
			m_aGC_VoteCards.Clear();
		m_sGC_VoteRound = round;
		m_sGC_VoteStatus = status;
		m_iGC_VoteSeconds = seconds;
		m_aGC_VoteCounts.Copy(counts);
		m_iGC_VoteSelected = selected;
		m_iGC_VoteWinner = winner;
		m_bGC_VoteClosed = closed;
		m_GC_OnVoteChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		m_aGC_VoteCards.Clear();
		m_aGC_VoteCounts.Clear();
		m_sGC_VoteRound = string.Empty;
		super.ShutdownLobbyPresentation();
	}
}
