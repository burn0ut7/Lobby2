//! Optional tests drive production conditions/actions with an isolated scheduler.
#ifdef WORKBENCH
class GC_MissionTestFollowup : GC_MissionTestAction
{
	ref GC_MissionTestAction m_Next;

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		super.Execute(context);
		return context.GetRuntime().QueueFollowup(m_Next, "Test follow-up");
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionEventMatches()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionTestCondition first = new GC_MissionTestCondition();
		GC_MissionTestCondition second = new GC_MissionTestCondition();
		GC_MissionTestAction action = new GC_MissionTestAction();
		GC_MissionEvent rule = new GC_MissionEvent();
		rule.m_sName = "Event match test";
		rule.m_bAllowBeforeGame = true;
		rule.m_aConditions = {first, second};
		rule.m_aActions = {action};
		// Input rows: false/false, true/false, false/true, true/true.
		array<bool> expectedAll = {false, false, false, true};
		array<bool> expectedAny = {false, true, true, true};
		array<bool> expectedNone = {true, false, false, false};
		bool truthTable = true;
		for (int row; row < 4; row++)
		{
			first.m_bMet = row == 1 || row == 3;
			second.m_bMet = row == 2 || row == 3;
			rule.m_eMatch = GC_EMissionConditionMatch.ALL_CONDITIONS;
			truthTable = (rule.IsMet(context) == expectedAll[row]) && truthTable;
			rule.m_eMatch = GC_EMissionConditionMatch.ANY_CONDITION;
			truthTable = (rule.IsMet(context) == expectedAny[row]) && truthTable;
			rule.m_eMatch = GC_EMissionConditionMatch.NO_CONDITIONS;
			truthTable = (rule.IsMet(context) == expectedNone[row]) && truthTable;
		}
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		array<ref GC_MissionEvent> events = {rule};
		bool started = runtime.Start(context, events);
		runtime.Update(1);
		bool scheduled = action.m_iCalls == 0;
		first.m_bMet = false;
		second.m_bMet = false;
		runtime.Update(2);
		runtime.Update(3);
		scheduled = scheduled && action.m_iCalls == 1;
		runtime.Stop();
		second.m_bInvert = true;
		bool inverted = !rule.IsMet(context);
		rule.m_aConditions.Clear();
		string error;
		bool emptyRejected = !rule.Validate(context, error);
		return string.Format("pass=%1 truthTable=%2 started=%3 once=%4 inversion=%5 emptyRejected=%6",
			truthTable && started && scheduled && inverted && emptyRejected, truthTable, started, scheduled, inverted, emptyRejected);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionExpansion()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionTestCondition yes = new GC_MissionTestCondition();
		yes.m_bMet = true;
		GC_MissionTestCondition no = new GC_MissionTestCondition();
		GC_ConditionGroup group = new GC_ConditionGroup();
		group.m_aConditions = {yes, no};
		group.m_iNumber = 1;
		group.m_eMatch = GC_EMissionGroupMatch.ALL;
		bool groups = !group.Evaluate(context);
		group.m_eMatch = GC_EMissionGroupMatch.ANY;
		groups = groups && group.Evaluate(context);
		group.m_eMatch = GC_EMissionGroupMatch.AT_LEAST;
		groups = groups && group.Evaluate(context);
		group.m_eMatch = GC_EMissionGroupMatch.AT_MOST;
		groups = groups && group.Evaluate(context);
		group.m_eMatch = GC_EMissionGroupMatch.EXACTLY;
		groups = groups && group.Evaluate(context);
		group.m_eMatch = GC_EMissionGroupMatch.ALL_SAME;
		groups = groups && !group.Evaluate(context);
		no.m_bInvert = true;
		groups = groups && group.Evaluate(context);
		no.m_bInvert = false;
		GC_ConditionGroup nested = new GC_ConditionGroup();
		nested.m_aConditions = {group};
		nested.m_bInvert = true;
		groups = groups && nested.Evaluate(context);

		GC_HeldCondition held = new GC_HeldCondition();
		held.m_fSeconds = 2;
		held.m_Condition = yes;
		context.SetTime(10);
		bool timing = !held.Evaluate(context);
		context.SetTime(12);
		timing = timing && held.Evaluate(context);
		yes.m_bMet = false;
		timing = timing && !held.Evaluate(context);
		yes.m_bMet = true;
		context.SetTime(13);
		timing = timing && !held.Evaluate(context);

