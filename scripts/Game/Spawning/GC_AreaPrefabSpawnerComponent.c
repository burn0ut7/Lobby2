[BaseContainerProps(), BaseContainerCustomStringTitleField("Prefab")]
class GC_AreaSpawnEntry
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to place. Its own components control replication, AI and other behavior.", "et")]
	ResourceName m_sPrefab;
	[Attribute("1", UIWidgets.SpinBox, "Number of this prefab. Maximum 128 objects across the entire spawner.", "1 128 1")]
	int m_iCount;
	[Attribute("0", UIWidgets.EditBox, "Optional clear-space radius in metres. Zero disables the check. Uses one stationary physics box trace per candidate, with this horizontal half-width and twice this height. This is an authored clearance approximation, not an exact prefab collision test.", "0 100")]
	float m_fClearanceRadius;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return !m_sPrefab.IsEmpty() && m_iCount >= 1 && m_iCount <= 128 && m_fClearanceRadius >= 0 && m_fClearanceRadius <= 100;
	}
}

[ComponentEditorProps(category: "GC Lobby/Spawning", description: "Standalone, server-only random prefab placement inside the owner's closed polyline. No game-mode or mission-framework dependency.")]
class GC_AreaPrefabSpawnerComponentClass : ScriptComponentClass
{
}

