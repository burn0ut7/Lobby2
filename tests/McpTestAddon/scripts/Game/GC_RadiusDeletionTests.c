#ifdef WORKBENCH
modded class GC_MissionRuleTests
{
	protected ref array<IEntity> m_aRadiusFixtures = {};
	protected vector m_vRadiusFixtureOrigin = "3000 1000 3000";

	//------------------------------------------------------------------------------------------------
	protected IEntity RadiusFixture(string prefab, vector offset)
	{
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = m_vRadiusFixtureOrigin + offset;
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), parameters);
		if (entity)
			m_aRadiusFixtures.Insert(entity);
		return entity;
	}

	//------------------------------------------------------------------------------------------------
	string RunRadiusDeletion(GC_LobbyManager manager, GC_MissionRuleEngine engine)
	{
		BaseWorld world = GetGame().GetWorld();
		if (world.FindEntityByName("GC_RadiusDeleteTestCenter"))
			return "REFUSED existing fixture name";
		IEntity center = RadiusFixture("{F71C000000000044}Prefabs/RadiusDeletion/Other.et", "0 0 0");
		IEntity inside = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "2 0 0");
		IEntity boundary = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "10 0 0");
		IEntity corner = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "9 0 9");
		IEntity above = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "0 11 0");
		IEntity keep = RadiusFixture("{F71C000000000043}Prefabs/RadiusDeletion/Protected.et", "3 0 0");
		IEntity other = RadiusFixture("{F71C000000000044}Prefabs/RadiusDeletion/Other.et", "4 0 0");
		if (!center || !inside || !boundary || !corner || !above || !keep || !other)
		{
			foreach (IEntity fixture : m_aRadiusFixtures)
				SCR_EntityHelper.DeleteEntityAndChildren(fixture);
			return "UNAVAILABLE fixture prefab";
		}
		center.SetName("GC_RadiusDeleteTestCenter");
		IEntitySource source = Resource.Load("{F71C000000000041}Prefabs/RadiusDeletion/Base.et").GetResource().ToEntitySource();
		bool hierarchy = false;
		for (int componentIndex = 0; componentIndex < source.GetComponentCount(); componentIndex++)
			hierarchy = hierarchy || source.GetComponent(componentIndex).GetClassName() == "Hierarchy";
		Check(!hierarchy && !inside.FindComponent(RplComponent) && !inside.GetParent(), "fixture has neither hierarchy nor replication component");
		GC_DeleteEntitiesAction authored = new GC_DeleteEntitiesAction();
		authored.m_Radius = new GC_RadiusDeletion();
		authored.m_Radius.m_sCenter = center.GetName();
		GC_RuleAction action = authored.BuildAction();
		Check(action.IsValid(), "empty whitelist and blacklist valid");
		array<IEntity> selected = {};
		GC_RadiusDeletionQuery query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && selected.Contains(inside) && selected.Contains(other) && selected.Contains(keep), "unfiltered query selects unrelated prefab types without hierarchy");
		Check(selected.Contains(boundary) && !selected.Contains(corner) && !selected.Contains(above) && !selected.Contains(center), "exact inclusive 3D radius and protected center");
		authored.m_Radius.m_aWhitelist = {"{F71C000000000041}Prefabs/RadiusDeletion/Base.et"};
		authored.m_Radius.m_aBlacklist = {"{F71C000000000043}Prefabs/RadiusDeletion/Protected.et"};
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && selected.Count() == 2 && selected.Contains(inside) && selected.Contains(boundary), "prefab ancestor whitelist and blacklist precedence");
		authored.m_Radius.m_aBlacklist = {""};
		Check(!manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()) && inside && keep, "invalid filter refuses before deletion");
		authored.m_Radius.m_aBlacklist = {"{F71C000000000043}Prefabs/RadiusDeletion/Protected.et"};
		Check(!manager.GC_ExecuteFrameworkAction(null, authored.BuildAction()) && inside, "missing engine source refuses without deletion");
		EntityID insideId = inside.GetID();
		EntityID boundaryId = boundary.GetID();
		Check(manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()), "radius deletion accepted through production handler");
		Check(!world.FindEntityByID(insideId) && !world.FindEntityByID(boundaryId) && keep && other && corner && above && center, "only eligible objects removed");
		Check(manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()), "repeated deletion is harmless");
		authored.m_Radius.m_aWhitelist = null;
		authored.m_Radius.m_aBlacklist = null;
		EntityID keepId = keep.GetID();
		EntityID otherId = other.GetID();
		Check(manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()) && !world.FindEntityByID(keepId) && !world.FindEntityByID(otherId), "no filters required for actual deletion");
		GC_RadiusDeletionRecord record = new GC_RadiusDeletionRecord();
		record.Capture(corner);
		record.m_sPrefab = "wrong prefab";
		record.Apply(world);
		Check(corner != null, "replica mismatch guard preserves unrelated object");
		record.Capture(corner);
		EntityID cornerId = corner.GetID();
		record.Apply(world);
		Check(!world.FindEntityByID(cornerId), "replica/JIP removal record deletes matching nonreplicated object");
		authored.m_Radius.m_fRadius = 0;
		Check(!authored.BuildAction().IsValid(), "zero radius rejected");
		IEntity building = RadiusFixture("{F71C000000000045}Prefabs/RadiusDeletion/Building.et", "50 0 0");
		vector mins, maxs;
		building.GetBounds(mins, maxs);
		vector middle = (mins + maxs) * 0.5;
		IEntity furniture = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "50 0 0" + middle);
		IEntity outside = RadiusFixture("{F71C000000000044}Prefabs/RadiusDeletion/Other.et", "50 0 0" + maxs + "5 0 5");
		center.SetOrigin(building.GetOrigin() + "0 -10 0");
		authored.m_Radius.m_fRadius = 10;
		authored.m_Radius.m_aWhitelist = {"{F71C000000000045}Prefabs/RadiusDeletion/Building.et"};
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && selected.Contains(building) && selected.Contains(furniture) && !selected.Contains(outside) && !furniture.GetParent(), "building includes unattached interior outside radius and whitelist");
		authored.m_Radius.m_aWhitelist = null;
		authored.m_Radius.m_aBlacklist = {"{F71C000000000045}Prefabs/RadiusDeletion/Building.et"};
		center.SetOrigin(furniture.GetOrigin());
		authored.m_Radius.m_fRadius = 0.5;
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && !selected.Contains(furniture) && !selected.Contains(building), "blacklisted building outside radius protects loose furniture inside radius");
		authored.m_Radius.m_aBlacklist = null;
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && selected.Contains(furniture) && !selected.Contains(center), "loose center inside building does not protect surrounding furniture");
		center.SetOrigin(building.GetOrigin() + "0 -10 0");
		authored.m_Radius.m_fRadius = 10;
		authored.m_Radius.m_aBlacklist = {"{F71C000000000041}Prefabs/RadiusDeletion/Base.et"};
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		Check(query.Collect(authored.m_Radius, manager.GetOwner(), selected) && !selected.Contains(building) && !selected.Contains(furniture), "blacklisted loose furniture protects entire building group");
		authored.m_Radius.m_aBlacklist = null;
		authored.m_Radius.m_aWhitelist = {"{F71C000000000045}Prefabs/RadiusDeletion/Building.et"};
		EntityID buildingId = building.GetID();
		EntityID furnitureId = furniture.GetID();
		Check(manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()) && !world.FindEntityByID(buildingId) && !world.FindEntityByID(furnitureId) && outside, "production deletion removes building and loose furniture together");
		IEntity nativeBuilding = RadiusFixture("{C3BCE9690AC14EA4}Prefabs/Structures/Agriculture/Barn/Barn_E_01/Barn_E_01_Base.et", "100 0 0");
		nativeBuilding.SetAngles("45 0 0");
		vector transform[4];
		nativeBuilding.GetWorldTransform(transform);
		IEntity nativeInterior = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", Vector(6, 2, 0).Multiply4(transform) - m_vRadiusFixtureOrigin);
		IEntity nativeExterior = RadiusFixture("{F71C000000000044}Prefabs/RadiusDeletion/Other.et", Vector(0, 2, 7).Multiply4(transform) - m_vRadiusFixtureOrigin);
		GC_BuildingDeletionGroup nativeGroup = new GC_BuildingDeletionGroup();
		int nativeBudget = 16384;
		Check(nativeGroup.Collect(nativeBuilding, nativeBudget) && nativeGroup.m_aContents.Contains(nativeInterior) && !nativeGroup.m_aContents.Contains(nativeExterior), "stock building configured interior volumes respect rotation");
		center.SetOrigin(nativeBuilding.GetOrigin() + "0 -10 0");
		authored.m_Radius.m_aWhitelist = {"{C3BCE9690AC14EA4}Prefabs/Structures/Agriculture/Barn/Barn_E_01/Barn_E_01_Base.et"};
		EntityID nativeId = nativeBuilding.GetID();
		EntityID nativeInteriorId = nativeInterior.GetID();
		Check(manager.GC_ExecuteFrameworkAction(engine, authored.BuildAction()) && !world.FindEntityByID(nativeId) && !world.FindEntityByID(nativeInteriorId) && nativeExterior, "stock building and loose interior removed through production handler");
		IEntity overlapA = RadiusFixture("{F71C000000000045}Prefabs/RadiusDeletion/Building.et", "200 0 0");
		IEntity overlapB = RadiusFixture("{F71C000000000046}Prefabs/RadiusDeletion/ProtectedBuilding.et", "210 0 0");
		IEntity shared = RadiusFixture("{F71C000000000042}Prefabs/RadiusDeletion/Variant.et", "205 2 0");
		center.SetOrigin(overlapA.GetOrigin() + "0 -1 0");
		authored.m_Radius.m_fRadius = 1;
		authored.m_Radius.m_aWhitelist = {"{F71C000000000045}Prefabs/RadiusDeletion/Building.et"};
		authored.m_Radius.m_aBlacklist = {"{F71C000000000046}Prefabs/RadiusDeletion/ProtectedBuilding.et"};
		selected.Clear();
		query = new GC_RadiusDeletionQuery();
		bool overlapCollected = query.Collect(authored.m_Radius, manager.GetOwner(), selected);
		Check(overlapCollected && !selected.Contains(overlapA) && !selected.Contains(overlapB) && !selected.Contains(shared), string.Format("overlap protection collected=%1 A=%2 B=%3 shared=%4", overlapCollected, selected.Contains(overlapA), selected.Contains(overlapB), selected.Contains(shared)));
		foreach (IEntity fixture : m_aRadiusFixtures)
		{
			if (fixture)
				SCR_EntityHelper.DeleteEntityAndChildren(fixture);
		}
		return string.Format("Radius deletion checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestRadiusDeletion()
	{
		if (!HasMissionRuleAuthority())
			return "UNAVAILABLE active Lobby2 server";
		GC_MissionRuleEngine previous = m_MissionRuleEngine;
		bool enabled = m_bMissionEventsEnabled;
		int previousRecords = m_aGC_RadiusRemovals.Count();
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		m_MissionRuleEngine = engine;
		m_bMissionEventsEnabled = true;
		engine.Start({}, null, GetLobbyPhase(), null, null, GetGame().GetWorld(), this);
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		string result = tests.RunRadiusDeletion(this, engine);
		engine.Stop();
		m_MissionRuleEngine = previous;
		m_bMissionEventsEnabled = enabled;
		// Runtime fixtures are local to this disposable test; do not retain their IDs for joining peers.
		while (m_aGC_RadiusRemovals.Count() > previousRecords)
			m_aGC_RadiusRemovals.RemoveOrdered(m_aGC_RadiusRemovals.Count() - 1);
		return result;
	}
}
#endif
