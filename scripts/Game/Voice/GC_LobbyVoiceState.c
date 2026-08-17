//! Owns Lobby2-only voice presentation state. Audio and authoritative group
//! membership remain owned by the base game.
class GC_LobbyVoiceState
{
	protected ref array<ref GC_VoiceChannelAssignment> m_aAssignments = {};

	GC_VoiceChannelAssignment Get(int playerId)
	{
		foreach (GC_VoiceChannelAssignment assignment : m_aAssignments)
		{
			if (assignment && assignment.GetPlayerId() == playerId)
				return assignment;
		}

		return null;
	}

	bool IsDeafened(int playerId)
	{
		GC_VoiceChannelAssignment assignment = Get(playerId);
		return assignment && assignment.IsDeafened();
	}

	bool SetDeafened(int playerId, bool deafened, out GC_VoiceChannelAssignment changed)
	{
		changed = Get(playerId);
		if (!changed)
		{
			changed = new GC_VoiceChannelAssignment();
			changed.Set(playerId, GC_VoiceChannelType.GLOBAL);
			m_aAssignments.Insert(changed);
		}

		if (changed.IsDeafened() == deafened)
			return false;

		changed.SetDeafened(deafened);
		return true;
	}

	bool SetChannel(int playerId, GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, out GC_VoiceChannelAssignment changed)
	{
		changed = Get(playerId);
		if (!changed)
		{
			changed = new GC_VoiceChannelAssignment();
			m_aAssignments.Insert(changed);
		}
		else if (changed.GetChannelType() == channelType && changed.GetFactionKey() == factionKey && changed.GetGroupRplId() == groupRplId)
		{
			return false;
		}

		bool deafened = changed.IsDeafened();
		changed.Set(playerId, channelType, factionKey, groupRplId);
		changed.SetDeafened(deafened);
		return true;
	}

	bool Remove(int playerId)
	{
		for (int i = m_aAssignments.Count() - 1; i >= 0; i--)
		{
			GC_VoiceChannelAssignment assignment = m_aAssignments[i];
			if (!assignment || assignment.GetPlayerId() != playerId)
				continue;

			m_aAssignments.RemoveOrdered(i);
			return true;
		}

		return false;
	}

	bool Apply(GC_VoiceChannelAssignment update)
	{
		if (!update)
			return false;

		GC_VoiceChannelAssignment assignment = Get(update.GetPlayerId());
		if (!assignment)
		{
			assignment = new GC_VoiceChannelAssignment();
			m_aAssignments.Insert(assignment);
		}

		assignment.CopyFrom(update);
		return true;
	}

	bool Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_aAssignments.Count());
		foreach (GC_VoiceChannelAssignment assignment : m_aAssignments)
		{
			if (!assignment || !assignment.RplSave(writer))
				return false;
		}

		return true;
	}

	bool Load(ScriptBitReader reader)
	{
		int assignmentCount;
		if (!reader.ReadInt(assignmentCount) || assignmentCount < 0)
			return false;

		m_aAssignments.Clear();
		for (int assignmentIndex; assignmentIndex < assignmentCount; assignmentIndex++)
		{
			GC_VoiceChannelAssignment assignment = new GC_VoiceChannelAssignment();
			if (!assignment.RplLoad(reader))
				return false;

			m_aAssignments.Insert(assignment);
		}

		return true;
	}

	void Clear()
	{
		m_aAssignments.Clear();
	}
}
