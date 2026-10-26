//! Optional probes exercise the production evaluator and scheduler, not copied logic.
#ifdef WORKBENCH
class GC_MissionTestCondition : GC_MissionCondition
{
	bool m_bMet;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		return m_bMet;
	}
}

class GC_MissionTestAction : GC_MissionAction
{
	int m_iCalls;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		m_iCalls++;
		return true;
	}
}

class GC_MissionTestLivingCondition : GC_PlayerCasualtiesCondition
{
	//------------------------------------------------------------------------------------------------
	void Configure(string factionKey, GC_EMissionCountComparison comparison, float percentage)
	{
		m_sFaction = factionKey;
		m_eComparison = comparison;
		m_fPercentage = percentage;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionFramework()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || GetManager() != this || !mode || !m_MissionFramework)
			return "UNAVAILABLE active authority";
		return string.Format("phase=%1 %2", GetPhaseName(GetLobbyPhase()), m_MissionFramework.Describe());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionRules()
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionTestCondition yes = new GC_MissionTestCondition();
		yes.m_bMet = true;
		GC_MissionTestCondition no = new GC_MissionTestCondition();
		GC_MissionTestAction first = new GC_MissionTestAction();
		first.m_fDelayAfterEventStarts = 2;
		GC_MissionTestAction second = new GC_MissionTestAction();
		second.m_fDelayAfterEventStarts = 5;
		GC_MissionEvent rule = new GC_MissionEvent();
		rule.m_sName = "Scheduler fixture";
		rule.m_aConditions = {yes, no};
		rule.m_aActions = {first, second};
		string error;
		bool valid = rule.Validate(context, error);
		bool all = !rule.IsMet(context);
		rule.m_eMatch = GC_EMissionConditionMatch.ANY_CONDITION;
		bool any = rule.IsMet(context);
		array<ref GC_MissionEvent> events = {rule};
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		bool started = runtime.Start(context, events);
		runtime.Update(100);
		if (GetLobbyPhase() != SCR_EGameModeState.GAME)
		{
			runtime.Update(200);
			bool gated = first.m_iCalls == 0 && second.m_iCalls == 0;
			runtime.Stop();
			return string.Format("pregame pass=%1 valid=%2 all=%3 any=%4 gated=%5", valid && all && any && started && gated, valid, all, any, gated);
		}
		yes.m_bMet = false;
		runtime.Update(101);
		bool waits = first.m_iCalls == 0 && second.m_iCalls == 0;
		runtime.Update(102);
		bool firstDue = first.m_iCalls == 1 && second.m_iCalls == 0;
		runtime.Update(105);
		bool independent = first.m_iCalls == 1 && second.m_iCalls == 1;
		yes.m_bMet = true;
		runtime.Update(200);
		bool once = first.m_iCalls == 1 && second.m_iCalls == 1;
		runtime.Stop();
		runtime.Start(context, events);
		runtime.Update(300);
		runtime.Stop();
		runtime.Update(400);
		bool cancelled = first.m_iCalls == 1 && second.m_iCalls == 1;

		array<ref GC_MissionEvent> duplicates = {rule, rule};
		bool duplicateRejected = !runtime.Start(context, duplicates);
		rule.m_aConditions.Clear();
		bool emptyRejected = !runtime.Start(context, events);
		rule.m_aConditions.Insert(yes);
		first.m_fDelayAfterEventStarts = -1;
		bool delayRejected = !runtime.Start(context, events);
		first.m_fDelayAfterEventStarts = 0;
		GC_MissionTestLivingCondition living = new GC_MissionTestLivingCondition();
		living.Configure("NO_SUCH_FACTION", GC_EMissionCountComparison.AT_MOST, 0);
		bool factionRejected = !living.Validate(context, error);
		GC_LobbyMissionContext freshContext = new GC_LobbyMissionContext(this);
		living.Configure("US", GC_EMissionCountComparison.AT_MOST, 0);
		bool unpopulated = !living.IsMet(freshContext);
		freshContext.CapturePlayers();
		float percentage;
		bool observed = freshContext.TryGetPlayerCasualties("US", percentage);
		living.Configure("US", GC_EMissionCountComparison.EXACTLY, percentage);
		bool exact = living.IsMet(freshContext) == observed;
		bool passed = valid && all && any && started && waits && firstDue && independent && once && cancelled
			&& duplicateRejected && emptyRejected && delayRejected && factionRejected && unpopulated && exact;
		string result = string.Format("pass=%1 all=%2 any=%3 waits=%4 firstDue=%5 independent=%6 once=%7 cancelled=%8",
			passed, all, any, waits, firstDue, independent, once, cancelled);
		return result + string.Format(" duplicate=%1 empty=%2 delay=%3 faction=%4 unpopulated=%5 exact=%6 casualtyPercentUS=%7 observedUS=%8",
			duplicateRejected, emptyRejected, delayRejected, factionRejected, unpopulated, exact, percentage, observed);
	}
}

#endif
