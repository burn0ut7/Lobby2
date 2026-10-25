//! Compare production mesh coverage against the engine's polygon containment.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeFillCoverage()
	{
		int samples, mismatches, overlaps;
		TriMeshDrawCommand mesh = new TriMeshDrawCommand();
		mesh.m_Vertices = {};
		mesh.m_Indices = {};
		for (int scenario; scenario < 6; scenario++)
		{
			array<vector> polygon = {"40 0 30", "280 0 30", "280 0 100", "130 0 100", "130 0 210", "40 0 210"};
			if (scenario == 1)
				polygon = {"-100 0 -100", "500 0 -100", "500 0 500", "-100 0 500"};
			if (scenario == 5)
			{
				polygon.Clear();
				for (int vertex; vertex < 128; vertex++)
				{
					float angle = vertex * Math.PI2 / 128;
					float radius = 90;
					if (vertex % 2 == 0)
						radius = 110;
					polygon.Insert(Vector(160 + Math.Cos(angle) * radius, 0, 120 + Math.Sin(angle) * radius));
				}
			}
			array<float> projected = {};
			for (int i; i < polygon.Count(); i++)
			{
				if (scenario == 2)
					polygon[i] = polygon[i] + "-160 0 -100";
				if (scenario == 3)
					polygon[i] = polygon[i] + "500 0 500";
				if (scenario == 4)
					polygon[i] = Vector(320 - polygon[i][0], 0, polygon[i][2]);
				projected.Insert(polygon[i][0]);
				projected.Insert(polygon[i][2]);
			}
			GC_MapShapeOutsideFill.Build(projected, 320, 240, mesh);
			for (int x; x < 32; x++)
			{
				for (int y; y < 24; y++)
				{
					vector point = Vector(x * 10 + 3.71, 0, y * 10 + 6.13);
					int covered;
					for (int triangle; triangle < mesh.m_Indices.Count(); triangle += 3)
					{
						array<vector> corners = {};
						for (int corner; corner < 3; corner++)
						{
							int index = mesh.m_Indices[triangle + corner] * 2;
							corners.Insert(Vector(mesh.m_Vertices[index], 0, mesh.m_Vertices[index + 1]));
						}
						if (Math2D.IsPointInPolygonXZ(corners, point))
							covered++;
					}
					bool expected = !Math2D.IsPointInPolygonXZ(polygon, point);
					if ((covered > 0) != expected)
						mismatches++;
					if (covered > 1)
						overlaps++;
					samples++;
				}
			}
		}
		return string.Format("scenarios=6 samples=%1 mismatches=%2 overlaps=%3", samples, mismatches, overlaps);
	}
}
