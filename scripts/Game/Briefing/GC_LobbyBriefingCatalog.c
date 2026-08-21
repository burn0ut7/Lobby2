[BaseContainerProps()]
class GC_LobbyBriefingCatalog
{
	static const int MAX_BRIEFINGS = 32;
	static const int MAX_BRIEFING_TITLE_LENGTH = 128;
	static const int MAX_BRIEFING_CONTENT_LENGTH = 16384;
	static const int MAX_MARKERS = 256;
	static const int MAX_MARKER_NAME_LENGTH = 128;
	static const int MAX_MARKER_DESCRIPTION_LENGTH = 512;

	[Attribute("", UIWidgets.Object, "Server-authoritative briefing definitions")]
	protected ref array<ref GC_Briefing> m_aBriefings;
	protected ref array<ref GC_BriefingMarker> m_aMarkers = {};
	protected int m_iBriefingRevision;
	protected int m_iMarkerRevision;

	protected void EnsureBriefings()
	{
		if (!m_aBriefings)
			m_aBriefings = {};
	}

	int GetBriefingRevision() { return m_iBriefingRevision; }
	int GetMarkerRevision() { return m_iMarkerRevision; }

	GC_Briefing GetBriefing(int order)
	{
		if (order <= 0)
			return null;

		EnsureBriefings();
		foreach (GC_Briefing briefing : m_aBriefings)
		{
			if (briefing && briefing.GetOrder() == order)
				return briefing;
		}

		return null;
	}

	array<ref GC_Briefing> GetBriefings()
	{
		EnsureBriefings();
		return m_aBriefings;
	}

	array<ref GC_Briefing> GetBriefingsFor(Faction faction, SCR_EGameModeState phase)
	{
		array<ref GC_Briefing> result = {};
		foreach (GC_Briefing briefing : GetBriefings())
		{
			if (briefing && briefing.IsVisibleToFaction(faction, phase))
				result.Insert(briefing);
		}

		return result;
	}

	bool UpsertBriefing(GC_Briefing update, out GC_Briefing applied)
	{
		applied = GetBriefing(update.GetOrder());
		if (!applied)
		{
			if (GetBriefings().Count() >= MAX_BRIEFINGS)
				return false;

			applied = new GC_Briefing();
			m_aBriefings.Insert(applied);
		}

		applied.CopyFrom(update);
		applied.SetRevision(++m_iBriefingRevision);
		return true;
	}

	bool RemoveBriefing(int order, out int revision)
	{
		EnsureBriefings();
		for (int index; index < m_aBriefings.Count(); index++)
		{
			GC_Briefing briefing = m_aBriefings[index];
			if (!briefing || briefing.GetOrder() != order)
				continue;

			m_aBriefings.RemoveOrdered(index);
			revision = ++m_iBriefingRevision;
			return true;
		}

		return false;
	}

	bool ApplyBriefing(GC_Briefing update)
	{
		if (!update)
			return false;

		GC_Briefing briefing = GetBriefing(update.GetOrder());
		if (briefing && update.GetRevision() <= briefing.GetRevision())
			return false;

		if (!briefing)
		{
			if (GetBriefings().Count() >= MAX_BRIEFINGS)
				return false;

			briefing = new GC_Briefing();
			m_aBriefings.Insert(briefing);
		}

		briefing.CopyFrom(update);
		if (update.GetRevision() > m_iBriefingRevision)
			m_iBriefingRevision = update.GetRevision();
		return true;
	}

	bool ApplyBriefingRemoval(int order, int revision)
	{
		if (revision <= m_iBriefingRevision)
			return false;

		EnsureBriefings();
		for (int index = m_aBriefings.Count() - 1; index >= 0; index--)
		{
			GC_Briefing briefing = m_aBriefings[index];
			if (briefing && briefing.GetOrder() == order)
				m_aBriefings.RemoveOrdered(index);
		}

		m_iBriefingRevision = revision;
		return true;
	}

	GC_BriefingMarker GetMarker(string name)
	{
		foreach (GC_BriefingMarker marker : m_aMarkers)
		{
			if (marker && marker.GetName() == name)
				return marker;
		}

		return null;
	}

