//! Optional validation fixtures. No spawning, automatic execution or Play entry.
class GC_SpawnActionTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunSpawnValidation()
	{
		GC_SpawnAction action = new GC_SpawnAction();
		Check(!action.BuildAction().IsValid(), "Missing spawn choice is rejected");
		GC_PrefabSpawn prefab = new GC_PrefabSpawn();
		action.m_Spawn = prefab;
		prefab.m_sPrefab = "test.et";
		Check(!action.BuildAction().IsValid(), "Prefab requires a named location");
		prefab.m_sLocation = "reinforcements";
		Check(action.BuildAction().IsValid(), "Plain prefab needs no AI settings");
		Check(action.BuildAction().m_sTextOrName == "reinforcements", "Failure diagnostics identify location");
		GC_AIGroupSpawn group = new GC_AIGroupSpawn();
		group.m_sPrefab = "test.et";
		group.m_sLocation = "reinforcements";
		group.m_aWaypoints = {""};
		Check(!group.IsValid(), "Empty waypoint names rejected");
		group.m_aWaypoints = {};
		Check(group.IsValid(), "AI group can keep prefab orders");
		GC_CrewedVehicleSpawn vehicle = new GC_CrewedVehicleSpawn();
		vehicle.m_sPrefab = "test.et";
		vehicle.m_sLocation = "reinforcements";
		Check(vehicle.IsValid(), "Empty crew list uses default crew");
		GC_SpawnCrewGroup crew = new GC_SpawnCrewGroup();
		Check(crew.m_bDriver && crew.m_bGunners && crew.m_bPassengers, "Script defaults fill all seat types");
		crew.m_bDriver = false;
		crew.m_bGunners = false;
		crew.m_bPassengers = false;
		vehicle.m_aCrewGroups = {crew};
		Check(!vehicle.IsValid(), "Crew group with no seat types rejected");
		crew.m_bGunners = true;
		crew.m_aCharacters = {""};
		Check(vehicle.IsValid(), "Blank custom character is an intentional empty seat");
		return string.Format("SpawnValidation: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}
