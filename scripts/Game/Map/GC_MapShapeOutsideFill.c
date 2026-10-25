//! Canvas polygons have no hole parameter. Partition the visible width at polygon
//! vertices, then fill the outside intervals with non-overlapping trapezoids.
//! Native canvas clipping trims their vertical extent to the map viewport.
class GC_MapShapeOutsideFill
{
	//------------------------------------------------------------------------------------------------
	static void Build(array<float> polygon, float width, float height, TriMeshDrawCommand mesh)
	{
		mesh.m_Vertices.Clear();
		mesh.m_Indices.Clear();
		if (width <= 0 || height <= 0 || polygon.Count() < 6)
			return;

		array<float> cuts = {0, width};
		float top = 0;
		float bottom = height;
		int count = polygon.Count() / 2;
		for (int i; i < count; i++)
		{
			float x = polygon[i * 2];
			float y = polygon[i * 2 + 1];
			if (x > 0 && x < width)
				cuts.Insert(x);
			top = Math.Min(top, y);
			bottom = Math.Max(bottom, y);
		}
		cuts.Sort();
		array<vector> crossings = {};
		for (int slab; slab < cuts.Count() - 1; slab++)
		{
			float left = cuts[slab];
			float right = cuts[slab + 1];
			if (right <= left)
				continue;
			float middle = (left + right) * 0.5;
			crossings.Clear();
			for (int edge; edge < count; edge++)
			{
				int next = (edge + 1) % count;
				float ax = polygon[edge * 2];
				float bx = polygon[next * 2];
				if (middle <= Math.Min(ax, bx) || middle >= Math.Max(ax, bx))
					continue;
				float ay = polygon[edge * 2 + 1];
				float slope = (polygon[next * 2 + 1] - ay) / (bx - ax);
				vector crossing = Vector(ay + (left - ax) * slope, ay + (right - ax) * slope, ay + (middle - ax) * slope);
				int insertion;
				while (insertion < crossings.Count() && crossings[insertion][2] < crossing[2])
					insertion++;
				crossings.InsertAt(crossing, insertion);
			}

			// A simple closed polygon alternates outside/inside at each crossing.
			vector upper = Vector(top, top, 0);
			for (int interval; interval <= crossings.Count(); interval += 2)
			{
				vector lower = Vector(bottom, bottom, 0);
				if (interval < crossings.Count())
					lower = crossings[interval];
				AddTrapezoid(mesh, left, right, upper, lower);
				if (interval + 1 < crossings.Count())
					upper = crossings[interval + 1];
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddTrapezoid(TriMeshDrawCommand mesh, float left, float right, vector upper, vector lower)
	{
		int first = mesh.m_Vertices.Count() / 2;
		mesh.m_Vertices.Insert(left);
		mesh.m_Vertices.Insert(upper[0]);
		mesh.m_Vertices.Insert(right);
		mesh.m_Vertices.Insert(upper[1]);
		mesh.m_Vertices.Insert(right);
		mesh.m_Vertices.Insert(lower[1]);
		mesh.m_Vertices.Insert(left);
		mesh.m_Vertices.Insert(lower[0]);
		mesh.m_Indices.Insert(first);
		mesh.m_Indices.Insert(first + 1);
		mesh.m_Indices.Insert(first + 2);
		mesh.m_Indices.Insert(first + 2);
		mesh.m_Indices.Insert(first + 3);
		mesh.m_Indices.Insert(first);
	}
}
