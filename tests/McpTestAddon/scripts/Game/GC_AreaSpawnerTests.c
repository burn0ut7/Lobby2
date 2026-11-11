#ifdef WORKBENCH
//! Optional explicit configuration helpers; no automatic test behavior or authority bypass.
modded class GC_AreaPrefabSpawnerComponent
{
	void GC_TestSettings(float spacing, bool water, bool align, bool randomYaw, float clearance = 0)
	{
		m_fMinimumSpacing = spacing;
		m_bAvoidWater = water;
		m_bAlignToTerrain = align;
		m_bRandomYaw = randomYaw;
		m_iAttemptsPerObject = 12;
		foreach (GC_AreaSpawnEntry entry : m_aPrefabs)
			entry.m_fClearanceRadius = clearance;
	}

	void GC_TestInvalidEntry()
	{
		m_aPrefabs[1].m_sPrefab = "";
	}
}

class GC_AreaSpawnerTests
{
	protected int m_iChecks;
	protected int m_iFailures;
	protected ref array<IEntity> m_aCleanup = {};

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string label)
	{
		m_iChecks++;
		if (!passed)
		{
			m_iFailures++;
			Print("GC | Area spawner test failed | " + label);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected GC_AreaPrefabSpawnerComponent Create(vector position, float yaw = 37)
	{
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), parameters.Transform);
		parameters.Transform[3] = position;
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load("{C01C382206EB40BB}Prefabs/AreaSpawner/Manual.et"), GetGame().GetWorld(), parameters);
		if (!entity)
			return null;
		m_aCleanup.Insert(entity);
		return GC_AreaPrefabSpawnerComponent.Cast(entity.FindComponent(GC_AreaPrefabSpawnerComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void Remember(GC_AreaPrefabSpawnerComponent component)
	{
		array<IEntity> spawned;
		component.GetSpawnedEntities(spawned);
		m_aCleanup.InsertAll(spawned);
	}

	//------------------------------------------------------------------------------------------------
	string Run()
	{
		if (!Replication.IsServer())
			return "Server required";
		GC_SpawnPolygon polygon = new GC_SpawnPolygon();
		Check(polygon.SetPoints({"0 0 0", "10 0 0", "10 0 10", "0 0 10"}), "square accepted");
		Check(polygon.Contains("5 100 5") && !polygon.Contains("15 0 5"), "XZ containment ignores altitude");
		Check(polygon.SetPoints({"0 0 0", "10 0 0", "10 0 10", "0 0 10", "0 0 0"}), "explicit closing point accepted");
		Check(!polygon.SetPoints({"0 0 0", "10 0 10", "0 0 10", "10 0 0"}), "crossed polygon rejected");
		Check(!polygon.SetPoints({"0 0 0", "5 0 0", "10 0 0"}), "zero area rejected");
		Check(!polygon.SetPoints({"0 0 0", "10 0 0", "10 0 0", "0 0 10"}), "duplicate adjacent point rejected");
		Check(polygon.SetPoints({"0 0 0", "10 0 0", "10 0 4", "4 0 4", "4 0 10", "0 0 10"}), "concave polygon accepted");
		bool candidatesValid = true;
		int candidates;
		for (int i; i < 100; i++)
		{
			vector point;
			if (polygon.TryCandidate(point))
			{
				candidates++;
				if (point[0] > 4 && point[2] > 4)
					candidatesValid = false;
			}
		}
		Check(candidates > 0 && candidatesValid, "bounded concave candidates avoid notch");
		GC_AreaSpawnEntry entry = new GC_AreaSpawnEntry();
		Check(!entry.IsValid(), "empty entry invalid");
		entry.m_sPrefab = "fixture";
		entry.m_iCount = 1;
		Check(entry.IsValid() && entry.m_iCount == 1, "valid entry settings");

		GC_AreaPrefabSpawnerComponent component = Create("1190 100 2700");
		Check(component != null, "configured child prefab loads standalone component");
		if (!component)
			return string.Format("Area spawner checks: %1 failures: %2", m_iChecks, m_iFailures);
		Check(component.GetSpawnedCount() == 0, "manual prefab does not spawn during construction");
		int created = component.Spawn();
		Remember(component);
		Check(created == 5 && component.IsComplete(), "per-entry counts spawn all five roots");
		Check(component.Spawn() == 5 && component.GetSpawnedCount() == 5, "second call is idempotent");
		Check(polygon.Read(PolylineShapeEntity.Cast(component.GetOwner())), "reads translated rotated world polygon");
		array<IEntity> objects;
		component.GetSpawnedEntities(objects);
		bool inside = true;
		bool grounded = true;
		bool spaced = true;
		bool rotated;
		foreach (int index, IEntity object : objects)
		{
			if (!object || !polygon.Contains(object.GetOrigin()))
				inside = false;
			if (object && Math.AbsFloat(object.GetOrigin()[1] - SCR_TerrainHelper.GetTerrainY(object.GetOrigin(), GetGame().GetWorld())) > 0.05)
				grounded = false;
			for (int j = index + 1; j < objects.Count(); j++)
			{
				if (GC_SpawnPolygon.DistanceSquared(object.GetOrigin(), objects[j].GetOrigin()) < 4)
					spaced = false;
			}
			if (object && Math.AbsFloat(object.GetYawPitchRoll()[0] - 37) > 0.1)
				rotated = true;
		}
		Check(inside && grounded, "all actual roots inside transformed polygon and on terrain");
		Check(spaced && rotated, "spacing and random yaw applied");
		GC_AreaPrefabSpawnerComponent limited = Create("1190 100 2700");
		limited.GC_TestSettings(1000, false, false, false);
		Check(limited.Spawn() == 1 && !limited.IsComplete() && !limited.GetLastError().IsEmpty(), "impossible spacing returns explicit partial result");
		Remember(limited);
		int attempts = limited.GetAttemptCount();
		Check(attempts <= 60 && limited.Spawn() == 1 && limited.GetAttemptCount() == attempts, "partial attempt bounded and never replayed");
		GC_AreaPrefabSpawnerComponent invalid = Create("1190 100 2700");
		invalid.GC_TestInvalidEntry();
		Check(invalid.Spawn() == -1 && invalid.GetSpawnedCount() == 0 && invalid.GetAttemptCount() == 0, "preflight rejects entire invalid batch before spawning");
		GC_AreaPrefabSpawnerComponent aligned = Create("1190 100 2700");
		aligned.GC_TestSettings(0, false, true, false);
		Check(aligned.Spawn() == 5, "terrain-aligned spawn succeeds");
		Remember(aligned);
		aligned.GetSpawnedEntities(objects);
		bool orientations = true;
		foreach (IEntity alignedObject : objects)
		{
			vector expected[4];
			Math3D.AnglesToMatrix("37 0 0", expected);
			expected[3] = alignedObject.GetOrigin();
			SCR_TerrainHelper.SnapAndOrientToTerrain(expected, GetGame().GetWorld());
			vector actual[4];
			alignedObject.GetWorldTransform(actual);
			if (vector.Distance(actual[1], expected[1]) > 0.01 || vector.Distance(actual[2], expected[2]) > 0.01)
				orientations = false;
		}
		Check(orientations, "native final transforms preserve chosen heading and terrain alignment");
		vector sea = "50 0 50";
		sea[1] = SCR_TerrainHelper.GetTerrainY(sea, GetGame().GetWorld());
		Check(ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), sea), "Arland sea fixture is actually underwater");
		GC_AreaPrefabSpawnerComponent water = Create(sea);
		water.GC_TestSettings(0, true, false, false);
		Check(water.Spawn() == 0 && !water.IsComplete() && water.GetAttemptCount() == 60, "water avoidance rejects every sea candidate");
		Remember(water);
		GC_AreaPrefabSpawnerComponent submerged = Create(sea);
		submerged.GC_TestSettings(0, false, false, false);
		Check(submerged.Spawn() == 5, "water permitted explicitly places on seabed");
		Remember(submerged);

		EntitySpawnParams blockerParameters = new EntitySpawnParams();
		blockerParameters.TransformMode = ETransformMode.WORLD;
		blockerParameters.Transform[3] = Vector(1190, SCR_TerrainHelper.GetTerrainY("1190 0 2700", GetGame().GetWorld()) + 50, 2700);
		IEntity blocker = GetGame().SpawnEntityPrefab(Resource.Load("{B3283803059E2750}Prefabs/Props/Crates/CrateWooden_01_plain.et"), GetGame().GetWorld(), blockerParameters);
		Check(blocker && blocker.GetPhysics(), "clearance fixture has native physics");
		if (blocker)
			m_aCleanup.Insert(blocker);
		GC_AreaPrefabSpawnerComponent blocked = Create("1190 100 2700");
		blocked.GC_TestSettings(0, false, false, false, 100);
		Check(blocked.Spawn() == 0 && !blocked.IsComplete(), "physics clearance rejects obstructed placements");
		Remember(blocked);
		for (int cleanup = m_aCleanup.Count() - 1; cleanup >= 0; cleanup--)
		{
			if (m_aCleanup[cleanup])
				SCR_EntityHelper.DeleteEntityAndChildren(m_aCleanup[cleanup]);
		}
		m_aCleanup.Clear();
		return string.Format("Area spawner checks: %1 failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	protected IEntity m_GC_AutomaticSpawnerTest;

	//------------------------------------------------------------------------------------------------
	string GC_TestAreaSpawner()
	{
		GC_AreaSpawnerTests tests = new GC_AreaSpawnerTests();
		return tests.Run();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAreaSpawnerAutoStart()
	{
		if (m_GC_AutomaticSpawnerTest || !Replication.IsServer())
			return "Already running or not server";
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "1190 100 2700";
		m_GC_AutomaticSpawnerTest = GetGame().SpawnEntityPrefab(Resource.Load("{D6ECB3EFB79F4EFC}Prefabs/AreaSpawner/Automatic.et"), GetGame().GetWorld(), parameters);
		return string.Format("automatic spawner created=%1", m_GC_AutomaticSpawnerTest != null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAreaSpawnerAutoFinish()
	{
		if (!m_GC_AutomaticSpawnerTest)
			return "No fixture";
		GC_AreaPrefabSpawnerComponent component = GC_AreaPrefabSpawnerComponent.Cast(m_GC_AutomaticSpawnerTest.FindComponent(GC_AreaPrefabSpawnerComponent));
		array<IEntity> spawned;
		component.GetSpawnedEntities(spawned);
		string report = string.Format("automatic count=%1 complete=%2 repeat=%3", component.GetSpawnedCount(), component.IsComplete(), component.Spawn());
		SCR_EntityHelper.DeleteEntityAndChildren(m_GC_AutomaticSpawnerTest);
		m_GC_AutomaticSpawnerTest = null;
		int survivors;
		foreach (IEntity entity : spawned)
		{
			if (entity)
			{
				survivors++;
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}
		}
		return report + string.Format(" retained-after-owner-deletion=%1", survivors);
	}
}
#endif
