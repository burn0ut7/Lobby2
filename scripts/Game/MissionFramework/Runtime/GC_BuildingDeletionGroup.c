//! Read-only spatial association; does not attach furniture or modify building components.
class GC_BuildingDeletionGroup
{
	IEntity m_Building;
	ref array<IEntity> m_aContents = {};
	ref array<IEntity> m_aBuildings = {};
	protected vector m_aTransform[4];
	protected vector m_vMins;
	protected vector m_vMaxs;
	protected int m_iVisited;

	//------------------------------------------------------------------------------------------------
	protected bool AddContent(IEntity entity)
	{
		m_iVisited++;
		if (m_iVisited > 4096)
			return false;
		if (!entity || entity == m_Building)
			return true;
		if (Building.Cast(entity))
		{
			if (!m_aBuildings.Contains(entity))
				m_aBuildings.Insert(entity);
			return true;
		}
		vector position = entity.GetOrigin().InvMultiply4(m_aTransform);
		for (int axis = 0; axis < 3; axis++)
		{
			if (position[axis] < m_vMins[axis] || position[axis] > m_vMaxs[axis])
				return true;
		}
		if (!m_aContents.Contains(entity))
			m_aContents.Insert(entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Collect(IEntity building, inout int budget)
	{
		m_Building = building;
		building.GetWorldTransform(m_aTransform);
		SCR_DestructibleBuildingComponent component = SCR_DestructibleBuildingComponent.Cast(building.FindComponent(SCR_DestructibleBuildingComponent));
		SCR_DestructibleBuildingComponentClass data;
		if (component)
			data = SCR_DestructibleBuildingComponentClass.Cast(component.GetComponentData(building));
		BaseWorld world = building.GetWorld();
		if (data && data.m_aInteriorQueryBoundingBoxes && !data.m_aInteriorQueryBoundingBoxes.IsEmpty())
		{
			if (data.m_aInteriorQueryBoundingBoxes.Count() > 64)
				return false;
			foreach (SCR_InteriorBoundingBox box : data.m_aInteriorQueryBoundingBoxes)
			{
				vector center;
				if (!box || !box.GetCenter(center) || box.GetScale() == vector.One)
					return false;
				box.GetBounds(m_vMins, m_vMaxs);
				if (!world.QueryEntitiesByOBB(m_vMins, m_vMaxs, m_aTransform, AddContent, null, EQueryEntitiesFlags.ALL))
					return false;
			}
		}
		else
		{
			building.GetBounds(m_vMins, m_vMaxs);
			if (!world.QueryEntitiesByOBB(m_vMins, m_vMaxs, m_aTransform, AddContent, null, EQueryEntitiesFlags.ALL))
				return false;
		}
		budget -= m_iVisited;
		return budget >= 0;
	}
}
