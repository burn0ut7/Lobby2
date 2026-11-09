//! Shared weak entity reference, resolved at most once per sample while missing.
class GC_AreaEntityReference
{
	string m_sName;
	IEntity m_Entity;
	vector m_vPosition;
	protected bool m_bReportedMissing;

	//------------------------------------------------------------------------------------------------
	void Sample(BaseWorld world)
	{
		if (!m_Entity && world)
			m_Entity = world.FindEntityByName(m_sName);
		if (m_Entity)
			m_vPosition = m_Entity.GetOrigin();
		else if (!m_bReportedMissing && world)
		{
			Print("GC | Mission Framework | Area Condition cannot find '" + m_sName + "'. Retrying on the next scheduled check.", LogLevel.WARNING);
			m_bReportedMissing = true;
		}
	}
}

//! Shared shape cache; no gameplay ownership or spatial queries live on the area entity.
class GC_MissionAreaGeometry
{
	ref GC_AreaEntityReference m_Reference;
	protected IEntity m_CachedEntity;
	protected ref array<vector> m_aLocalPoints = {};
	protected ref array<vector> m_aWorldPoints = {};
	protected vector m_aTransform[4];
	protected vector m_vMinimum;
	protected vector m_vMaximum;
	protected float m_fRadiusSquared;
	protected bool m_bRadius;
	protected bool m_bValid;
	protected bool m_bReportedInvalid;

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_bValid && m_Reference.m_Entity;
	}

	//------------------------------------------------------------------------------------------------
	void Sample()
	{
		IEntity entity = m_Reference.m_Entity;
		if (!entity)
		{
			m_bValid = false;
			m_CachedEntity = null;
			return;
		}
		GC_RadiusAreaEntity radius = GC_RadiusAreaEntity.Cast(entity);
		m_bRadius = radius != null;
		if (radius)
		{
			float metres = radius.GetRadius();
			m_bValid = metres >= 0.1 && metres <= 100000;
			m_fRadiusSquared = metres * metres;
		}
		else
		{
			PolylineShapeEntity polygon = PolylineShapeEntity.Cast(entity);
			if (!polygon || !polygon.IsClosed())
				m_bValid = false;
			else
			{
				vector transform[4];
				entity.GetWorldTransform(transform);
				bool rebuild = m_CachedEntity != entity;
				if (rebuild)
				{
					m_aLocalPoints.Clear();
					polygon.GetPointsPositions(m_aLocalPoints);
				}
				for (int axis; axis < 4; axis++)
				{
					if (m_aTransform[axis] != transform[axis])
						rebuild = true;
					m_aTransform[axis] = transform[axis];
				}
				if (rebuild)
					BuildPolygon(entity);
			}
		}
		m_CachedEntity = entity;
		if (!m_bValid && !m_bReportedInvalid)
		{
			Print("GC | Mission Framework | Invalid area '" + m_Reference.m_sName + "'. Use Radius Area or a closed polygon with nonzero horizontal area with 3-128 points.", LogLevel.WARNING);
			m_bReportedInvalid = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPolygon(IEntity entity)
	{
		m_bValid = false;
		m_aWorldPoints.Clear();
		if (m_aLocalPoints.Count() < 3 || m_aLocalPoints.Count() > 128)
			return;
		foreach (vector localPoint : m_aLocalPoints)
			m_aWorldPoints.Insert(entity.CoordToParent(localPoint));
		m_vMinimum = m_aWorldPoints[0];
		m_vMaximum = m_vMinimum;
		float twiceArea;
		for (int i; i < m_aWorldPoints.Count(); i++)
		{
			vector point = m_aWorldPoints[i];
			vector a = point - m_aWorldPoints[0];
			vector b = m_aWorldPoints[(i + 1) % m_aWorldPoints.Count()] - m_aWorldPoints[0];
			twiceArea += a[0] * b[2] - b[0] * a[2];
			m_vMinimum[0] = Math.Min(m_vMinimum[0], point[0]);
			m_vMinimum[2] = Math.Min(m_vMinimum[2], point[2]);
			m_vMaximum[0] = Math.Max(m_vMaximum[0], point[0]);
			m_vMaximum[2] = Math.Max(m_vMaximum[2], point[2]);
		}
		m_bValid = Math.AbsFloat(twiceArea) > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	bool Contains(vector position)
	{
		if (!IsAvailable())
			return false;
		if (m_bRadius)
			return vector.DistanceSq(m_Reference.m_vPosition, position) <= m_fRadiusSquared;
		if (position[0] < m_vMinimum[0] || position[0] > m_vMaximum[0] || position[2] < m_vMinimum[2] || position[2] > m_vMaximum[2])
			return false;
		return Math2D.IsPointInPolygonXZ(m_aWorldPoints, position);
	}
}

//! One result for each unique target/area pair, shared by Inside and Outside leaves.
class GC_MissionAreaObservation
{
	ref GC_AreaEntityReference m_Target;
	ref GC_MissionAreaGeometry m_Area;
	bool m_bChanged;
	protected bool m_bAvailable;
	protected bool m_bInside;
	protected bool m_bSampled;

	//------------------------------------------------------------------------------------------------
	void Sample()
	{
		bool available = m_Target.m_Entity && m_Area.IsAvailable();
		bool inside = available && m_Area.Contains(m_Target.m_vPosition);
		m_bChanged = !m_bSampled || available != m_bAvailable || inside != m_bInside;
		m_bSampled = true;
		m_bAvailable = available;
		m_bInside = inside;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_bAvailable && m_Target.m_Entity && m_Area.IsAvailable();
	}

	//------------------------------------------------------------------------------------------------
	bool Passes(bool outside)
	{
		return IsAvailable() && m_bInside != outside;
	}
}

//! Owned by the server-local interpreter. No frame callback, spatial query, or client replication.
class GC_MissionAreaTracker
{
	protected BaseWorld m_World;
	protected ref map<string, ref GC_AreaEntityReference> m_mEntities = new map<string, ref GC_AreaEntityReference>();
	protected ref map<string, ref GC_MissionAreaGeometry> m_mAreas = new map<string, ref GC_MissionAreaGeometry>();
	protected ref array<ref GC_MissionAreaObservation> m_aPairs = {};
	protected ref map<GC_RuleCondition, ref GC_MissionAreaObservation> m_mChecks = new map<GC_RuleCondition, ref GC_MissionAreaObservation>();

	//------------------------------------------------------------------------------------------------
	void GC_MissionAreaTracker(BaseWorld world)
	{
		m_World = world;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_AreaEntityReference GetReference(string name)
	{
		GC_AreaEntityReference resolved;
		if (m_mEntities.Find(name, resolved))
			return resolved;
		resolved = new GC_AreaEntityReference();
		resolved.m_sName = name;
		m_mEntities.Set(name, resolved);
		return resolved;
	}

	//------------------------------------------------------------------------------------------------
	bool Register(GC_RuleCondition check)
	{
		foreach (GC_MissionAreaObservation pair : m_aPairs)
		{
			if (pair.m_Target.m_sName == check.m_sAreaTarget && pair.m_Area.m_Reference.m_sName == check.m_sAreaName)
			{
				m_mChecks.Set(check, pair);
				return true;
			}
		}
		if (m_aPairs.Count() >= 256)
			return false;
		GC_MissionAreaGeometry area;
		if (!m_mAreas.Find(check.m_sAreaName, area))
		{
			area = new GC_MissionAreaGeometry();
			area.m_Reference = GetReference(check.m_sAreaName);
			m_mAreas.Set(check.m_sAreaName, area);
		}
		GC_MissionAreaObservation observation = new GC_MissionAreaObservation();
		observation.m_Target = GetReference(check.m_sAreaTarget);
		observation.m_Area = area;
		m_aPairs.Insert(observation);
		m_mChecks.Set(check, observation);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	GC_MissionAreaObservation GetObservation(GC_RuleCondition check)
	{
		return m_mChecks.Get(check);
	}

	//------------------------------------------------------------------------------------------------
	void Sample(array<GC_RuleCondition> changed, bool initial = false)
	{
		foreach (string name, GC_AreaEntityReference resolved : m_mEntities)
			resolved.Sample(m_World);
		foreach (string areaName, GC_MissionAreaGeometry area : m_mAreas)
			area.Sample();
		foreach (GC_MissionAreaObservation pair : m_aPairs)
			pair.Sample();
		foreach (GC_RuleCondition check, GC_MissionAreaObservation observation : m_mChecks)
		{
			if (initial || observation.m_bChanged)
				changed.Insert(check);
		}
	}
}