class GC_AreaPrefabSpawnerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Object, "Prefabs and counts to spawn, in order. Up to 32 entries and 128 total objects. Configure a child prefab to reuse this setup.")]
	protected ref array<ref GC_AreaSpawnEntry> m_aPrefabs;
	[Attribute("1", UIWidgets.CheckBox, "Spawn once after initialization during play. Disable to call Spawn() explicitly from a trusted server script. No spawning occurs in the editor.")]
	protected bool m_bSpawnOnInit;
	[Attribute("1", UIWidgets.CheckBox, "Reject water detected at the terrain sample. Finite river volumes above the bed may be missed. Disable for deliberate seabed placement; this does not float objects on water.")]
	protected bool m_bAvoidWater;
	[Attribute("1", UIWidgets.CheckBox, "Choose a random heading for each object. Unchecked uses the spawner's world heading.")]
	protected bool m_bRandomYaw;
	[Attribute("1", UIWidgets.CheckBox, "Align pitch and roll with the terrain surface while preserving the chosen heading. Unchecked keeps objects upright.")]
	protected bool m_bAlignToTerrain;
	[Attribute("0", UIWidgets.EditBox, "Vertical offset above the sampled terrain surface, in metres.", "-100 100")]
	protected float m_fHeightOffset;
	[Attribute("0", UIWidgets.EditBox, "Minimum horizontal distance between objects placed by this instance. Zero disables spacing. Existing world objects are handled by each entry's clearance radius.", "0 1000")]
	protected float m_fMinimumSpacing;
	[Attribute("50", UIWidgets.SpinBox, "Candidate attempts per requested object, including points rejected outside concave polygons. Maximum 4096 candidates per instance. Exhaustion reports a partial result; it never forces an invalid placement.", "1 256 1")]
	protected int m_iAttemptsPerObject;
	protected bool m_bAttempted;
	protected bool m_bRunning;
	protected int m_iRequested;
	protected int m_iAttempts;
	protected string m_sLastError;
	protected ref array<IEntity> m_aSpawned = {};
	protected ref array<vector> m_aPlacedPositions = {};

	//------------------------------------------------------------------------------------------------
	//! One attempt per instance. Repeated calls return the existing count without spawning again.
	//! -1 means authority/edit-mode/preflight rejection; a partial batch returns its actual count.
	int Spawn()
	{
		if (!GetGame() || !GetGame().InPlayMode() || !Replication.IsServer() || !GetOwner())
			return -1;
		if (m_bAttempted)
		{
			if (m_iRequested == 0)
				return -1;
			return m_aSpawned.Count();
		}
		m_bAttempted = true;
		GetGame().GetCallqueue().Remove(SpawnOnInit);
		GC_SpawnPolygon polygon = new GC_SpawnPolygon();
		array<ref Resource> resources = {};
		if (!Prepare(polygon, resources))
			return -1;
		m_bRunning = true;
		BaseWorld world = GetOwner().GetWorld();
		vector ownerTransform[4];
		GetOwner().GetWorldTransform(ownerTransform);
		vector ownerAngles = Math3D.MatrixToAngles(ownerTransform);
		for (int index; index < m_aPrefabs.Count(); index++)
		{
			GC_AreaSpawnEntry entry = m_aPrefabs[index];
			for (int item; item < entry.m_iCount; item++)
			{
				for (int attempt; attempt < m_iAttemptsPerObject && m_iAttempts < 4096; attempt++)
				{
					m_iAttempts++;
					vector position;
					if (!polygon.TryCandidate(position))
						continue;
					vector transform[4];
					if (!PreparePlacement(world, polygon, entry, position, ownerAngles[0], transform))
						continue;
					EntitySpawnParams parameters = new EntitySpawnParams();
					parameters.TransformMode = ETransformMode.WORLD;
					Math3D.MatrixCopy(transform, parameters.Transform);
					IEntity spawned = GetGame().SpawnEntityPrefab(resources[index], world, parameters);
					if (!spawned)
					{
						Fail("Engine failed to spawn " + entry.m_sPrefab + "; existing placements retained.");
						m_bRunning = false;
						return m_aSpawned.Count();
					}
					m_aSpawned.Insert(spawned);
					m_aPlacedPositions.Insert(transform[3]);
					break;
				}
			}
		}
		m_bRunning = false;
		if (m_aSpawned.Count() != m_iRequested)
			Fail(string.Format("Placed %1 of %2 after %3 candidates. Area, water, spacing or clearance rejected remaining placements.", m_aSpawned.Count(), m_iRequested, m_iAttempts));
		return m_aSpawned.Count();
	}

	//------------------------------------------------------------------------------------------------
	bool IsComplete()
	{
		return m_bAttempted && !m_bRunning && m_iRequested > 0 && m_aSpawned.Count() == m_iRequested && m_sLastError.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	int GetSpawnedCount()
	{
		return m_aSpawned.Count();
	}

	//------------------------------------------------------------------------------------------------
	int GetAttemptCount()
	{
		return m_iAttempts;
	}

	//------------------------------------------------------------------------------------------------
	string GetLastError()
	{
		return m_sLastError;
	}

	//------------------------------------------------------------------------------------------------
	//! Snapshot of created roots, some of which may since have been deleted by other systems.
	void GetSpawnedEntities(out array<IEntity> entities)
	{
		entities = {};
		entities.Copy(m_aSpawned);
	}

	//------------------------------------------------------------------------------------------------
	protected bool Prepare(GC_SpawnPolygon polygon, array<ref Resource> resources)
	{
		if (!m_aPrefabs || m_aPrefabs.IsEmpty() || m_aPrefabs.Count() > 32 || m_iAttemptsPerObject < 1 || m_iAttemptsPerObject > 256
			|| !(m_fMinimumSpacing >= 0 && m_fMinimumSpacing <= 1000) || !(m_fHeightOffset >= -100 && m_fHeightOffset <= 100))
			return Fail("Invalid settings or empty prefab list.");
		if (!polygon.Read(PolylineShapeEntity.Cast(GetOwner())))
			return Fail("Requires a closed, non-self-intersecting XZ polygon with 3-128 distinct points and nonzero area.");
		int total;
		foreach (GC_AreaSpawnEntry entry : m_aPrefabs)
		{
			if (!entry || !entry.IsValid())
				return Fail("Invalid prefab entry, count or clearance radius.");
			total += entry.m_iCount;
			if (total > 128)
				return Fail("More than 128 requested objects.");
			Resource resource = Resource.Load(entry.m_sPrefab);
			if (!resource || !resource.IsValid() || !resource.GetResource().ToEntitySource())
				return Fail("Unavailable entity prefab: " + entry.m_sPrefab);
			resources.Insert(resource);
		}
		m_iRequested = total;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool PreparePlacement(BaseWorld world, GC_SpawnPolygon polygon, GC_AreaSpawnEntry entry, vector position, float heading, out vector transform[4])
	{
		position[1] = SCR_TerrainHelper.GetTerrainY(position, world);
		if (m_bAvoidWater && ChimeraWorldUtils.TryGetWaterSurfaceSimple(world, position))
			return false;
		if (m_bRandomYaw)
			heading = Math.RandomFloat(0, 360);
		Math3D.AnglesToMatrix(Vector(heading, 0, 0), transform);
		transform[3] = position;
		if (m_bAlignToTerrain && !SCR_TerrainHelper.SnapAndOrientToTerrain(transform, world))
			return false;
		if (!polygon.Contains(transform[3]))
			return false;
		if (m_bAvoidWater && ChimeraWorldUtils.TryGetWaterSurfaceSimple(world, transform[3]))
			return false;
		transform[3][1] = transform[3][1] + m_fHeightOffset;
		foreach (vector previous : m_aPlacedPositions)
		{
			if (GC_SpawnPolygon.DistanceSquared(previous, transform[3]) < m_fMinimumSpacing * m_fMinimumSpacing)
				return false;
		}
		if (entry.m_fClearanceRadius > 0)
		{
			float radius = entry.m_fClearanceRadius;
			TraceBox trace = new TraceBox();
			trace.Start = transform[3];
			trace.Mins = Vector(-radius, 0.01, -radius);
			trace.Maxs = Vector(radius, radius * 2 + 0.01, radius);
			trace.Exclude = GetOwner();
			trace.Flags = TraceFlags.ENTS;
			trace.LayerMask = EPhysicsLayerDefs.Projectile;
			if (world.TracePosition(trace, null) < 0)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Fail(string reason)
	{
		m_sLastError = reason;
		Print("GC | Area prefab spawner | " + reason, LogLevel.WARNING);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnOnInit()
	{
		Spawn();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (m_bSpawnOnInit && GetGame().InPlayMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(SpawnOnInit, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(SpawnOnInit);
		super.OnDelete(owner);
	}
}
