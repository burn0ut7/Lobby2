#ifdef WORKBENCH
modded class GC_MissionVote
{
	bool m_bGC_TestHoldLoad;
	bool m_bGC_TestLoadRequested;

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
	string GC_TestVoting(string action)
	{
		GC_MissionVote vote = GC_GetMissionVote();
		if (!vote)
			return "UNAVAILABLE voting owner";
		if (action == "hold-load")
			vote.m_bGC_TestHoldLoad = true;
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
