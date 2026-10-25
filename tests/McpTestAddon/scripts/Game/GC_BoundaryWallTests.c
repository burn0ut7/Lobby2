#ifdef WORKBENCH
modded class GC_BoundaryWallComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSnapshot()
	{
		bool mesh = m_Wall != null;
		bool visible = mesh && (m_Wall.GetFlags() & EntityFlags.VISIBLE) != 0;
		return string.Format("enabled=%1 audience=%2 points=%3 mesh=%4 visible=%5 opacity=%6 color=%7 failed=%8",
			m_bEnabled, m_aAudience.Count(), m_aPolygon.Count(), mesh, visible, m_fOpacity, m_iColor, m_bFailed);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	bool GC_TestWallAOPolygon(int vertices, float radius)
	{
		if (vertices < 3 || vertices > 60 || radius < 1 || m_aGC_AOFixtures.IsEmpty())
			return false;
		array<vector> points = {};
		for (int i; i < vertices; i++)
		{
			float angle = Math.PI2 * i / vertices;
			points.Insert(m_vGC_AOOrigin + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius));
		}
		return m_aGC_AOFixtures[0].SetPolygon(points, false, "");
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestWallSnapshot(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return "missing entity";
		GC_BoundaryWallComponent wall = GC_BoundaryWallComponent.Cast(entity.FindComponent(GC_BoundaryWallComponent));
		if (!wall)
			return "missing component";
		return wall.GC_TestSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	bool GC_TestWallConfigure(string entityName, bool enabled, float range, float height, int color)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return false;
		GC_BoundaryWallComponent wall = GC_BoundaryWallComponent.Cast(entity.FindComponent(GC_BoundaryWallComponent));
		return wall && wall.Configure(enabled, range, height, color);
	}
}
#endif
