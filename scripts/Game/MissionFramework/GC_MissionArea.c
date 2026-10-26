//! Shared world-space geometry for area checks, spawning and debug display.
class GC_MissionArea
{
	//------------------------------------------------------------------------------------------------
	static bool WorldPoints(IEntity area, notnull array<vector> points)
	{
		points.Clear();
		PolylineShapeEntity shape = PolylineShapeEntity.Cast(area);
		if (!shape)
			return false;
		shape.GetPointsPositions(points);
		if (points.Count() < 3)
			return false;
		vector transform[4];
		shape.GetWorldTransform(transform);
		for (int i; i < points.Count(); i++)
			points[i] = points[i].Multiply4(transform);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool Contains(IEntity area, float radius, vector position)
	{
		array<vector> points = {};
		if (WorldPoints(area, points))
			return Math2D.IsPointInPolygonXZ(points, position);
		return vector.Distance(area.GetOrigin(), position) <= radius;
	}
}

//! Strong shape references live only for this context and are released on Stop.
class GC_MissionAreaDebug
{
	ref array<ref Shape> m_aShapes = {};

	//------------------------------------------------------------------------------------------------
	void Draw(IEntity area, float radius)
	{
		m_aShapes.Clear();
		array<vector> points = {};
		if (GC_MissionArea.WorldPoints(area, points))
		{
			for (int i; i < points.Count(); i++)
			{
				vector line[2];
				line[0] = points[i];
				line[1] = points[(i + 1) % points.Count()];
				m_aShapes.Insert(Shape.CreateLines(Color.YELLOW, ShapeFlags.NOZBUFFER, line, 2));
			}
		}
		else
			m_aShapes.Insert(Shape.CreateSphere(Color.YELLOW, ShapeFlags.WIREFRAME | ShapeFlags.NOZBUFFER, area.GetOrigin(), radius));
	}
}
