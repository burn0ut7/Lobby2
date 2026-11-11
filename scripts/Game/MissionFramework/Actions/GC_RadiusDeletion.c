[BaseContainerProps(), BaseContainerCustomStringTitleField("Radius")]
class GC_RadiusDeletion
{
	[Attribute("", UIWidgets.EditComboBox, "Existing entity whose current position is the center. The center and the active game mode are protected, including their parents.", enums: {ParamEnum("", "", "rst-choices:2:GC_EntityReferenceChoices")})]
	string m_sCenter;
	[Attribute("10", UIWidgets.SpinBox, "Radius in meters, including the boundary. Uses entity origins in 3D. Selected buildings include loose objects inside their interior volumes, even beyond the radius. Attached children are also removed.", "0.1 1000 0.1")]
	float m_fRadius = 10;
	[Attribute("", UIWidgets.ResourceNamePicker, "Optional prefabs eligible for deletion, including inherited variants. Empty permits every prefab type. Objects without prefab data can only be selected when this list is empty.", "et")]
	ref array<ResourceName> m_aWhitelist;
	[Attribute("", UIWidgets.ResourceNamePicker, "Optional protected prefabs, including inherited variants. Blacklist wins. Blacklisted buildings protect their interiors; a protected interior object protects the whole building. Protected attached children also protect their parent.", "et")]
	ref array<ResourceName> m_aBlacklist;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return !m_sCenter.IsEmpty() && m_fRadius >= 0.1 && m_fRadius <= 1000
			&& (!m_aWhitelist || m_aWhitelist.Count() <= 128) && (!m_aBlacklist || m_aBlacklist.Count() <= 128);
	}
}

//! Bounded box queries per execution; selection and interior protection finish before mutation.
class GC_RadiusDeletionQuery
{
	protected vector m_vCenter;
	protected float m_fRadiusSq;
	protected IEntity m_Owner;
	protected IEntity m_CenterEntity;
	protected int m_iVisited;
	protected ref array<IEntity> m_aCandidates = {};
	protected ref array<IEntity> m_aBuildings = {};
	protected ref set<IEntity> m_aProtected = new set<IEntity>();
	protected ref array<ResourceName> m_aWhitelist = {};
	protected ref array<ResourceName> m_aBlacklist = {};

