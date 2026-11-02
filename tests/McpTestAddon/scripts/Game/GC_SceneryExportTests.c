#ifdef WORKBENCH
//! One-off static scenery export for offline website map generation.
modded class GC_LobbyManager
{
	protected ref array<string> m_aGC_SceneryRows;

	//------------------------------------------------------------------------------------------------
	string GC_TestSceneryExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "scenery-export", "Arland"))
			return "denied";
		string world = GetGame().GetWorldFile();
		world.ToLower();
		if (!world.Contains("worlds/examples/arland/arland_lobby.ent"))
			return "unsupported-world";
		m_aGC_SceneryRows = {};
		vector mins, maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		bool complete = GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, GC_TestSceneryEntity);
		if (!complete)
			return "incomplete-query";
		FileHandle output = FileIO.OpenFile("$profile:GC_MapScenery_Arland.tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("kind\tx\tz\trx\trz\tresource\tbottom\ttop\tsubmergedDepth");
		foreach (string row : m_aGC_SceneryRows)
			output.WriteLine(row);
		output.Close();
		string result = "exported=" + m_aGC_SceneryRows.Count();
		m_aGC_SceneryRows = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "scenery-export", "Arland", result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_TestSceneryEntity(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return true;
		string resource = entity.GetPrefabData().GetPrefabName();
		string path = resource;
		path.ToLower();
		int kind = -1;
		Tree tree = Tree.Cast(entity);
		if (tree)
		{
			TreeClass data = TreeClass.Cast(tree.GetPrefabData());
			if (data)
				kind = data.SoundType;
		}
		else if (path.Contains("/rocks/"))
			kind = 100;
		if (kind < 0)
			return true;
		vector mins, maxs;
		entity.GetWorldBounds(mins, maxs);
		vector center = (mins + maxs) * 0.5;
		float rx = (maxs[0] - mins[0]) * 0.5;
		float rz = (maxs[2] - mins[2]) * 0.5;
		if (center[0] < 0 || center[0] > 4096 || center[2] < 0 || center[2] > 4096 || rx <= 0 || rz <= 0)
			return true;
		// Query at the top of the world bounds: only fully submerged objects fade.
		// This water query does not trace through vegetation or rock geometry.
		vector probe = Vector(center[0], maxs[1], center[2]);
		vector surface, transform[4], extents;
		EWaterSurfaceType waterType;
		float submergedDepth;
		if (ChimeraWorldUtils.TryGetWaterSurface(GetGame().GetWorld(), probe, surface, waterType, transform, extents))
			submergedDepth = Math.Max(0, surface[1] - maxs[1]);
		string row = string.Format("%1\t%2\t%3\t%4\t%5\t%6", kind, center[0], center[2], rx, rz, resource);
		row += string.Format("\t%1\t%2\t%3", mins[1], maxs[1], submergedDepth);
		m_aGC_SceneryRows.Insert(row);
		return true;
	}
}
#endif
