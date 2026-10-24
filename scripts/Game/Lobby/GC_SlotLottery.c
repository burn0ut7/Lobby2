//! One shared draw. Only the manager authority mutates this state.
class GC_SlotLottery
{
	int m_iId;
	//! Authority-only audit attribution; clients never supply the administrator.
	int m_iRequesterId;
	WorldTimestamp m_StartedAt;
	string m_sLabel;
	ref array<int> m_aSlotIds = {};
	ref array<string> m_aEntrants = {};

	//------------------------------------------------------------------------------------------------
	int GetSecondsRemaining()
	{
		ChimeraWorld world = GetGame().GetWorld();
		return Math.Max(0, 30 - Math.Floor(world.GetServerTimestamp().DiffSeconds(m_StartedAt)));
	}

	//------------------------------------------------------------------------------------------------
	bool Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iId);
		writer.Write(m_StartedAt, 64);
		writer.WriteString(m_sLabel);
		writer.WriteInt(m_aSlotIds.Count());
		foreach (int slotId : m_aSlotIds)
			writer.WriteInt(slotId);
		writer.WriteInt(m_aEntrants.Count());
		foreach (string identity : m_aEntrants)
			writer.WriteString(identity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Load(ScriptBitReader reader)
	{
		int slotCount, entrantCount;
		if (!reader.ReadInt(m_iId) || !reader.Read(m_StartedAt, 64)
			|| !reader.ReadString(m_sLabel) || !reader.ReadInt(slotCount) || slotCount < 0 || slotCount > 512)
			return false;
		m_aSlotIds.Clear();
		for (int i; i < slotCount; i++)
		{
			int slotId;
			if (!reader.ReadInt(slotId))
				return false;
			m_aSlotIds.Insert(slotId);
		}
		if (!reader.ReadInt(entrantCount) || entrantCount < 0 || entrantCount > 256)
			return false;
		m_aEntrants.Clear();
		for (int i; i < entrantCount; i++)
		{
			string identity;
			if (!reader.ReadString(identity))
				return false;
			m_aEntrants.Insert(identity);
		}
		return true;
	}
}
