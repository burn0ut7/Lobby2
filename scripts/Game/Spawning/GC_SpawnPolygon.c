//! Validated world-space XZ polygon. One bounded candidate per call; no rejection loop hidden inside.
class GC_SpawnPolygon
{
	protected ref array<vector> m_aPoints = {};
	protected vector m_vMinimum;
	protected vector m_vMaximum;
	protected bool m_bValid;

	//------------------------------------------------------------------------------------------------
	bool Read(PolylineShapeEntity shape)
	{
		m_bValid = false;
		if (!shape || !shape.IsClosed())
			return false;
		array<vector> localPoints = {};
		shape.GetPointsPositions(localPoints);
		vector transform[4];
		shape.GetWorldTransform(transform);
		array<vector> points = {};
		foreach (vector point : localPoints)
			points.Insert(point.Multiply4(transform));
		return SetPoints(points);
	}

	//------------------------------------------------------------------------------------------------
	bool SetPoints(array<vector> points)
	{
		m_bValid = false;
		m_aPoints.Clear();
		if (!points || points.Count() < 3 || points.Count() > 129)
			return false;
		m_aPoints.Copy(points);
		if (DistanceSquared(m_aPoints[0], m_aPoints[m_aPoints.Count() - 1]) < 0.0001)
			m_aPoints.Remove(m_aPoints.Count() - 1);
		int count = m_aPoints.Count();
		if (count < 3 || count > 128)
			return false;
		m_vMinimum = m_aPoints[0];
		m_vMaximum = m_vMinimum;
		float twiceArea;
		for (int i; i < count; i++)
		{
			vector a = m_aPoints[i];
			vector b = m_aPoints[(i + 1) % count];
			if (DistanceSquared(a, b) < 0.0001)
				return false;
			twiceArea += Cross(m_aPoints[0], a, b);
			m_vMinimum[0] = Math.Min(m_vMinimum[0], a[0]);
			m_vMinimum[2] = Math.Min(m_vMinimum[2], a[2]);
			m_vMaximum[0] = Math.Max(m_vMaximum[0], a[0]);
			m_vMaximum[2] = Math.Max(m_vMaximum[2], a[2]);
			for (int j = i + 2; j < count; j++)
			{
				if (i == 0 && j == count - 1)
					continue;
				if (Intersects(a, b, m_aPoints[j], m_aPoints[(j + 1) % count]))
					return false;
			}
		}
		m_bValid = Math.AbsFloat(twiceArea) > 0.01;
		return m_bValid;
	}

	//------------------------------------------------------------------------------------------------
	bool Contains(vector point)
	{
		return m_bValid && Math2D.IsPointInPolygonXZ(m_aPoints, point);
	}

	//------------------------------------------------------------------------------------------------
	bool TryCandidate(out vector point)
	{
		if (!m_bValid)
			return false;
		point = Vector(Math.RandomFloat(m_vMinimum[0], m_vMaximum[0]), 0, Math.RandomFloat(m_vMinimum[2], m_vMaximum[2]));
		return Contains(point);
	}

	//------------------------------------------------------------------------------------------------
	static float DistanceSquared(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	//------------------------------------------------------------------------------------------------
	protected static float Cross(vector a, vector b, vector c)
	{
		return (b[0] - a[0]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[0] - a[0]);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool OnSegment(vector a, vector b, vector point)
	{
		return Math.AbsFloat(Cross(a, b, point)) < 0.0001
			&& point[0] >= Math.Min(a[0], b[0]) && point[0] <= Math.Max(a[0], b[0])
			&& point[2] >= Math.Min(a[2], b[2]) && point[2] <= Math.Max(a[2], b[2]);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Intersects(vector a, vector b, vector c, vector d)
	{
		float first = Cross(a, b, c);
		float second = Cross(a, b, d);
		float third = Cross(c, d, a);
		float fourth = Cross(c, d, b);
		if (((first > 0 && second < 0) || (first < 0 && second > 0))
			&& ((third > 0 && fourth < 0) || (third < 0 && fourth > 0)))
			return true;
		return OnSegment(a, b, c) || OnSegment(a, b, d) || OnSegment(c, d, a) || OnSegment(c, d, b);
	}
}
