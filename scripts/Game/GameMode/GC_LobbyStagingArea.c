//! Scenario-authored circular staging area. Center is a safe world-space return
//! position, including height; an empty faction key permits every faction.
[BaseContainerProps()]
class GC_LobbyStagingArea
{
	[Attribute("0 0 0", UIWidgets.EditBox, "Safe world-space return position inside this staging area")]
	vector m_vCenter;
	[Attribute("50", UIWidgets.EditBox, "Horizontal staging radius in metres (1-5000)")]
	float m_fRadius;
	[Attribute("", UIWidgets.EditBox, "Base-game faction key, or empty for every faction")]
	string m_sFactionKey;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return m_fRadius >= 1 && m_fRadius <= 5000 && SCR_Global.IsPositionWithinTerrainBounds(m_vCenter);
	}

	//------------------------------------------------------------------------------------------------
	bool AllowsFaction(Faction faction)
	{
		return m_sFactionKey.IsEmpty() || (faction && faction.GetFactionKey() == m_sFactionKey);
	}

	//------------------------------------------------------------------------------------------------
	bool Contains(vector position)
	{
		return vector.DistanceSqXZ(position, m_vCenter) <= m_fRadius * m_fRadius;
	}
}
