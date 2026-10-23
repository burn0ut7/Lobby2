//! Server-owned original location for an administrator-created slot.
//! Character movement and group recentering never change this anchor.
class GC_SlotSpawnPoint
{
	protected vector m_vOrigin;
	protected vector m_vAngles;

	//------------------------------------------------------------------------------------------------
	void GC_SlotSpawnPoint(vector origin, vector angles)
	{
		m_vOrigin = origin;
		m_vAngles = angles;
	}

	//------------------------------------------------------------------------------------------------
	EntitySpawnParams CreateParams()
	{
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(m_vAngles, params.Transform);
		params.Transform[3] = m_vOrigin;
		return params;
	}
}