		GC_MissionFlag first = new GC_MissionFlag();
		first.m_sName = "Test A";
		GC_MissionFlag second = new GC_MissionFlag();
		second.m_sName = "Test B";
		GC_FlagCondition flag = new GC_FlagCondition();
		flag.m_sFlag = first.m_sName;
		second.m_Condition = flag;
		second.m_bKeepSet = true;
		array<ref GC_MissionFlag> flags = {first, second};
		GC_MissionTestAction count = new GC_MissionTestAction();
		GC_MissionEvent rule = new GC_MissionEvent();
		rule.m_sName = "Test rule";
		rule.m_bAllowBeforeGame = true;
		rule.m_aConditions = {no};
		rule.m_aActions = {count};
		array<ref GC_MissionEvent> events = {rule};
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		bool started = runtime.Start(context, events, flags);
		runtime.Update(20);
		bool controls = count.m_iCalls == 0;
		controls = runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.RUN, false) && controls;
		controls = controls && count.m_iCalls == 0;
		runtime.Update(21);
		controls = controls && count.m_iCalls == 1;
		controls = !runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.RUN, false) && controls;
		controls = runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.REARM, false) && controls;
		no.m_bMet = true;
		runtime.Update(22);
		controls = controls && count.m_iCalls == 2;
		count.m_fDelayAfterEventStarts = 10;
		runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.REARM, false);
		runtime.Update(23);
		runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.DISABLE, false);
		runtime.Update(40);
		controls = controls && count.m_iCalls == 2;

		GC_SetFlagAction setFlag = new GC_SetFlagAction();
		setFlag.m_sFlag = first.m_sName;
		setFlag.m_bSet = true;
		bool flagRules = setFlag.Execute(context) && runtime.IsFlagSet(first.m_sName) && !runtime.IsFlagSet(second.m_sName);
		runtime.Update(41);
		flagRules = flagRules && runtime.IsFlagSet(second.m_sName);
		setFlag.m_bSet = false;
		setFlag.Execute(context);
		runtime.Update(42);
		flagRules = flagRules && runtime.IsFlagSet(second.m_sName) && !runtime.IsFlagSet(first.m_sName);
		GC_RandomFlagAction random = new GC_RandomFlagAction();
		random.m_sFlag = first.m_sName;
		random.m_fChance = 100;
		bool randomBounds = random.Execute(context) && runtime.IsFlagSet(first.m_sName);
		random.m_fChance = 0;
		randomBounds = random.Execute(context) && !runtime.IsFlagSet(first.m_sName) && randomBounds;
		GC_ChooseFlagsAction choose = new GC_ChooseFlagsAction();
		choose.m_aFlags = {first.m_sName, second.m_sName};
		choose.m_iNumber = 2;
		choose.m_bClearFirst = true;
		bool sampling = choose.Execute(context) && choose.m_aFlags.Count() == 2
			&& runtime.IsFlagSet(first.m_sName) && runtime.IsFlagSet(second.m_sName);
		runtime.Stop();
		bool stopped = !setFlag.Execute(context);

		GC_MissionTestFollowup parent = new GC_MissionTestFollowup();
		parent.m_Next = new GC_MissionTestAction();
		parent.m_Next.m_fDelayAfterEventStarts = 5;
		rule.m_aActions = {parent};
		runtime.Start(context, events, flags);
		runtime.Update(50);
		bool followups = parent.m_iCalls == 1 && parent.m_Next.m_iCalls == 0;
		runtime.ControlEvent(rule.m_sName, GC_EMissionEventOperation.DISABLE, false);
		runtime.Update(60);
		followups = followups && parent.m_Next.m_iCalls == 0;
		runtime.Stop();
		GC_AreaCondition area = new GC_AreaCondition();
		area.m_eComparison = GC_EMissionCountComparison.AT_LEAST;
		area.m_fPercentage = 50;
		bool percentages = area.Matches(1, 2) && !area.Matches(1, 3) && !area.Matches(0, 0);
		GC_MissionCaptureProgress progress = new GC_MissionCaptureProgress();
		int status;
		bool capture = !progress.Update(true, 0, 10, false, status);
		capture = !progress.Update(true, 6, 10, false, status) && capture;
		capture = !progress.Update(false, 8, 10, false, status) && status == 2 && capture;
		capture = !progress.Update(true, 13, 10, false, status) && capture;
		capture = progress.Update(true, 14, 10, false, status) && status == 0 && capture;
		capture = progress.Update(false, 19, 10, false, status) && status == 4 && capture;
		capture = !progress.Update(false, 24, 10, false, status) && status == 3 && capture;
		bool passed = started && groups && timing && controls && flagRules && randomBounds && sampling && stopped && followups && percentages && capture;
		return string.Format("pass=%1 groups=%2 timing=%3 control=%4 flags=%5 random=%6 sampling=%7",
			passed, groups, timing, controls, flagRules, randomBounds, sampling)
			+ string.Format(" stopped=%1 followups=%2 percentages=%3 capture=%4", stopped, followups, percentages, capture);
	}
}
#endif
