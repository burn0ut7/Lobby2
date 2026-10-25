#ifdef WORKBENCH
modded class GC_BoundaryWallComponent
{
	protected int m_iGC_MeshUpdates;

	//------------------------------------------------------------------------------------------------
	override protected bool UpdateNearbyMesh(vector position)
	{
		bool result = super.UpdateNearbyMesh(position);
		m_iGC_MeshUpdates++;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLocalMesh()
	{
		if (!m_Wall)
			return GC_TestSnapshot();
		float maximumDistance;
		int visibleColumns;
		for (int column; column < m_iVisibleColumns; column++)
		{
			vector offset = m_aVertices[column * 2] + m_Wall.GetOrigin() - m_vLastVisualPosition;
			offset[1] = 0;
			maximumDistance = Math.Max(maximumDistance, offset.Length());
			visibleColumns++;
		}
		return string.Format("updates=%1 columns=%2 maximumDistance=%3 range=%4 %5",
			m_iGC_MeshUpdates, visibleColumns, maximumDistance, m_fRange, GC_TestSnapshot());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBounds()
	{
		if (!m_Wall)
			return "no mesh";
		vector minimum, maximum, worldMinimum, worldMaximum;
		m_Wall.GetBounds(minimum, maximum);
		m_Wall.GetWorldBounds(worldMinimum, worldMaximum);
		return string.Format("local=%1/%2 world=%3/%4 origin=%5 flags=%6 opacity=%7",
			minimum, maximum, worldMinimum, worldMaximum, m_Wall.GetOrigin(), m_Wall.GetFlags(), m_fOpacity);
	}

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
	string GC_TestWallLocal(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (entityName.IsEmpty())
		{
			GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
			if (player)
				entity = player.GC_GetSetupZone();
		}
		if (!entity)
			return "missing entity";
		GC_BoundaryWallComponent wall = GC_BoundaryWallComponent.Cast(entity.FindComponent(GC_BoundaryWallComponent));
		if (!wall)
			return "missing component";
		return wall.GC_TestLocalMesh();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestWallBounds(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (entityName.IsEmpty())
		{
			GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
			if (player)
				entity = player.GC_GetSetupZone();
		}
		if (!entity)
			return "missing entity";
		GC_BoundaryWallComponent wall = GC_BoundaryWallComponent.Cast(entity.FindComponent(GC_BoundaryWallComponent));
		if (!wall)
			return "missing component";
		return wall.GC_TestBounds();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestWallLook(float degrees)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character)
			return "missing character";
		character.GetCharacterController().SetHeadingAngle(degrees * Math.DEG2RAD, true);
		return string.Format("requestedHeading=%1 position=%2", degrees, character.GetOrigin());
	}

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
