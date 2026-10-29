//! Bounded server-side combat references, independent of the general session-log ring.
class GC_SpectatorEventBuffer
{
	protected ref array<ref GC_SessionEvent> m_aEvents = {};
	protected ref map<int, GC_SessionEvent> m_mFatalHits = new map<int, GC_SessionEvent>();
	protected ref set<int> m_aCombinedHits = new set<int>();
	protected int m_iCapacity;
	protected int m_iCursor;
	protected int m_iRevision;
	protected int m_iIndexRevision = -1;

	void GC_SpectatorEventBuffer(int capacity)
	{
		m_iCapacity = capacity;
	}

	void Insert(GC_SessionEvent entry)
	{
		if (m_aEvents.Count() < m_iCapacity)
			m_aEvents.Insert(entry);
		else
		{
			m_aEvents[m_iCursor] = entry;
			m_iCursor = (m_iCursor + 1) % m_iCapacity;
		}
		m_iRevision = entry.m_iSequence;
	}

	int Count()
	{
		return m_aEvents.Count();
	}

	GC_SessionEvent GetNewest(int offset)
	{
		int count = Count();
		int start = count - 1;
		if (count == m_iCapacity)
			start = (m_iCursor + count - 1) % count;
		return m_aEvents[(start - offset + count) % count];
	}

	//------------------------------------------------------------------------------------------------
	//! Pair fatal damage once per buffer revision without changing canonical records.
	void UpdateIndex()
	{
		if (m_iIndexRevision == m_iRevision)
			return;
		m_iIndexRevision = m_iRevision;
		map<string, GC_SessionEvent> hits = new map<string, GC_SessionEvent>();
		m_mFatalHits.Clear();
		m_aCombinedHits.Clear();
		for (int i = Count() - 1; i >= 0; i--)
		{
			GC_SessionEvent entry = GetNewest(i);
			if (entry.m_sType == "damage")
				hits.Set(entry.m_sTargetEntity, entry);
			else
			{
				GC_SessionEvent hit;
				if (entry.m_sDeathCause != "bleeding" && entry.m_sDeathCause != "respawn"
					&& !entry.m_sTargetEntity.IsEmpty() && hits.Find(entry.m_sTargetEntity, hit)
					&& entry.m_sActor == hit.m_sActor && entry.m_fSeconds == hit.m_fSeconds)
				{
					m_mFatalHits.Set(entry.m_iSequence, hit);
					m_aCombinedHits.Insert(hit.m_iSequence);
				}
			}
		}
	}

	bool IsCombined(int sequence)
	{
		return m_aCombinedHits.Contains(sequence);
	}

	bool GetFatalHit(int sequence, out GC_SessionEvent hit)
	{
		return m_mFatalHits.Find(sequence, hit);
	}
}
