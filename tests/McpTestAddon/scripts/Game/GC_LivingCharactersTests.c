//! Optional production-logic checks; run explicitly, never from production startup.
modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunLivingCharacters()
	{
		GC_LivingCharactersCondition condition = new GC_LivingCharactersCondition();
		condition.m_aFactions = {"US", "US"};
		GC_RuleCondition anywhere = condition.BuildCheck();
		Check(anywhere && anywhere.m_bAreaAnywhere && anywhere.UsesArea(), "anywhere uses shared population scheduler");
		Check(anywhere.m_aAreaFactions.Count() == 1, "duplicate factions count once");
		GC_AreaCount count = new GC_AreaCount();
		count.m_iComparison = 1;
		count.m_iRequiredCount = 5;
		condition.m_Requirement = count;
		GC_RuleCondition counted = condition.BuildCheck();
		Check(counted && counted.m_iAreaComparison == 1 && counted.m_iAreaRequired == 5, "living count below five builds without an area");
		GC_LivingProportion proportion = new GC_LivingProportion();
		proportion.m_fRequiredProportion = 0.9;
		condition.m_Requirement = proportion;
		Check(!condition.BuildCheck(), "proportion requires an area");
		condition.m_Area = new GC_LivingCharacterArea();
		Check(!condition.BuildCheck(), "blank area is invalid");
		condition.m_Area.m_sName = "Extraction";
		GC_RuleCondition extraction = condition.BuildCheck();
		Check(extraction && extraction.m_bAreaProportion && !extraction.m_bAreaAnywhere, "extraction builds one condition leaf");
		Check(!extraction.Evaluate(null), "missing observation cannot pass");
		GC_RuleCondition none = new GC_RuleCondition();
		none.m_eGroupNeeds = GC_ERuleCheck.NONE;
		none.m_aChecks = {extraction};
		Check(!none.Evaluate(null), "None does not turn unavailable extraction into true");
		GC_MissionAreaTracker tracker = new GC_MissionAreaTracker(null);
		Check(tracker.Register(extraction), "extraction observation registers");
		condition.m_Requirement = count;
		GC_RuleCondition areaCount = condition.BuildCheck();
		Check(tracker.Register(areaCount), "area count registers");
		Check(tracker.GetObservation(extraction) == tracker.GetObservation(areaCount), "count and proportion share one observation");
		Check(tracker.Register(anywhere), "anywhere registers without geometry");
		Check(tracker.GetObservation(anywhere) != tracker.GetObservation(extraction), "anywhere does not share area-limited counts");
		Check(GC_MissionAreaObservation.PassesProportion(9, 10, 0.9), "nine of ten reaches ninety percent");
		Check(!GC_MissionAreaObservation.PassesProportion(8, 9, 0.9), "eight of nine is below ninety percent");
		Check(GC_MissionAreaObservation.PassesProportion(9, 9, 0.9), "nine of nine reaches ninety percent");
		Check(!GC_MissionAreaObservation.PassesProportion(0, 0, 0), "empty population fails even at zero");
		Check(GC_MissionAreaObservation.PassesProportion(0, 10, 0), "zero threshold passes a nonempty population");
		Check(!GC_MissionAreaObservation.PassesProportion(9, 10, 1), "everyone requires all ten");
		Check(GC_MissionAreaObservation.PassesProportion(10, 10, 1), "all ten reaches everyone");
		Check(!GC_MissionAreaObservation.PassesProportion(11, 10, 0.9), "impossible counts rejected");
		condition.m_Requirement = proportion;
		proportion.m_fRequiredProportion = 1.1;
		Check(!condition.BuildCheck(), "ratio above one rejected");
		proportion.m_fRequiredProportion = -0.1;
		Check(!condition.BuildCheck(), "negative ratio rejected");
		proportion.m_fRequiredProportion = 0.9;
		condition.m_iWho = 3;
		Check(!condition.BuildCheck(), "unknown population rejected");
		condition.m_iWho = 0;
		condition.m_aFactions = {""};
		Check(!condition.BuildCheck(), "blank faction rejected");
		return string.Format("Living Characters checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}