	//------------------------------------------------------------------------------------------------
	protected bool LoadFilters(array<ResourceName> input, array<ResourceName> output)
	{
		if (!input)
			return true;
		foreach (ResourceName name : input)
		{
			if (name.IsEmpty())
				return false;
			Resource resource = Resource.Load(name);
			if (!resource || !resource.IsValid() || !resource.GetResource().ToEntitySource())
				return false;
			ResourceName canonical = resource.GetResource().ToBaseContainer().GetResourceName();
			if (canonical.IsEmpty())
				return false;
			if (!output.Contains(canonical))
				output.Insert(canonical);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Matches(IEntity entity, array<ResourceName> prefabs)
	{
		if (prefabs.IsEmpty() || !entity.GetPrefabData())
			return false;
		BaseContainer container = entity.GetPrefabData().GetPrefab();
		while (container)
		{
			if (prefabs.Contains(container.GetResourceName()))
				return true;
			container = container.GetAncestor();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void Protect(IEntity entity)
	{
		while (entity)
		{
			m_aProtected.Insert(entity);
			entity = entity.GetParent();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool AddCandidate(IEntity entity)
	{
		m_iVisited++;
		if (m_iVisited > 4096)
			return false;
		// Keep intersecting buildings even when their origin is outside the radius:
		// their interiors can overlap the selection and must honor the blacklist.
		if (Building.Cast(entity) && !m_aBuildings.Contains(entity))
			m_aBuildings.Insert(entity);
		if (entity && vector.DistanceSq(entity.GetOrigin(), m_vCenter) <= m_fRadiusSq && !m_aCandidates.Contains(entity))
			m_aCandidates.Insert(entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Hierarchy is inspected only to prevent collateral deletion of protected attachments.
	protected bool ProtectedTree(IEntity entity, inout int budget, int depth = 0)
	{
		if (depth > 32)
			budget = 0;
		budget--;
		if (budget < 0 || m_aProtected.Contains(entity) || Matches(entity, m_aBlacklist))
			return true;
		IEntity child = entity.GetChildren();
		while (child)
		{
			if (ProtectedTree(child, budget, depth + 1))
				return true;
			child = child.GetSibling();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ProtectBuildings(array<ref GC_BuildingDeletionGroup> groups, inout int budget)
	{
		// Shared/intersecting interiors propagate protection before any deletion.
		bool changed = true;
		set<IEntity> protectedBuildings = new set<IEntity>();
		while (changed)
		{
			changed = false;
			foreach (GC_BuildingDeletionGroup group : groups)
			{
				if (protectedBuildings.Contains(group.m_Building))
					continue;
				bool protect = ProtectedAncestor(group.m_Building) || ProtectedTree(group.m_Building, budget);
				foreach (IEntity content : group.m_aContents)
				{
					if (ProtectedAncestor(content) || ProtectedTree(content, budget))
						protect = true;
					if (budget < 0)
						return false;
				}
				if (!protect)
					continue;
				Protect(group.m_Building);
				protectedBuildings.Insert(group.m_Building);
				foreach (IEntity content : group.m_aContents)
					Protect(content);
				changed = true;
			}
		}
		return budget >= 0;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ProtectedAncestor(IEntity entity)
	{
		while (entity)
		{
			if (entity == m_Owner || entity == m_CenterEntity || Matches(entity, m_aBlacklist))
				return true;
			entity = entity.GetParent();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	bool Collect(GC_RadiusDeletion settings, IEntity owner, array<IEntity> selected)
	{
		if (!settings || !settings.IsValid() || !owner)
			return false;
		BaseWorld world = owner.GetWorld();
		IEntity center = world.FindEntityByName(settings.m_sCenter);
		if (!center || !LoadFilters(settings.m_aWhitelist, m_aWhitelist) || !LoadFilters(settings.m_aBlacklist, m_aBlacklist))
			return false;
		Protect(owner);
		Protect(center);
		m_Owner = owner;
		m_CenterEntity = center;
		m_vCenter = center.GetOrigin();
		m_fRadiusSq = settings.m_fRadius * settings.m_fRadius;
		vector extent = Vector(settings.m_fRadius, settings.m_fRadius, settings.m_fRadius);
		if (!world.QueryEntitiesByAABB(m_vCenter - extent, m_vCenter + extent, AddCandidate, null, EQueryEntitiesFlags.ALL))
			return false;
		int budget = 16384;
		if (m_aBuildings.Count() > 128)
			return false;
		array<ref GC_BuildingDeletionGroup> groups = {};
		for (int buildingIndex = 0; buildingIndex < m_aBuildings.Count(); buildingIndex++)
		{
			IEntity building = m_aBuildings[buildingIndex];
			GC_BuildingDeletionGroup group = new GC_BuildingDeletionGroup();
			if (!group.Collect(building, budget))
				return false;
			// Loose reference entities remain usable inside a cleanup area. Their
			// presence alone does not protect the surrounding building.
			group.m_aContents.RemoveItem(center);
			group.m_aContents.RemoveItem(owner);
			groups.Insert(group);
			// Follow overlapping buildings beyond the original radius so their
			// blacklist protection also covers contents brought in by this group.
			foreach (IEntity neighbor : group.m_aBuildings)
			{
				if (!m_aBuildings.Contains(neighbor))
					m_aBuildings.Insert(neighbor);
			}
			if (m_aBuildings.Count() > 128)
				return false;
		}
		if (!ProtectBuildings(groups, budget))
			return false;
		foreach (IEntity entity : m_aCandidates)
		{
			if (ProtectedAncestor(entity))
				continue;
			if (!m_aWhitelist.IsEmpty() && !Matches(entity, m_aWhitelist))
				continue;
			if (!ProtectedTree(entity, budget))
				selected.Insert(entity);
			if (budget < 0)
			{
				selected.Clear();
				return false;
			}
		}
		// A selected building brings its entire unprotected interior, even beyond
		// the radius or whitelist. Blacklisting any member protects the group.
		foreach (GC_BuildingDeletionGroup group : groups)
		{
			if (!selected.Contains(group.m_Building))
				continue;
			foreach (IEntity content : group.m_aContents)
			{
				if (!selected.Contains(content))
					selected.Insert(content);
			}
		}
		return true;
	}
}