	array<ref GC_BriefingMarker> GetMarkers()
	{
		return m_aMarkers;
	}

	array<ref GC_BriefingMarker> GetMarkersFor(Faction faction, SCR_EGameModeState phase)
	{
		array<ref GC_BriefingMarker> result = {};
		foreach (GC_BriefingMarker marker : m_aMarkers)
		{
			if (marker && marker.IsVisibleToFaction(faction, phase))
				result.Insert(marker);
		}

		return result;
	}

	bool UpsertMarker(GC_BriefingMarker update, out GC_BriefingMarker applied)
	{
		applied = GetMarker(update.GetName());
		if (!applied)
		{
			if (m_aMarkers.Count() >= MAX_MARKERS)
				return false;

			applied = new GC_BriefingMarker();
			m_aMarkers.Insert(applied);
		}

		applied.CopyFrom(update);
		applied.SetRevision(++m_iMarkerRevision);
		return true;
	}

	bool RemoveMarker(string name, out int revision)
	{
		for (int index; index < m_aMarkers.Count(); index++)
		{
			GC_BriefingMarker marker = m_aMarkers[index];
			if (!marker || marker.GetName() != name)
				continue;

			m_aMarkers.RemoveOrdered(index);
			revision = ++m_iMarkerRevision;
			return true;
		}

		return false;
	}

	bool ApplyMarker(GC_BriefingMarker update)
	{
		if (!update)
			return false;

		GC_BriefingMarker marker = GetMarker(update.GetName());
		if (marker && update.GetRevision() <= marker.GetRevision())
			return false;

		if (!marker)
		{
			if (m_aMarkers.Count() >= MAX_MARKERS)
				return false;

			marker = new GC_BriefingMarker();
			m_aMarkers.Insert(marker);
		}

		marker.CopyFrom(update);
		if (update.GetRevision() > m_iMarkerRevision)
			m_iMarkerRevision = update.GetRevision();
		return true;
	}

	bool ApplyMarkerRemoval(string name, int revision)
	{
		if (revision <= m_iMarkerRevision)
			return false;

		for (int index = m_aMarkers.Count() - 1; index >= 0; index--)
		{
			GC_BriefingMarker marker = m_aMarkers[index];
			if (marker && marker.GetName() == name)
				m_aMarkers.RemoveOrdered(index);
		}

		m_iMarkerRevision = revision;
		return true;
	}

	bool Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iBriefingRevision);
		writer.WriteInt(GetBriefings().Count());
		foreach (GC_Briefing briefing : m_aBriefings)
		{
			if (!briefing || !briefing.RplSave(writer))
				return false;
		}

		writer.WriteInt(m_iMarkerRevision);
		writer.WriteInt(m_aMarkers.Count());
		foreach (GC_BriefingMarker marker : m_aMarkers)
		{
			if (!marker || !marker.RplSave(writer))
				return false;
		}

		return true;
	}

	bool Load(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iBriefingRevision))
			return false;

		int briefingCount;
		if (!reader.ReadInt(briefingCount) || briefingCount < 0 || briefingCount > MAX_BRIEFINGS)
			return false;

		GetBriefings().Clear();
		for (int briefingIndex; briefingIndex < briefingCount; briefingIndex++)
		{
			GC_Briefing briefing = new GC_Briefing();
			if (!briefing.RplLoad(reader))
				return false;
			m_aBriefings.Insert(briefing);
		}

		if (!reader.ReadInt(m_iMarkerRevision))
			return false;

		int markerCount;
		if (!reader.ReadInt(markerCount) || markerCount < 0 || markerCount > MAX_MARKERS)
			return false;

		m_aMarkers.Clear();
		for (int markerIndex; markerIndex < markerCount; markerIndex++)
		{
			GC_BriefingMarker marker = new GC_BriefingMarker();
			if (!marker.RplLoad(reader))
				return false;
			m_aMarkers.Insert(marker);
		}

		return true;
	}

	void Clear()
	{
		GetBriefings().Clear();
		m_aMarkers.Clear();
	}
}
