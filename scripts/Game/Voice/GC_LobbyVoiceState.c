//! Owns Lobby2-only voice presentation state. Audio and authoritative group
//! membership remain owned by the base game.
class GC_LobbyVoiceState
{
	protected ref array<ref GC_VoiceChannelAssignment> m_aAssignments = {};
	protected ref array<UUID> m_aPersonalChannelOwners = {};
	//! Server-only transport allocation; controllers replicate resolved frequencies.
	protected ref map<string, int> m_mFrequencies = new map<string, int>();
	protected int m_iNextFrequency = 2;

	int GetFrequency(string roomKey)
	{
		if (!Replication.IsServer() || roomKey.IsEmpty())
			return GC_LobbyRadio.INACTIVE_FREQUENCY;
		if (!m_mFrequencies.Contains(roomKey))
		{
			if (m_iNextFrequency >= GC_LobbyRadio.INACTIVE_FREQUENCY)
				return GC_LobbyRadio.INACTIVE_FREQUENCY;
			m_mFrequencies.Set(roomKey, m_iNextFrequency);
			m_iNextFrequency++;
		}
		return m_mFrequencies.Get(roomKey);
	}

	bool RegisterPersonalChannel(UUID ownerIdentity)
	{
		if (ownerIdentity.IsEmpty() || m_aPersonalChannelOwners.Contains(ownerIdentity))
			return false;

		m_aPersonalChannelOwners.Insert(ownerIdentity);
		if (Replication.IsServer())
			GetFrequency("personal:" + ownerIdentity);
		return true;
	}

	bool HasPersonalChannel(UUID ownerIdentity)
	{
		return !ownerIdentity.IsEmpty() && m_aPersonalChannelOwners.Contains(ownerIdentity);
	}

	GC_VoiceChannelAssignment Get(int playerId)
	{
		foreach (GC_VoiceChannelAssignment assignment : m_aAssignments)
		{
			if (assignment && assignment.GetPlayerId() == playerId)
				return assignment;
		}

		return null;
	}

	UUID GetPlayerIdentity(int playerId)
	{
		GC_VoiceChannelAssignment assignment = Get(playerId);
		if (!assignment)
			return UUID.NULL_UUID;

		return assignment.GetPlayerIdentity();
	}

	int GetPlayerIdByIdentity(UUID identity)
	{
		if (identity.IsEmpty())
			return -1;

		foreach (GC_VoiceChannelAssignment assignment : m_aAssignments)
		{
			if (assignment && assignment.GetPlayerIdentity() == identity)
				return assignment.GetPlayerId();
		}

		return -1;
	}

	bool SetPlayerIdentity(int playerId, UUID identity, out GC_VoiceChannelAssignment changed)
	{
		changed = Get(playerId);
		if (playerId <= 0 || identity.IsEmpty())
			return false;

		if (!changed)
		{
			changed = new GC_VoiceChannelAssignment();
			changed.Set(playerId, GC_VoiceChannelType.GLOBAL);
			m_aAssignments.Insert(changed);
		}
		else if (changed.GetPlayerIdentity() == identity)
		{
			return false;
		}

		changed.SetPlayerIdentity(identity);
		return true;
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

	bool SetChannel(int playerId, GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, UUID personalOwnerIdentity, out GC_VoiceChannelAssignment changed)
	{
		changed = Get(playerId);
		if (!changed)
		{
			changed = new GC_VoiceChannelAssignment();
			m_aAssignments.Insert(changed);
		}
		else if (changed.GetChannelType() == channelType && changed.GetFactionKey() == factionKey && changed.GetGroupRplId() == groupRplId && changed.GetPersonalOwnerIdentity() == personalOwnerIdentity)
		{
			return false;
		}

		bool deafened = changed.IsDeafened();
		changed.Set(playerId, channelType, factionKey, groupRplId, personalOwnerIdentity);
		changed.SetDeafened(deafened);
		return true;
	}

	array<int> GetPersonalChannelMembers(UUID ownerIdentity)
	{
		array<int> members = {};
		foreach (GC_VoiceChannelAssignment assignment : m_aAssignments)
		{
			if (assignment && assignment.GetChannelType() == GC_VoiceChannelType.PERSONAL && assignment.GetPersonalOwnerIdentity() == ownerIdentity)
				members.Insert(assignment.GetPlayerId());
		}

		return members;
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
		writer.WriteInt(m_aPersonalChannelOwners.Count());
		foreach (UUID ownerIdentity : m_aPersonalChannelOwners)
		{
			string serializedIdentity = ownerIdentity;
			writer.WriteString(serializedIdentity);
		}

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
		int personalChannelCount;
		if (!reader.ReadInt(personalChannelCount) || personalChannelCount < 0)
			return false;

		m_aPersonalChannelOwners.Clear();
		for (int personalChannelIndex; personalChannelIndex < personalChannelCount; personalChannelIndex++)
		{
			string ownerIdentity;
			if (!reader.ReadString(ownerIdentity))
				return false;
			m_aPersonalChannelOwners.Insert(ownerIdentity);
		}

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
		m_mFrequencies.Clear();
		m_iNextFrequency = 2;
		m_aAssignments.Clear();
		m_aPersonalChannelOwners.Clear();
	}
}
