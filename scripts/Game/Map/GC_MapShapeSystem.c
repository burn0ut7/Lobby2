//! Authority owns geometry revisions. Clients retain their latest permitted snapshot.
class GC_MapShapeSystem
{
	protected ref array<GC_MapShapeComponent> m_aShapes = {};
	protected int m_iRevision;

	//------------------------------------------------------------------------------------------------
	void Changed()
	{
		m_iRevision++;
	}

	//------------------------------------------------------------------------------------------------
	int GetRevision()
	{
		return m_iRevision;
	}

	//------------------------------------------------------------------------------------------------
	void Register(GC_MapShapeComponent shape)
	{
		if (Replication.IsServer() && !m_aShapes.Contains(shape))
		{
			m_aShapes.Insert(shape);
			Changed();
		}
	}

	//------------------------------------------------------------------------------------------------
	void Unregister(GC_MapShapeComponent shape)
	{
		m_aShapes.RemoveItem(shape);
		Changed();
	}

	//------------------------------------------------------------------------------------------------
	void GetSnapshot(int playerId, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<string> names = null, array<vector> frameBounds = null)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || mode.GetMapShapes() != this || !GC_LobbyManager.GetManager())
			return;
		foreach (GC_MapShapeComponent shape : m_aShapes)
		{
			if (shape)
				shape.AppendSnapshot(playerId, counts, colors, fills, inverted, widths, points, frameBounds);
		}
		if (names)
		{
			while (names.Count() < counts.Count())
				names.Insert("");
			mode.GetBoundaryAdmin().AppendMap(playerId, counts, colors, fills, inverted, widths, points, names);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted recorder read. Administrative boundary hints are not player map content.
	void GetFactionSnapshot(Faction faction, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || mode.GetMapShapes() != this)
			return;
		foreach (GC_MapShapeComponent shape : m_aShapes)
		{
			if (shape)
				shape.AppendFactionSnapshot(faction, counts, colors, fills, inverted, widths, points);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_aShapes.Clear();
	}
}
