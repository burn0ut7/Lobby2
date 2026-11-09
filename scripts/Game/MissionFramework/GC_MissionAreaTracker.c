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
	protected ref map<IEntity, bool> m_mMembership = new map<IEntity, bool>();

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_bValid && m_Reference.m_Entity;
	}

	//------------------------------------------------------------------------------------------------
	void Sample()
	{
		m_mMembership.Clear();
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
	bool ContainsEntity(IEntity entity, vector position)
	{
		bool inside;
		if (m_mMembership.Find(entity, inside))
			return inside;
		inside = Contains(position);
		m_mMembership.Set(entity, inside);
		return inside;
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

//! Shared counts for a target selection and area; comparison and side stay on the condition.
class GC_MissionAreaObservation
{
	GC_RuleCondition m_Definition;
	ref array<ref GC_AreaEntityReference> m_aTargets = {};
	ref GC_MissionAreaGeometry m_Area;
	bool m_bChanged;
	protected bool m_bAvailable;
	protected int m_iInside;
	protected int m_iTotal;
	protected bool m_bSampled;

	//------------------------------------------------------------------------------------------------
	bool Matches(GC_RuleCondition other)
	{
		if (m_Definition.m_sAreaName != other.m_sAreaName || m_Definition.m_iAreaTargets != other.m_iAreaTargets)
			return false;
		array<string> first = m_Definition.m_aAreaNames;
		array<string> second = other.m_aAreaNames;
		if (other.m_iAreaTargets == 1)
		{
			if (m_Definition.m_iAreaWho != other.m_iAreaWho)
				return false;
			first = m_Definition.m_aAreaFactions;
			second = other.m_aAreaFactions;
		}
		int firstCount;
		int secondCount;
		if (first)
			firstCount = first.Count();
		if (second)
			secondCount = second.Count();
		if (firstCount != secondCount)
			return false;
		if (first)
		{
			foreach (string value : first)
			{
				if (!second || !second.Contains(value))
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Sample(GC_AreaPopulation population)
	{
		bool available = m_Area.IsAvailable();
		int total;
		int inside;
		if (m_Definition.m_iAreaTargets == 0)
		{
			foreach (GC_AreaEntityReference target : m_aTargets)
			{
				if (!target.m_Entity)
				{
					available = false;
					continue;
				}
				total++;
				if (available && m_Area.ContainsEntity(target.m_Entity, target.m_vPosition))
					inside++;
			}
		}
		else if (!population || !population.IsAvailable())
			available = false;
		else if (available)
		{
			foreach (GC_AreaCharacter character : population.m_aCharacters)
			{
				if ((m_Definition.m_iAreaWho == 0 && !character.m_bPlayer) || (m_Definition.m_iAreaWho == 1 && character.m_bPlayer))
					continue;
				array<string> factions = m_Definition.m_aAreaFactions;
				if (factions && !factions.IsEmpty() && !factions.Contains(character.m_sFaction))
					continue;
				total++;
				if (m_Area.ContainsEntity(character.m_Entity, character.m_vPosition))
					inside++;
			}
		}
		m_bChanged = !m_bSampled || available != m_bAvailable || inside != m_iInside || total != m_iTotal;
		m_bSampled = true;
		m_bAvailable = available;
		m_iInside = inside;
		m_iTotal = total;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		if (!m_bAvailable || !m_Area.IsAvailable())
			return false;
		foreach (GC_AreaEntityReference target : m_aTargets)
		{
			if (!target.m_Entity)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Passes(bool outside, int comparison, int required)
	{
		if (!IsAvailable())
			return false;
		int count = m_iInside;
		if (outside)
			count = m_iTotal - m_iInside;
		if (comparison == 0)
			return count > required;
		if (comparison == 1)
			return count < required;
		if (comparison == 2)
			return count == required;
		return comparison == 3 && count != required;
	}
}

//! One server-owned sampler. Native registries supply populations; no spawn overrides or spatial queries.
class GC_MissionAreaTracker
{
	protected BaseWorld m_World;
	protected GC_LobbyManager m_Manager;
	protected ref GC_AreaPopulation m_Population;
	protected bool m_bIncludeAI;
	protected ref map<string, ref GC_AreaEntityReference> m_mEntities = new map<string, ref GC_AreaEntityReference>();
	protected ref map<string, ref GC_MissionAreaGeometry> m_mAreas = new map<string, ref GC_MissionAreaGeometry>();
	protected ref array<ref GC_MissionAreaObservation> m_aPairs = {};
	protected ref map<GC_RuleCondition, ref GC_MissionAreaObservation> m_mChecks = new map<GC_RuleCondition, ref GC_MissionAreaObservation>();
	protected ref map<GC_RuleCondition, bool> m_mAnswers = new map<GC_RuleCondition, bool>();
	protected ref map<GC_RuleCondition, bool> m_mAvailability = new map<GC_RuleCondition, bool>();

	//------------------------------------------------------------------------------------------------
	void GC_MissionAreaTracker(BaseWorld world, GC_LobbyManager manager = null)
	{
		m_World = world;
		m_Manager = manager;
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
			if (pair.Matches(check))
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
		observation.m_Definition = check;
		if (check.m_iAreaTargets == 0)
		{
			foreach (string name : check.m_aAreaNames)
			{
				GC_AreaEntityReference target = GetReference(name);
				if (!observation.m_aTargets.Contains(target))
					observation.m_aTargets.Insert(target);
			}
		}
		else
		{
			if (!m_Population)
				m_Population = new GC_AreaPopulation();
			if (check.m_iAreaWho != 0)
				m_bIncludeAI = true;
		}
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
		if (m_Population)
			m_Population.Sample(m_Manager, m_bIncludeAI);
		foreach (string areaName, GC_MissionAreaGeometry area : m_mAreas)
			area.Sample();
		foreach (GC_MissionAreaObservation pair : m_aPairs)
			pair.Sample(m_Population);
		foreach (GC_RuleCondition check, GC_MissionAreaObservation observation : m_mChecks)
		{
			if (!initial && !observation.m_bChanged)
				continue;
			bool available = observation.IsAvailable();
			bool answer = observation.Passes(check.m_bAreaOutside, check.m_iAreaComparison, check.m_iAreaRequired);
			if (initial || !m_mAvailability.Contains(check) || m_mAvailability.Get(check) != available || m_mAnswers.Get(check) != answer)
				changed.Insert(check);
			m_mAvailability.Set(check, available);
			m_mAnswers.Set(check, answer);
		}
	}
}
