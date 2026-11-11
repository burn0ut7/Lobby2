#ifdef WORKBENCH
modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunAIGroupConditions()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!Replication.IsServer() || world.FindEntityByName("GC_GroupTriggerFixture"))
			return "Group fixture refused";
		GC_AIGroupCondition continuous = new GC_AIGroupCondition();
		continuous.m_eMode = EGroupControlMode.AUTONOMOUS;
		Check(!continuous.BuildCheck(), "missing group name rejected");
		continuous.m_sGroup = "GC_GroupTriggerFixture";
		continuous.m_eMode = EGroupControlMode.LAST;
		Check(!continuous.BuildCheck(), "enum sentinel rejected");
		continuous.m_eMode = EGroupControlMode.AUTONOMOUS;
		continuous.m_bContinuous = true;
		GC_AIGroupCondition latch = new GC_AIGroupCondition();
		latch.m_sGroup = continuous.m_sGroup;
		latch.m_eMode = continuous.m_eMode;
		GC_Event liveRule = StateRule("group-live", SCR_EGameModeState.GAME);
		liveRule.m_aConditions = {continuous};
		liveRule.m_Advanced.m_bRepeatable = true;
		GC_Event stickyRule = StateRule("group-latch", SCR_EGameModeState.GAME);
		stickyRule.m_aConditions = {latch};
		stickyRule.m_Advanced.m_bRepeatable = true;
		GC_CompiledMissionRule live = liveRule.BuildRule();
		GC_CompiledMissionRule sticky = stickyRule.BuildRule();
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "group-native-flag";
		flag.m_aConditions = {continuous};
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({live, sticky}, {flag}, SCR_EGameModeState.GAME, null, null, world);
		engine.Advance(1);
		Check(engine.GetRunCount() == 0, "missing group fails closed and retries");
		Resource resource = Resource.Load("Prefabs/AI/Groups/Group_Base.et");
		IEntity entity = GetGame().SpawnEntityPrefab(resource, world);
		SCR_AIGroupInfoComponent info;
		if (entity)
			info = SCR_AIGroupInfoComponent.Cast(entity.FindComponent(SCR_AIGroupInfoComponent));
		if (!info)
		{
			engine.Stop();
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return "Native group fixture unavailable";
		}
		entity.SetName(continuous.m_sGroup);
		info.SetGroupControlMode(EGroupControlMode.IDLE);
		engine.Advance(2);
		Check(engine.GetRunCount() == 0, "late group binds with current nonmatching mode");
		info.SetGroupControlMode(EGroupControlMode.AUTONOMOUS);
		Check(engine.GetRunCount() == 2 && engine.GetFlag("group-native-flag"), "native callback fires both events and flag without a polling tick");
		info.SetGroupControlMode(EGroupControlMode.FOLLOWING_WAYPOINT);
		info.SetGroupControlMode(EGroupControlMode.AUTONOMOUS);
		Check(engine.GetRunCount() == 3, "continuous reenters but one-shot stays latched");
		engine.Advance(3);
		Check(engine.GetRunCount() == 3, "unchanged binding pass does not rerun events");
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 3);
		info.SetGroupControlMode(EGroupControlMode.IDLE);
		info.SetGroupControlMode(EGroupControlMode.AUTONOMOUS);
		Check(engine.GetRunCount() == 3, "postgame releases listeners");
		engine.Stop();
		engine.Start({live, sticky}, null, SCR_EGameModeState.BRIEFING, null, null, world);
		info.SetGroupControlMode(EGroupControlMode.IDLE);
		info.SetGroupControlMode(EGroupControlMode.AUTONOMOUS);
		Check(engine.GetRunCount() == 0, "briefing does not observe mode transitions");
		engine.SetGameModeState(SCR_EGameModeState.GAME, 0);
		Check(engine.GetRunCount() == 2, "match entry samples already matching group and resets latch");
		engine.Stop();

		// Verify availability and inversion directly using the production observer.
		GC_RuleCondition liveCheck = continuous.BuildCheck();
		GC_RuleCondition stickyCheck = latch.BuildCheck();
		GC_AIGroupObservation observation = new GC_AIGroupObservation(continuous.m_sGroup);
		observation.AddCondition(liveCheck);
		observation.AddCondition(stickyCheck);
		observation.Bind(world);
		Check(observation.Passes(liveCheck) && observation.Passes(stickyCheck), "current mode matches both observation styles");
		info.SetGroupControlMode(EGroupControlMode.IDLE);
		Check(!observation.Passes(liveCheck) && observation.Passes(stickyCheck), "continuous false and sticky true after leaving mode");
		SCR_EntityHelper.DeleteEntityAndChildren(entity);
		Check(!observation.IsAvailable(liveCheck) && observation.IsAvailable(stickyCheck) && observation.Passes(stickyCheck), "deletion unavailable for live check but retains confirmed latch");
		map<GC_RuleCondition, ref GC_AIGroupObservation> observations = new map<GC_RuleCondition, ref GC_AIGroupObservation>();
		observations.Set(liveCheck, observation);
		GC_RuleCondition none = new GC_RuleCondition();
		none.m_eGroupNeeds = GC_ERuleCheck.NONE;
		none.m_aChecks = {liveCheck};
		Check(!none.Evaluate(null, -1, null, null, null, null, null, null, null, null, observations), "missing group cannot pass inversion");
		entity = GetGame().SpawnEntityPrefab(resource, world);
		if (entity)
		{
			entity.SetName(continuous.m_sGroup);
			info = SCR_AIGroupInfoComponent.Cast(entity.FindComponent(SCR_AIGroupInfoComponent));
			info.SetGroupControlMode(EGroupControlMode.FOLLOWING_WAYPOINT);
			observation.Bind(world);
			Check(observation.IsAvailable(liveCheck) && !observation.Passes(liveCheck), "replacement group resolves by name");
			observation.Stop();
			info.SetGroupControlMode(EGroupControlMode.AUTONOMOUS);
			Check(!observation.IsAvailable(stickyCheck), "stop detaches native listener and clears latch");
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		else
			Check(false, "replacement spawned");
		return string.Format("AI group checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	string GC_TestAIGroupConditions()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunAIGroupConditions();
	}
}
#endif
