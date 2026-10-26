class GC_MissionSpawnClearance
{
	bool m_bBlocked;

	//------------------------------------------------------------------------------------------------
	bool Check(IEntity entity)
	{
		if (!entity.GetParent() && entity.GetPhysics())
		{
			m_bBlocked = true;
			return false;
		}
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Prefabs In Area")]
class GC_SpawnPrefabsInAreaAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Name of a center entity or polygon polyline.")]
	string m_sArea;
	[Attribute("25", UIWidgets.EditBox, "Radius in meters if the area is not a polygon.", params: "0.1 10000 1")]
	float m_fRadius;
	[Attribute("", UIWidgets.ResourceAssignArray, "Prefabs to distribute over the terrain.", params: "et")]
	ref array<ResourceName> m_aPrefabs;
	[Attribute("1", UIWidgets.EditBox, "How many of each prefab. At most 1000 total per action.", params: "1 1000 1")]
	int m_iCount;
	[Attribute("", UIWidgets.EditBox, "Optional unique name prefix. Names become prefix_1, prefix_2, and so on.")]
	string m_sNamePrefix;
	[Attribute("1", UIWidgets.CheckBox, "Avoid water.")]
	bool m_bAvoidWater;
	[Attribute("1", UIWidgets.CheckBox, "Randomize heading.")]
	bool m_bRandomHeading;
	[Attribute("1", UIWidgets.CheckBox, "Align to the ground slope.")]
	bool m_bAlignToGround;
	[Attribute("0", UIWidgets.EditBox, "Optional clearance radius around each position. Zero disables collision checks. Choose a radius suitable for the prefab's size.", params: "0 1000 0.1")]
	float m_fClearance;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_sArea.IsEmpty() || m_fRadius <= 0 || m_fRadius > 10000 || !m_aPrefabs || m_aPrefabs.IsEmpty()
			|| m_iCount < 1 || m_iCount > 1000 || m_aPrefabs.Count() > 1000 / m_iCount || m_fClearance < 0 || m_fClearance > 1000)
		{
			error = "Spawn Prefabs In Area: choose an area, prefabs, positive radius and up to 1000 total spawns.";
			return false;
		}
		foreach (ResourceName name : m_aPrefabs)
		{
			Resource prefab = Resource.Load(name);
			if (!prefab || !prefab.IsValid())
			{
				error = "Spawn Prefabs In Area: invalid prefab " + name;
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		IEntity area = GetGame().GetWorld().FindEntityByName(m_sArea);
		if (!context.IsActive() || !area)
			return false;
		int total = m_iCount * m_aPrefabs.Count();
		if (!m_sNamePrefix.IsEmpty())
		{
			for (int i = 1; i <= total; i++)
			{
				if (GetGame().GetWorld().FindEntityByName(m_sNamePrefix + "_" + i))
					return false;
			}
		}
		array<vector> polygon = {};
		bool polygonArea = GC_MissionArea.WorldPoints(area, polygon);
		vector minimum = area.GetOrigin() - Vector(m_fRadius, 0, m_fRadius);
		vector maximum = area.GetOrigin() + Vector(m_fRadius, 0, m_fRadius);
		if (polygonArea)
		{
			minimum = polygon[0];
			maximum = polygon[0];
			foreach (vector point : polygon)
			{
				minimum = SCR_Math3D.Min(minimum, point);
				maximum = SCR_Math3D.Max(maximum, point);
			}
		}
		int spawned;
		int attempts;
		// Native GetRandomPointInPolygon retries without a limit on degenerate polygons.
		// Bound rejection sampling so invalid authoring cannot hang the mission.
		while (spawned < total && attempts++ < total * 100 && context.IsActive())
		{
			vector position = Vector(Math.RandomFloat(minimum[0], maximum[0]), area.GetOrigin()[1], Math.RandomFloat(minimum[2], maximum[2]));
			if (polygonArea)
			{
				if (!Math2D.IsPointInPolygonXZ(polygon, position))
					continue;
			}
			else
			{
				// Uniform disk sampling does not discard valid attempts at the circle's corners.
				float angle = Math.RandomFloat(0, Math.PI2);
				float distance = m_fRadius * Math.Sqrt(Math.RandomFloat01());
				position = area.GetOrigin() + Vector(Math.Cos(angle) * distance, 0, Math.Sin(angle) * distance);
			}
			position[1] = SCR_TerrainHelper.GetTerrainY(position, null, true);
			if (m_bAvoidWater && ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), position))
				continue;
			if (m_fClearance > 0)
			{
				GC_MissionSpawnClearance clearance = new GC_MissionSpawnClearance();
				GetGame().GetWorld().QueryEntitiesBySphere(position, m_fClearance, clearance.Check);
				if (clearance.m_bBlocked)
					continue;
			}
			float heading;
			if (m_bRandomHeading)
				heading = Math.RandomFloat(0, 360);
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			Math3D.AnglesToMatrix(Vector(heading, 0, 0), params.Transform);
			params.Transform[3] = position;
			if (m_bAlignToGround && !SCR_TerrainHelper.SnapAndOrientToTerrain(params.Transform, GetGame().GetWorld()))
				continue;
			IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(m_aPrefabs[spawned / m_iCount]), GetGame().GetWorld(), params);
			if (!entity)
				break;
			spawned++;
			if (!m_sNamePrefix.IsEmpty())
				entity.SetName(m_sNamePrefix + "_" + spawned);
		}
		PrintFormat("GC | Mission Framework | area spawn: area=%1 spawned=%2 requested=%3 attempts=%4", m_sArea, spawned, total, attempts);
		return spawned == total;
	}
}
