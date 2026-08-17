//! Immutable server-local copy of a playable group's authored world transform.
//! SCR_AIGroup may recenter its runtime origin when players are added, so Lobby2
//! must not use that mutable origin as the formation anchor.
class GC_PlayerGroupSpawnTransform
{
	protected SCR_AIGroup m_Group;
	protected vector m_vAxisX;
	protected vector m_vAxisY;
	protected vector m_vAxisZ;
	protected vector m_vOrigin;
	protected vector m_vAngles;

	void GC_PlayerGroupSpawnTransform(SCR_AIGroup group)
	{
		m_Group = group;
		if (!group)
			return;

		vector transform[4];
		group.GetWorldTransform(transform);
		m_vAxisX = transform[0];
		m_vAxisY = transform[1];
		m_vAxisZ = transform[2];
		m_vOrigin = transform[3];
		m_vAngles = group.GetAngles();
	}

	bool IsFor(SCR_AIGroup group)
	{
		return m_Group == group;
	}

	vector GetOrigin()
	{
		return m_vOrigin;
	}

	vector GetAngles()
	{
		return m_vAngles;
	}

	vector TransformOffset(vector offset)
	{
		return m_vOrigin + m_vAxisX * offset[0] + m_vAxisY * offset[1] + m_vAxisZ * offset[2];
	}

	void ApplyRotation(EntitySpawnParams params)
	{
		if (!params)
			return;

		params.Transform[0] = m_vAxisX;
		params.Transform[1] = m_vAxisY;
		params.Transform[2] = m_vAxisZ;
	}
}
