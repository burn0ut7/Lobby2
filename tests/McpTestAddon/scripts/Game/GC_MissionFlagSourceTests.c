#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionFlagSources()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		GC_FlagCondition recovered = new GC_FlagCondition();
		recovered.m_sFlag = "Blackbox recovered";
		GC_MissionTestAction count = new GC_MissionTestAction();
		GC_MissionEvent rule = new GC_MissionEvent();
		rule.m_sName = "Recovery reaction";
		rule.m_bAllowBeforeGame = true;
		rule.m_aConditions = {recovered};
		rule.m_aActions = {count};
		array<ref GC_MissionEvent> events = {rule};
		bool started = runtime.Start(context, events);
		runtime.Update(1);
		bool waiting = !runtime.HasFlag(recovered.m_sFlag) && !recovered.Evaluate(context) && count.m_iCalls == 0;
		GC_SetFlagAction source = new GC_SetFlagAction();
		source.m_sFlag = recovered.m_sFlag;
		source.m_bSet = true;
		string error;
		bool written = source.Validate(context, error) && source.Execute(context);
		runtime.Update(2);
		GC_SetFlagAction otherSource = new GC_SetFlagAction();
		otherSource.m_sFlag = recovered.m_sFlag;
		otherSource.m_bSet = true;
		bool shared = otherSource.Execute(context);
		array<string> names = {};
		array<bool> values = {};
		runtime.CopyFlagSnapshot(names, values);
		shared = shared && names.Count() == 1 && values[0] && count.m_iCalls == 1;
		source.m_bSet = false;
		bool cleared = source.Execute(context) && !recovered.Evaluate(context);
		GC_RandomFlagAction random = new GC_RandomFlagAction();
		random.m_sFlag = "Random source";
		random.m_fChance = 0;
		bool randomized = random.Validate(context, error) && random.Execute(context)
			&& runtime.HasFlag(random.m_sFlag) && !runtime.IsFlagSet(random.m_sFlag);
		random.m_fChance = 100;
		randomized = random.Execute(context) && runtime.IsFlagSet(random.m_sFlag) && randomized;
		GC_ChooseFlagsAction choose = new GC_ChooseFlagsAction();
		choose.m_aFlags = {"Choice A", "Choice B"};
		choose.m_iNumber = 2;
		bool chosen = choose.Validate(context, error) && choose.Execute(context)
			&& runtime.IsFlagSet("Choice A") && runtime.IsFlagSet("Choice B");
		recovered.m_sFlag = "";
		source.m_sFlag = "";
		bool blanks = !recovered.Validate(context, error) && !source.Validate(context, error) && !runtime.SetFlag("", true);
		runtime.Stop();
		bool stopped = !runtime.SetFlag("After stop", true);
		events.Clear();
		bool emptyStarted = runtime.Start(context, events);
		bool reset = !runtime.HasFlag("Blackbox recovered") && runtime.SetFlag("Interaction only", true);
		runtime.CopyFlagSnapshot(names, values);
		reset = reset && names.Count() == 1 && names[0] == "Interaction only" && values[0];
		runtime.Stop();
		return string.Format("pass=%1 waiting=%2 written=%3 shared=%4 cleared=%5 random=%6 chosen=%7",
			started && waiting && written && shared && cleared && randomized && chosen && blanks && stopped && emptyStarted && reset,
			waiting, written, shared, cleared, randomized, chosen)
			+ string.Format(" blanks=%1 stopped=%2 emptyStarted=%3 reset=%4", blanks, stopped, emptyStarted, reset);
	}
}
#endif
