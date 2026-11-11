//! Controlled population inputs; all counting, timing and condition evaluation use production code.
class GC_CaptureTestPopulation : GC_AreaPopulation
{
	bool m_bReady = true;

	override bool IsAvailable()
	{
		return m_bReady;
	}

	override void Sample(GC_LobbyManager manager, bool includeAI)
	{
	}

	void Add(string faction, bool player = true, float x = 1)
	{
		GC_AreaCharacter character = new GC_AreaCharacter();
		character.m_sFaction = faction;
		character.m_bPlayer = player;
		character.m_vPosition = Vector(x, 0, 0);
		m_aCharacters.Insert(character);
	}
}

class GC_CaptureTestGeometry : GC_MissionAreaGeometry
{
	bool m_bReady = true;

	override bool IsAvailable()
	{
		return m_bReady;
	}

	override bool ContainsEntity(IEntity entity, vector position)
	{
		return position[0] >= 0;
	}
}

class GC_CaptureTestTracker : GC_MissionAreaTracker
{
	void Install(GC_CaptureTestPopulation population, GC_CaptureTestGeometry area)
	{
		m_Population = population;
		foreach (GC_MissionAreaObservation observation : m_aPairs)
			observation.m_Area = area;
	}
}

modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunCaptureWorld()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!Replication.IsServer() || world.FindEntityByName("GC_CaptureFixtureArea") || world.FindEntityByName("GC_CaptureFixtureTarget"))
			return "Capture world fixture refused: authority or existing names";
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = Vector(100, 100, 100);
		IEntity area = GetGame().SpawnEntityPrefab(Resource.Load("{2C4447C53A1E450B}Prefabs/Logic/Mission/RadiusArea.et"), world, parameters);
		IEntity target = GetGame().SpawnEntity(GenericEntity, world, parameters);
		if (!area || !target)
		{
			if (area)
				SCR_EntityHelper.DeleteEntityAndChildren(area);
			if (target)
				SCR_EntityHelper.DeleteEntityAndChildren(target);
			return "Capture world fixture failed to spawn";
		}
		area.SetName("GC_CaptureFixtureArea");
		target.SetName("GC_CaptureFixtureTarget");
		GC_AreaCondition condition = new GC_AreaCondition();
		condition.m_sArea = area.GetName();
		GC_NamedAreaTargets targets = new GC_NamedAreaTargets();
		targets.m_aNames = {target.GetName()};
		condition.m_Targets = targets;
		condition.m_Requirement = new GC_AreaCount();
		condition.m_Capture = new GC_AreaCaptureOptions();
		condition.m_Capture.m_fSeconds = 2;
		condition.m_Capture.m_bSkipDiscovery = true;
		GC_Event authored = StateRule("captured", SCR_EGameModeState.GAME);
		authored.m_aConditions = {condition};
		authored.m_Advanced.m_bRepeatable = true;
		GC_CompiledMissionRule rule = authored.BuildRule();
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({rule}, null, SCR_EGameModeState.BRIEFING, null, null, world);
		engine.Advance(10);
		Check(engine.GetRunCount() == 0, "real area cannot capture during briefing");
		engine.SetGameModeState(SCR_EGameModeState.GAME, 10);
		engine.Advance(11);
		Check(engine.GetRunCount() == 0, "real area waits for full capture duration");
		engine.Advance(12);
		Check(engine.GetRunCount() == 1 && engine.GetFlag("captured"), "native geometry and production scheduler execute capture event");
		target.SetOrigin(Vector(120, 100, 100));
		engine.Advance(13);
		engine.Advance(14);
		Check(engine.GetRunCount() == 1, "moving outside loses capture without running positive event");
		target.SetOrigin(Vector(100, 100, 100));
		engine.Advance(15);
		engine.Advance(16);
		Check(engine.GetRunCount() == 2, "moving back recaptures and repeats event");
		target.SetOrigin(Vector(120, 100, 100));
		engine.Advance(17);
		engine.Advance(18);
		target.SetOrigin(Vector(100, 100, 100));
		engine.Advance(19);
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 19);
		engine.Advance(100);
		Check(engine.GetRunCount() == 2, "postgame cancels incomplete capture");
		engine.Stop();
		SCR_EntityHelper.DeleteEntityAndChildren(target);
		SCR_EntityHelper.DeleteEntityAndChildren(area);
		Check(!world.FindEntityByName("GC_CaptureFixtureArea") && !world.FindEntityByName("GC_CaptureFixtureTarget"), "test world objects removed");
		return string.Format("Capture world checks: %1; failures: %2", m_iChecks, m_iFailures);
	}

	//------------------------------------------------------------------------------------------------
	string RunCapture()
	{
		GC_AreaCaptureOptions options = new GC_AreaCaptureOptions();
		Check(options.IsValid() && options.m_iCheckInterval == 1, "capture authored defaults valid");
		options.m_fSeconds = 3;
		options.m_bSkipDiscovery = true;
		GC_AreaCaptureState state = new GC_AreaCaptureState();
		Check(state.Sample(options, true, 0) == 1 && !state.IsCaptured(), "skip discovery begins capture without unobserved time");
		Check(state.Sample(options, true, 1) == -1 && state.GetProgress() == 1, "unchanged population still accumulates capture time");
		state.Sample(options, true, 2);
		Check(state.Sample(options, false, 3) == 2 && state.GetProgress() == 1, "contest reports interruption and rolls back");
		Check(state.Sample(options, false, 4) == -1 && state.GetProgress() == 0, "rollback continues beyond first interrupted tick without repeated messages");
		Check(state.Sample(options, true, 5) == 1, "capture resumes");
		state.Sample(options, true, 6);
		Check(state.Sample(options, true, 7) == 0 && state.IsCaptured(), "capture after full three ticks");
		Check(state.Sample(options, false, 8) == 4, "loss begins");
		Check(state.Sample(options, true, 9) == 5 && state.IsCaptured(), "loss interrupted preserves captured state");
		state.Sample(options, false, 10);
		state.Sample(options, false, 11);
		Check(state.Sample(options, false, 12) == 3 && !state.IsCaptured(), "loss completes after configured duration");
		options.m_iInterruption = 1;
		state = new GC_AreaCaptureState();
		state.Sample(options, true, 0);
		state.Sample(options, true, 1);
		state.Sample(options, false, 2);
		state.Sample(options, false, 3);
		Check(state.GetProgress() == 1, "Hold preserves progress through contested ticks");
		options.m_iInterruption = 2;
		state.Sample(options, false, 4);
		Check(state.GetProgress() == 0, "Reset clears interrupted progress");
		options.m_iCheckInterval = 2;
		options.m_fSeconds = 4;
		state = new GC_AreaCaptureState();
		state.Sample(options, true, 0);
		state.Sample(options, true, 1);
		Check(state.GetProgress() == 0, "interval suppresses intermediate ticks");
		state.Sample(options, true, 2);
		Check(state.GetProgress() == 2, "two-second tick");
		Check(state.Sample(options, true, 4) == 0, "four seconds needs two ticks");
		state = new GC_AreaCaptureState();
		state.Sample(options, true, 0);
		state.Sample(options, true, 100);
		Check(!state.IsCaptured() && state.GetProgress() == 2, "server stall never invents catch-up ticks");
		state.Suspend(100);
		Check(state.GetProgress() == 0, "missing area resets pending progress");
		options.m_bSkipDiscovery = false;
		options.m_bStopAfterFirstChange = true;
		state = new GC_AreaCaptureState();
		Check(state.Sample(options, true, 0) == -1 && state.IsCaptured(), "discovery silently establishes result");
		state.Sample(options, false, 2);
		Check(state.Sample(options, false, 4) == 3 && !state.IsCaptured(), "one-shot can complete a loss after discovery");
		state.Sample(options, true, 6);
		state.Sample(options, true, 8);
		Check(!state.IsCaptured(), "one-shot ignores subsequent capture attempts");
		options.m_bSkipDiscovery = true;
		options.m_fSeconds = 0;
		state = new GC_AreaCaptureState();
		Check(state.Sample(options, true, 0) == 0 && state.IsCaptured(), "zero duration captures immediately");
		options.m_iCheckInterval = 0;
		Check(!options.IsValid(), "zero interval invalid");
		options.m_iCheckInterval = 1;
		options.m_fSeconds = -1;
		Check(!options.IsValid(), "negative duration invalid");
		options.m_fSeconds = 2;
		options.m_bStopAfterFirstChange = false;

		GC_AreaCondition authored = new GC_AreaCondition();
		authored.m_sArea = "CaptureTest";
		authored.m_Capture = options;
		GC_CharacterAreaTargets targets = new GC_CharacterAreaTargets();
		targets.m_aFactions = {"US"};
		targets.m_iWho = 2;
		authored.m_Targets = targets;
		GC_LocalFactionShare share = new GC_LocalFactionShare();
		share.m_fRequiredShare = 0.75;
		share.m_aIgnoredFactions = {"CIV"};
		authored.m_Requirement = share;
		GC_RuleCondition check = authored.BuildCheck();
		Check(check && check.m_Capture != options, "capture settings copied into compiled rule");
		GC_CaptureTestPopulation population = new GC_CaptureTestPopulation();
		GC_CaptureTestGeometry geometry = new GC_CaptureTestGeometry();
		GC_MissionAreaObservation observation = new GC_MissionAreaObservation();
		observation.m_Definition = check;
		observation.m_Area = geometry;
		population.Add("US");
		population.Add("US");
		population.Add("US", false);
		population.Add("USSR");
		population.Add("CIV");
		population.Add("USSR", true, -1);
		observation.Sample(population);
		Check(observation.PassesCondition(check), "three of four local contestants reaches 75%; civilians and outside enemies excluded");
		check.m_fAreaProportion = 1;
		Check(!observation.PassesCondition(check), "100% requires uncontested occupation");
		check.m_fAreaProportion = 0.75;
		check.m_bAreaOutside = true;
		Check(!observation.PassesCondition(check), "outside share uses outside contestants");
		check.m_bAreaOutside = false;
		check.m_iAreaWho = 0;
		observation.Sample(population);
		Check(!observation.PassesCondition(check), "players-only excludes AI from both numerator and denominator");
		check.m_iAreaWho = 2;
		check.m_aAreaFactions.Insert("USSR");
		check.m_fAreaProportion = 1;
		observation.Sample(population);
		Check(observation.PassesCondition(check), "multiple capturing factions combine as one force");
		population.m_aCharacters.Clear();
		observation.Sample(population);
		Check(!observation.PassesCondition(check), "empty area cannot meet at-least share");
		check.m_bShareAtMost = true;
		Check(observation.PassesCondition(check), "empty area meets at-most share");
		share.m_aIgnoredFactions = {"US"};
		Check(!authored.BuildCheck(), "capturing and ignored faction overlap rejected");
		share.m_aIgnoredFactions = {};
		targets.m_aFactions = {};
		Check(!authored.BuildCheck(), "local share requires capturing factions");
		targets.m_aFactions = {"US"};
		check = authored.BuildCheck();
		GC_CaptureTestTracker tracker = new GC_CaptureTestTracker(null);
		Check(tracker.Register(check), "capture registers with shared sampler");
		tracker.Install(population, geometry);
		population.Add("US");
		array<GC_RuleCondition> changed = {};
		tracker.Sample(changed, true, 0, false);
		Check(!tracker.IsAvailable(check) && !check.Evaluate(null, SCR_EGameModeState.BRIEFING, null, null, null, null, null, tracker), "capture unavailable before Match");
		tracker.Sample(changed, true, 0, true);
		Check(!tracker.PassesCondition(check), "Match starts capture timing");
		tracker.Sample(changed, false, 1, true);
		changed.Clear();
		tracker.Sample(changed, false, 2, true);
		Check(changed.Contains(check) && check.Evaluate(null, SCR_EGameModeState.GAME, null, null, null, null, null, tracker), "unchanged counts publish completed capture to rule evaluation");
		geometry.m_bReady = false;
		tracker.Sample(changed, false, 3, true);
		GC_RuleCondition none = new GC_RuleCondition();
		none.m_eGroupNeeds = GC_ERuleCheck.NONE;
		none.m_aChecks = {check};
		Check(!none.Evaluate(null, SCR_EGameModeState.GAME, null, null, null, null, null, tracker), "unavailable captured area fails closed under None");
		geometry.m_bReady = true;
		tracker.Sample(changed, true, 4, false);
		Check(!tracker.IsAvailable(check), "capture unavailable after Match");
		GC_CaptureMessages messages = new GC_CaptureMessages();
		Check(messages.IsValid() && !messages.GetText(5).IsEmpty() && messages.GetText(6).IsEmpty(), "six named message states");
		GC_RuleAction reversed = messages.BuildMessage(0, {"US"}, true);
		Check(reversed && reversed.m_sTextOrName.Contains("lost"), "at-most requirement true announces faction loss, matching TilW");
		messages.m_sCaptured = "";
		Check(!messages.BuildMessage(0, null), "empty individual message suppressed");
		return string.Format("Capture checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestCaptureWorld()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunCaptureWorld();
	}

	string GC_TestCaptureMessage(int status)
	{
		GC_CaptureMessages messages = new GC_CaptureMessages();
		messages.m_sLocation = "Depot";
		GC_RuleAction action = messages.BuildMessage(status, {"US"});
		if (!action)
			return "No message for this status";
		if (!HasMissionRuleAuthority())
			return "Capture message fixture requires active Lobby2 authority";
		// Temporary runtime-only owner; restore the authored engine and enabled setting.
		GC_MissionRuleEngine previous = m_MissionRuleEngine;
		bool previousEnabled = m_bMissionEventsEnabled;
		GC_MissionRuleEngine fixture = new GC_MissionRuleEngine();
		m_MissionRuleEngine = fixture;
		m_bMissionEventsEnabled = true;
		fixture.Start({}, null, GetLobbyPhase());
		GC_MissionRuleEngine foreign = new GC_MissionRuleEngine();
		bool rejected = !GC_ExecuteFrameworkAction(foreign, action);
		bool accepted = GC_ExecuteFrameworkAction(m_MissionRuleEngine, action);
		fixture.Stop();
		m_MissionRuleEngine = previous;
		m_bMissionEventsEnabled = previousEnabled;
		return string.Format("capture message accepted=%1 foreign-engine rejected=%2 text=%3", accepted, rejected, action.m_sTextOrName);
	}

	string GC_TestCapture()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunCapture();
	}

	string GC_TestCaptureRegression()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunLivingCharacters();
	}
}
#endif
