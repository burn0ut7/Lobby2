#ifdef WORKBENCH
//! Explicit offline cartography probe; no gameplay hooks or state changes.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapOceanInfo()
	{
		BaseWorld world = GetGame().GetWorld();
		return string.Format("%1\t%2\t%3", GetGame().GetWorldFile(), world.IsOcean(), world.GetOceanBaseHeight());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRiverSurfaceExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!GC_MapExportReady() || !player || !AuthorizeAdminAction(player.GetPlayerId(), "river-surface-export", m_sGC_ExportName))
			return "denied";
		FileHandle output = FileIO.OpenFile("$profile:GC_RiverSurfaces_" + m_sGC_ExportName + ".tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("river\tindex\tx\ty\tz");
		array<RiverEntity> rivers = {};
		RiverEntity.GetExistingInstances(rivers);
		int count;
		foreach (int riverIndex, RiverEntity river : rivers)
		{
			if (river.GetWorld() != GetGame().GetWorld())
				continue;
			array<vector> outline = {};
			river.GetOutlinePoints(outline);
			foreach (int index, vector point : outline)
			{
				output.WriteLine(string.Format("%1\t%2\t%3\t%4\t%5", riverIndex, index, point[0], point[1], point[2]));
				count++;
			}
		}
		output.Close();
		return "vertices=" + count;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestWaterDepthExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "water-depth-export", m_sGC_ExportName))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		FileHandle input = FileIO.OpenFile("$profile:GC_WaterProbePoints.tsv", FileMode.READ);
		if (!input)
			return "missing-input";
		array<vector> points = {};
		string line;
		while (input.ReadLine(line) >= 0)
		{
			array<string> fields = {};
			line.Split("\t", fields, false);
			if ((fields.Count() != 2 && fields.Count() != 3) || points.Count() >= 100000)
			{
				input.Close();
				return "invalid-input";
			}
			float x = fields[0].ToFloat();
			float z = fields[1].ToFloat();
			if (x < 0 || x > m_vGC_ExportSize[0] || z < 0 || z > m_vGC_ExportSize[2])
			{
				input.Close();
				return "outside-world";
			}
			float hint = -1e10;
			if (fields.Count() == 3)
			{
				hint = fields[2].ToFloat();
				if (!(hint >= -10000 && hint <= m_fGC_MaxWaterHint))
				{
					input.Close();
					return "invalid-surface-hint";
				}
			}
			points.Insert(Vector(x, hint, z));
		}
		input.Close();
		FileHandle output = FileIO.OpenFile("$profile:GC_WaterDepth_" + m_sGC_ExportName + ".tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("x\tz\tground\tsurface\ttype\twet");
		BaseWorld world = GetGame().GetWorld();
		int wetCount;
		foreach (vector point : points)
		{
			float ground = world.GetSurfaceY(point[0], point[2]);
			float surfaceHint = point[1];
			point[1] = ground;
			vector surface, transform[4], extents;
			EWaterSurfaceType type;
			bool wet = ChimeraWorldUtils.TryGetWaterSurface(world, point, surface, type, transform, extents);
			// River volumes can stop well above the riverbed. Search the column
			// up to the authored outline; the native query confirms each sample.
			// Outline interpolation is a hint, not the final sloping water plane.
			while (!wet && point[1] < surfaceHint)
			{
				point[1] = Math.Min(point[1] + 0.5, surfaceHint);
				wet = ChimeraWorldUtils.TryGetWaterSurface(world, point, surface, type, transform, extents);
			}
			if (wet)
				wetCount++;
			output.WriteLine(string.Format("%1\t%2\t%3\t%4\t%5\t%6", point[0], point[2], ground, surface[1], type, wet));
		}
		output.Close();
		string result = string.Format("samples=%1 wet=%2", points.Count(), wetCount);
		GC_AdminActionLogger.Log(player.GetPlayerId(), "water-depth-export", m_sGC_ExportName, result);
		return result;
	}
}
#endif
