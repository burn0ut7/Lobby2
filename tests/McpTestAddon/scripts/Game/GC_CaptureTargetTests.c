#ifdef WORKBENCH
modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunCaptureTargets()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!Replication.IsServer() || world.FindEntityByName("GC_TargetCaptureArea") || world.FindEntityByName("GC_TargetCaptureTruck") || world.FindEntityByName("GC_TargetCaptureCargo"))
			return "Capture targets fixture refused";
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "1190 100 2700";
		IEntity area = GetGame().SpawnEntityPrefab(Resource.Load("{2C4447C53A1E450B}Prefabs/Logic/Mission/RadiusArea.et"), world, parameters);
		IEntity truck = GetGame().SpawnEntityPrefab(Resource.Load("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et"), world, parameters);
		IEntity cargo = GetGame().SpawnEntityPrefab(Resource.Load("{B3283803059E2750}Prefabs/Props/Crates/CrateWooden_01_plain.et"), world, parameters);
		if (!area || !truck || !cargo)
		{
			if (area)
				SCR_EntityHelper.DeleteEntityAndChildren(area);
			if (truck)
				SCR_EntityHelper.DeleteEntityAndChildren(truck);
			if (cargo)
				SCR_EntityHelper.DeleteEntityAndChildren(cargo);
			return "Capture targets fixture unavailable";
		}
		area.SetName("GC_TargetCaptureArea");
		truck.SetName("GC_TargetCaptureTruck");
		cargo.SetName("GC_TargetCaptureCargo");
		GC_AreaCondition authored = new GC_AreaCondition();
		authored.m_sArea = area.GetName();
		GC_NamedAreaTargets targets = new GC_NamedAreaTargets();
		targets.m_aNames = {truck.GetName(), cargo.GetName(), truck.GetName()};
		authored.m_Targets = targets;
		GC_LivingProportion proportion = new GC_LivingProportion();
		proportion.m_fRequiredProportion = 1;
		authored.m_Requirement = proportion;
		authored.m_Capture = new GC_AreaCaptureOptions();
		authored.m_Capture.m_iCheckInterval = 1;
		authored.m_Capture.m_fSeconds = 2;
		authored.m_Capture.m_bSkipDiscovery = true;
		GC_RuleCondition check = authored.BuildCheck();
		Check(check && check.m_aAreaNames.Count() == 2, "mixed vehicle/object proportions compile and deduplicate");
		GC_MissionAreaTracker tracker = new GC_MissionAreaTracker(world);
		Check(tracker.Register(check), "mixed capture registers");
		array<GC_RuleCondition> changed = {};
		tracker.Sample(changed, true, 0);
		tracker.Sample(changed, false, 1);
		Check(!tracker.PassesCondition(check), "vehicle and object wait for duration");
		tracker.Sample(changed, false, 2);
		Check(tracker.PassesCondition(check), "vehicle and object capture together");
		cargo.SetOrigin("1220 100 2700");
		tracker.Sample(changed, false, 3);
		tracker.Sample(changed, false, 4);
		Check(!tracker.PassesCondition(check), "all-target proportion loses when cargo leaves");
		check.m_fAreaProportion = 0.5;
		tracker.Sample(changed, false, 5);
		tracker.Sample(changed, false, 6);
		Check(tracker.PassesCondition(check), "half-target proportion captures with truck alone");
		SCR_EntityHelper.DeleteEntityAndChildren(cargo);
		tracker.Sample(changed, false, 7);
		Check(!tracker.IsAvailable(check) && !tracker.PassesCondition(check), "missing named object cannot pass proportion");
		cargo = GetGame().SpawnEntityPrefab(Resource.Load("{B3283803059E2750}Prefabs/Props/Crates/CrateWooden_01_plain.et"), world, parameters);
		if (cargo)
			cargo.SetName("GC_TargetCaptureCargo");
		tracker.Sample(changed, false, 8);
		Check(cargo && tracker.IsAvailable(check), "replacement name is resolved without world scan");
		check.m_bAreaOutside = true;
		tracker.Sample(changed, false, 9);
		tracker.Sample(changed, false, 10);
		Check(!tracker.PassesCondition(check), "outside proportion shares capture loss timing");
		truck.SetOrigin("1220 100 2700");
		tracker.Sample(changed, false, 11);
		tracker.Sample(changed, false, 12);
		Check(tracker.PassesCondition(check), "outside proportion captures after vehicle exits");
		SCR_EntityHelper.DeleteEntityAndChildren(truck);
		if (cargo)
			SCR_EntityHelper.DeleteEntityAndChildren(cargo);
		SCR_EntityHelper.DeleteEntityAndChildren(area);
		return string.Format("Capture target checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCaptureTargets()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunCaptureTargets();
	}
}
#endif
