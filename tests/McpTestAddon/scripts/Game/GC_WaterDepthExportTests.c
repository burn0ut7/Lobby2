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
			if (fields.Count() != 2 || points.Count() >= 100000)
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
			points.Insert(Vector(x, 0, z));
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
			vector surface, transform[4], extents;
			EWaterSurfaceType type;
			bool wet;
			// Water queries only hit inside the finite volume. Walk from above
			// the map down to the bed, including the final partial step.
			point[1] = Math.Max(m_fGC_WaterColumnTop, ground);
			while (true)
			{
				wet = ChimeraWorldUtils.TryGetWaterSurface(world, point, surface, type, transform, extents) && surface[1] > ground;
				if (wet || point[1] <= ground)
					break;
				point[1] = Math.Max(ground, point[1] - 0.5);
			}
			if (wet)
				wetCount++;
			output.WriteLine(string.Format("%1\t%2\t%3\t%4\t%5\t%6", point[0], point[2], ground, surface[1], type, wet));
		}
		output.Close();
		string result = string.Format("samples=%1 wet=%2 method=top-down-water-column-0.5m", points.Count(), wetCount);
		GC_AdminActionLogger.Log(player.GetPlayerId(), "water-depth-export", m_sGC_ExportName, result);
		return result;
	}
}
#endif
