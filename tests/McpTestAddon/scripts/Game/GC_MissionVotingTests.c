#ifdef WORKBENCH
modded class GC_MissionVote
{
	bool m_bGC_TestHoldLoad;
	bool m_bGC_TestLoadRequested;

	string GC_TestPopulationFilter()
	{
		int failures;
		m_aCandidates = {"{\"maxPlayers\":100}", "{\"maxPlayers\":200}", "{\"maxPlayers\":300}",
			"{\"maxPlayers\":10}", "{\"maxPlayers\":20}", "{\"maxPlayers\":40}"};
		SelectForPopulation(50);
		for (int i; i < 6; i++)
		{
			int capacity = GetMaximumCapacity(m_aRows[i]);
			if (!HasCandidate(i) || (i < 3 && capacity < 50) || (i >= 3 && capacity > 50))
				failures++;
			for (int j = i + 1; j < 6; j++)
			{
				if (m_aRows[i] == m_aRows[j])
					failures++;
			}
		}
		m_aRows.Clear();
		m_aCounts.Clear();
		m_aCandidates = {"{\"maxPlayers\":100}", "{\"maxPlayers\":200}", "{\"maxPlayers\":300}",
			"{\"maxPlayers\":400}", "{\"maxPlayers\":50}", "{}"};
		SelectForPopulation(50);
		for (int slot; slot < 6; slot++)
		{
			if (!HasCandidate(slot))
				failures++;
		}
		m_aRows.Clear();
		m_aCounts.Clear();
		m_aCandidates = {"{\"maxPlayers\":40}"};
		SelectForPopulation(50);
		if (HasCandidate(0) || !HasCandidate(3) || HasCandidate(-1) || HasCandidate(6))
			failures++;
		m_aCounts[2] = 100;
		if (PickWinner() != 3)
			failures++;
		m_aRows.Clear();
		m_aCounts.Clear();
		m_aCandidates.Clear();
		SelectForPopulation(50);
		if (!m_aRows.IsEmpty() || PickWinner() != -1)
			failures++;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		m_aCandidates = {string.Format("{\"maxPlayers\":%1}", players.Count())};
		SelectCandidates();
		if (!HasCandidate(0) || HasCandidate(3))
			failures++;
		return string.Format("PopulationGroups: failures=%1 connected=%2; broad groups, sparse borrowing, unknown counts, unique choices and empty-slot winners checked", failures, players.Count());
	}

	//! Explicit synthetic 50-player preview, never used by production selection.
	void GC_TestPopulationPreview()
	{
		m_bGC_TestHoldLoad = true;
		m_aCandidates.Clear();
		m_aRows.Clear();
		m_aCounts.Clear();
		m_mBallots.Clear();
		m_sRound = PersistenceIdUtils.Generate();
		array<int> capacities = {60, 55, 50, 48, 45, 40};
		foreach (int capacity : capacities)
			m_aCandidates.Insert(string.Format("{\"name\":\"Capacity %1\",\"minPlayers\":20,\"maxPlayers\":%1,\"description\":\"50-player selection test\",\"tags\":\"Test fixture\",\"thumbnail\":\"\",\"plays\":\"0\",\"lastPlayed\":\"Never played\"}", capacity));
		SelectForPopulation(50);
	}

	override protected void LoadWinner()
	{
		if (m_bGC_TestHoldLoad)
		{
			m_bGC_TestLoadRequested = true;
			return;
		}
		super.LoadWinner();
	}

	string GC_TestSnapshot()
	{
		string counts;
		foreach (int count : m_aCounts)
			counts += count.ToString() + ",";
		return string.Format("ready=%1 candidates=%2 choices=%3 started=%4 remaining=%5 closed=%6 winner=%7 ballots=%8 counts=%9", m_bReady, m_aCandidates.Count(), m_aRows.Count(), m_bStarted, Remaining(), m_bClosed, m_iWinner, m_mBallots.Count(), counts) + string.Format(" loadRequested=%1 status=%2", m_bGC_TestLoadRequested, m_sStatus);
	}

	string GC_TestRejectedVotes(GC_LobbyPlayerController controller)
	{
		string before = GC_TestSnapshot();
		CastVote(controller, "stale-round", 0);
		CastVote(controller, m_sRound, 6);
		CastVote(controller, m_sRound, -1);
		if (before != GC_TestSnapshot())
			return "FAIL rejected ballot mutated state";
		return "PASS stale round and out-of-range ballots rejected";
	}
}

modded class GC_LobbyManager
{
	string GC_TestVotingPopulation()
	{
		GC_MissionVote isolated = new GC_MissionVote(this);
		return isolated.GC_TestPopulationFilter();
	}

	string GC_TestVoting(string action)
	{
		GC_MissionVote vote = GC_GetMissionVote();
		if (!vote)
			return "UNAVAILABLE voting owner";
		if (action == "hold-load")
			vote.m_bGC_TestHoldLoad = true;
		if (action == "population-preview")
			vote.GC_TestPopulationPreview();
		if (action == "reject")
			return vote.GC_TestRejectedVotes(GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()));
		return vote.GC_TestSnapshot();
	}

	string GC_TestVotingClient()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		return string.Format("cards=%1 counts=%2 selected=%3 remaining=%4 closed=%5 menu=%6", controller.m_aGC_VoteCards.Count(), controller.m_aGC_VoteCounts.Count(), controller.m_iGC_VoteSelected, controller.m_iGC_VoteSeconds, controller.m_bGC_VoteClosed, GetGame().GetMenuManager().GetTopMenu().Type());
	}

	string GC_TestVotingPollClick()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		controller.GC_RequestVoteState();
		return GC_TestDebriefPointer("VoteCard0", "Thumbnail", true);
	}

	string GC_TestVotingScroll(int index, string area, float amount)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget();
		Widget card = root.FindAnyWidget("VoteCard" + index);
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(card.FindAnyWidget(area));
		if (!scroll)
			return "UNAVAILABLE scroll";
		if (area == "TagsScroll")
			scroll.SetSliderPos(amount, 0);
		else
			scroll.SetSliderPos(0, amount);
		float x, y;
		scroll.GetSliderPos(x, y);
		return string.Format("x=%1 y=%2", x, y);
	}
}
#endif
