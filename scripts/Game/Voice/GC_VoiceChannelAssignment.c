enum GC_VoiceChannelType
{
	GLOBAL = 0,
	FACTION = 1,
	GROUP = 2,
	PERSONAL = 3
}

enum GC_EBriefingVoiceChannel
{
	GROUP = 0,
	FACTION_HQ = 1
}

//! One server-authoritative active voice channel for one connected player.
class GC_VoiceChannelAssignment
{
	protected int m_iPlayerId = -1;
	//! Server-resolved base-game identity reference used by client presentation.
	protected UUID m_PlayerIdentity;
	protected GC_VoiceChannelType m_eChannelType = GC_VoiceChannelType.GLOBAL;
	protected string m_sFactionKey;
	protected RplId m_GroupRplId = RplId.Invalid();
	protected UUID m_PersonalOwnerIdentity;
	//! Authority-only provenance; clients render the replicated destination itself.
	protected bool m_bAdminAssigned;
	protected bool m_bDeafened;
	protected bool m_bAdminMuted;
	protected bool m_bPrioritySpeaker;
	protected bool m_bPriorityTransmitting;


	//------------------------------------------------------------------------------------------------
	bool IsAdminAssigned()
	{
		return m_bAdminAssigned;
	}

	//------------------------------------------------------------------------------------------------
	void SetAdminAssigned(bool value)
	{
		m_bAdminAssigned = value;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAdminMuted()
	{
		return m_bAdminMuted;
	}

	//------------------------------------------------------------------------------------------------
	void SetAdminMuted(bool value)
	{
		m_bAdminMuted = value;
	}

	//------------------------------------------------------------------------------------------------
	bool IsPrioritySpeaker()
	{
		return m_bPrioritySpeaker;
	}

	//------------------------------------------------------------------------------------------------
	void SetPrioritySpeaker(bool value)
	{
		m_bPrioritySpeaker = value;
	}

	//------------------------------------------------------------------------------------------------
	bool IsPriorityTransmitting()
	{
		return m_bPriorityTransmitting;
	}

	//------------------------------------------------------------------------------------------------
	void SetPriorityTransmitting(bool value)
	{
		m_bPriorityTransmitting = value;
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	UUID GetPlayerIdentity()
	{
		return m_PlayerIdentity;
	}

	GC_VoiceChannelType GetChannelType()
	{
		return m_eChannelType;
	}

	string GetFactionKey()
	{
		return m_sFactionKey;
	}

	RplId GetGroupRplId()
	{
		return m_GroupRplId;
	}

	UUID GetPersonalOwnerIdentity()
	{
		return m_PersonalOwnerIdentity;
	}

	bool IsDeafened()
	{
		return m_bDeafened;
	}

	void SetDeafened(bool deafened)
	{
		m_bDeafened = deafened;
	}

	void SetPlayerIdentity(UUID playerIdentity)
	{
		m_PlayerIdentity = playerIdentity;
	}

	void Set(int playerId, GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		m_iPlayerId = playerId;
		m_eChannelType = channelType;
		m_sFactionKey = factionKey;
		m_GroupRplId = groupRplId;
		m_PersonalOwnerIdentity = personalOwnerIdentity;
	}

	void CopyFrom(GC_VoiceChannelAssignment source)
	{
		if (!source)
			return;

		Set(source.m_iPlayerId, source.m_eChannelType, source.m_sFactionKey, source.m_GroupRplId, source.m_PersonalOwnerIdentity);
		m_PlayerIdentity = source.m_PlayerIdentity;
		m_bDeafened = source.m_bDeafened;
		m_bAdminMuted = source.m_bAdminMuted;
		m_bPrioritySpeaker = source.m_bPrioritySpeaker;
		m_bPriorityTransmitting = source.m_bPriorityTransmitting;

	}

	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPlayerId);
		string playerIdentity = m_PlayerIdentity;
		writer.WriteString(playerIdentity);
		writer.WriteIntRange(m_eChannelType, GC_VoiceChannelType.GLOBAL, GC_VoiceChannelType.PERSONAL);
		writer.WriteString(m_sFactionKey);
		writer.WriteRplId(m_GroupRplId);
		string personalOwnerIdentity = m_PersonalOwnerIdentity;
		writer.WriteString(personalOwnerIdentity);
		writer.WriteBool(m_bDeafened);
		writer.WriteBool(m_bAdminMuted);
		writer.WriteBool(m_bPrioritySpeaker);
		writer.WriteBool(m_bPriorityTransmitting);

		return true;
	}

	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iPlayerId))
			return false;
		string playerIdentity;
		if (!reader.ReadString(playerIdentity))
			return false;
		m_PlayerIdentity = playerIdentity;
		if (!reader.ReadIntRange(m_eChannelType, GC_VoiceChannelType.GLOBAL, GC_VoiceChannelType.PERSONAL))
			return false;
		if (!reader.ReadString(m_sFactionKey))
			return false;
		if (!reader.ReadRplId(m_GroupRplId))
			return false;
		string personalOwnerIdentity;
		if (!reader.ReadString(personalOwnerIdentity))
			return false;
		m_PersonalOwnerIdentity = personalOwnerIdentity;
		if (!reader.ReadBool(m_bDeafened))
			return false;
		if (!reader.ReadBool(m_bAdminMuted))
			return false;
		if (!reader.ReadBool(m_bPrioritySpeaker))
			return false;
		if (!reader.ReadBool(m_bPriorityTransmitting))
			return false;


		return true;
	}

	static bool Extract(GC_VoiceChannelAssignment instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iPlayerId);
		string playerIdentity = instance.m_PlayerIdentity;
		snapshot.SerializeString(playerIdentity);
		snapshot.SerializeInt(instance.m_eChannelType);
		snapshot.SerializeString(instance.m_sFactionKey);
		RplId groupRplId = instance.m_GroupRplId;
		snapshot.SerializeBytes(groupRplId, 4);
		string personalOwnerIdentity = instance.m_PersonalOwnerIdentity;
		snapshot.SerializeString(personalOwnerIdentity);
		snapshot.SerializeBool(instance.m_bDeafened);
		snapshot.SerializeBool(instance.m_bAdminMuted);
		snapshot.SerializeBool(instance.m_bPrioritySpeaker);
		snapshot.SerializeBool(instance.m_bPriorityTransmitting);

		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_VoiceChannelAssignment instance)
	{
		snapshot.SerializeInt(instance.m_iPlayerId);
		string playerIdentity;
		snapshot.SerializeString(playerIdentity);
		instance.m_PlayerIdentity = playerIdentity;
		int channelType;
		snapshot.SerializeInt(channelType);
		instance.m_eChannelType = channelType;
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeBytes(instance.m_GroupRplId, 4);
		string personalOwnerIdentity;
		snapshot.SerializeString(personalOwnerIdentity);
		instance.m_PersonalOwnerIdentity = personalOwnerIdentity;
		snapshot.SerializeBool(instance.m_bDeafened);
		snapshot.SerializeBool(instance.m_bAdminMuted);
		snapshot.SerializeBool(instance.m_bPrioritySpeaker);
		snapshot.SerializeBool(instance.m_bPriorityTransmitting);

		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		RplId groupRplId;
		snapshot.SerializeBytes(groupRplId, 4);
		packet.Serialize(groupRplId, 32);
		snapshot.EncodeString(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);

	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		RplId groupRplId;
		packet.Serialize(groupRplId, 32);
		snapshot.SerializeBytes(groupRplId, 4);
		snapshot.DecodeString(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);

		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(GC_VoiceChannelAssignment instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareString(instance.m_PlayerIdentity)
			&& snapshot.CompareInt(instance.m_eChannelType)
			&& snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.Compare(instance.m_GroupRplId, 4)
			&& snapshot.CompareString(instance.m_PersonalOwnerIdentity)
			&& snapshot.CompareBool(instance.m_bDeafened)
			&& snapshot.CompareBool(instance.m_bAdminMuted)
			&& snapshot.CompareBool(instance.m_bPrioritySpeaker)
			&& snapshot.CompareBool(instance.m_bPriorityTransmitting);
	}
}
