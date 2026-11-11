//! Static world IDs are used by the native composition/destruction systems too.
//! Type, prefab and position guard against a local runtime ID identifying a different entity.
class GC_RadiusDeletionRecord
{
	EntityID m_EntityId;
	string m_sType;
	ResourceName m_sPrefab;
	vector m_vPosition;

	//------------------------------------------------------------------------------------------------
	static ResourceName PrefabName(IEntity entity)
	{
		if (entity && entity.GetPrefabData() && entity.GetPrefabData().GetPrefab())
			return entity.GetPrefabData().GetPrefab().GetResourceName();
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void Capture(IEntity entity)
	{
		m_EntityId = entity.GetID();
		m_sType = entity.ClassName();
		m_sPrefab = PrefabName(entity);
		m_vPosition = entity.GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	void Apply(BaseWorld world)
	{
		IEntity entity = world.FindEntityByID(m_EntityId);
		if (!entity)
			return;
		if (entity.FindComponent(RplComponent) || entity.ClassName() != m_sType || PrefabName(entity) != m_sPrefab
			|| vector.DistanceSq(entity.GetOrigin(), m_vPosition) > 0.0001)
		{
			Print("GC | Radius deletion | Local entity does not match the authority's removal record; skipped.", LogLevel.WARNING);
			return;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	//------------------------------------------------------------------------------------------------
	void Save(ScriptBitWriter writer)
	{
		writer.WriteEntityId(m_EntityId);
		writer.WriteString(m_sType);
		writer.WriteString(m_sPrefab);
		writer.WriteVector(m_vPosition);
	}

	//------------------------------------------------------------------------------------------------
	bool Load(ScriptBitReader reader)
	{
		return reader.ReadEntityId(m_EntityId) && reader.ReadString(m_sType) && reader.ReadString(m_sPrefab)
			&& reader.ReadVector(m_vPosition) && m_EntityId != EntityID.INVALID && !m_sType.IsEmpty();
	}
}
