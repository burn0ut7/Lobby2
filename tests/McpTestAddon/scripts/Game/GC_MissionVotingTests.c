#ifdef WORKBENCH
modded class GC_MissionVote
{
	bool m_bGC_TestHoldLoad;
	bool m_bGC_TestLoadRequested;

	string GC_TestPopulationFilter()
	{
		array<string> rows = {
			"{\"minPlayers\":20,\"maxPlayers\":40}",
			"{\"minPlayers\":50,\"maxPlayers\":60}",
			"{\"minPlayers\":20,\"maxPlayers\":39}",
			"{\"minPlayers\":20,\"maxPlayers\":61}",
			"{\"minPlayers\":10,\"maxPlayers\":100}",
			"{\"minPlayers\":50,\"maxPlayers\":50}",
			"{}", "{\"minPlayers\":null,\"maxPlayers\":60}",
			"{\"minPlayers\":60,\"maxPlayers\":40}",
			"{\"minPlayers\":0,\"maxPlayers\":60}",
			"{\"minPlayers\":51,\"maxPlayers\":60}"
		};
		array<int> expected = {0, 1, -1, -1, -1, 2, -1, -1, -1, -1, -1};
		int failures;
		string observed;
		for (int i; i < rows.Count(); i++)
		{
			observed += GetPopulationBand(rows[i], 50).ToString() + ",";
			if (GetPopulationBand(rows[i], 50) != expected[i])
				failures++;
		}
		if (GetPopulationBand(rows[0], 0) != -1 || GetPopulationBand(rows[0], 51) != -1)
			failures++;
		m_aCandidates = {rows[0], rows[1], rows[5], rows[4]};
		SelectForPopulation(50);
		observed += " slots=" + m_aRows.ToString();
		if (m_aRows.Count() != 6 || m_aRows[0] != rows[1] || m_aRows[1] != rows[5]
			|| !m_aRows[2].IsEmpty() || m_aRows[3] != rows[0] || HasCandidate(4) || HasCandidate(5))
			failures++;
		if (HasCandidate(-1) || HasCandidate(6) || !HasCandidate(0))
			failures++;
		m_aCounts[2] = 100;
		m_aCounts[3] = 1;
		if (PickWinner() != 3)
			failures++;
		m_aRows.Clear();
		m_aCounts.Clear();
		m_aCandidates = {rows[4]};
		SelectForPopulation(50);
		if (!m_aRows.IsEmpty() || m_sStatus != "#GC-Vote_Empty" || PickWinner() != -1)
			failures++;
		// Real production selector samples the connected player list, not a supplied count.
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		string fits = string.Format("{\"minPlayers\":%1,\"maxPlayers\":%1}", players.Count());
		m_aCandidates = {"{}", fits};
		SelectCandidates();
		if (!HasCandidate(0) || m_aRows[0] != fits || HasCandidate(3))
			failures++;
		return string.Format("PopulationBands: checks=17 failures=%1 connected=%2", failures, players.Count()) + " observed=" + observed;
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
