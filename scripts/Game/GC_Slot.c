enum GC_SlotState
{
    FREE,
    LOCKED,
    DEAD,
    DISCONNECTED,
    RESERVED
}

class GC_Slot
{
	//! Unique slot ID
	protected int m_iID;

	//! Current server-session player identifier.
	protected int m_iPlayerId = -1;

	//! Persistent player identity identifier.
	protected UUID m_PlayerUUID;

	//! Last known player name for lobby display.
	protected string m_sCachedPlayerName;

	//! Current slot state represented by this slot.
	protected GC_SlotState m_eState = GC_SlotState.FREE;

	//! Character or spectator prefab assigned to this slot.
	protected ResourceName m_Prefab;

	//! Runtime group assigned to this slot, if any.
	protected SCR_AIGroup m_pGroup;

	//! Entity spawned for and currently represented by this slot.
	protected IEntity m_Entity;

	int GetID()
	{
		return m_iID;
	}

	void SetID(int id)
	{
		m_iID = id;
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	UUID GetUUID()
	{
		return m_PlayerUUID;
	}

	string GetCachedPlayerName()
	{
		return m_sCachedPlayerName;
	}

	void SetCachedPlayerName(string playerName)
	{
		m_sCachedPlayerName = playerName;
	}

	GC_SlotState GetState()
	{
		return m_eState;
	}

	void SetState(GC_SlotState state)
	{
		if (m_eState == state)
			return;

		m_eState = state;
	}

	ResourceName GetPrefab()
	{
		return m_Prefab;
	}

	void SetPrefab(ResourceName prefab)
	{
		m_Prefab = prefab;
	}

	SCR_AIGroup GetGroup()
	{
		return m_pGroup;
	}

	void SetGroup(SCR_AIGroup group)
	{
		m_pGroup = group;
	}

	IEntity GetEntity()
	{
		return m_Entity;
	}

	void SetEntity(IEntity entity)
	{
		m_Entity = entity;
	}

	//! Returns the RPL ID of the slot entity's replication component.
	RplId GetEntityRplId()
	{
		if (!m_Entity)
			return RplId.Invalid();

		RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(m_Entity);
		if (!rplComponent)
			return RplId.Invalid();

		return rplComponent.Id();
	}

	//! Sets the complete player assignment from the current server player ID.
	//! An invalid player ID clears the int, UUID, and cached name.
	void SetPlayer(int playerId)
	{
		if (playerId < 0)
		{
			m_iPlayerId = -1;
			m_PlayerUUID = "";
			m_sCachedPlayerName = "";
		}
		else
		{
			string playerIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			m_iPlayerId = playerId;
			m_PlayerUUID = playerIdentity;
			m_sCachedPlayerName = GetGame().GetPlayerManager().GetPlayerNameByIdentity(playerIdentity);
			if (!m_sCachedPlayerName)
				m_sCachedPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		}
	}

	//! Copies a complete server-originated slot state into this local slot.
	//! This does not call Update(), so receiving a network update cannot rebroadcast it.
	void CopyFrom(GC_Slot source)
	{
		if (!source)
			return;

		m_iID = source.m_iID;
		m_iPlayerId = source.m_iPlayerId;
		m_PlayerUUID = source.m_PlayerUUID;
		m_sCachedPlayerName = source.m_sCachedPlayerName;
		m_eState = source.m_eState;
		m_Prefab = source.m_Prefab;
		m_pGroup = source.m_pGroup;
		m_Entity = source.m_Entity;
	}

	//! Serializes this slot as part of its owning replicated component.
	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iID);
		return RplSaveData(writer);
	}

	//! Serializes all slot data after the slot ID.
	bool RplSaveData(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPlayerId);

		string uuid = m_PlayerUUID;
		writer.WriteString(uuid);
		writer.WriteString(m_sCachedPlayerName);
		writer.WriteIntRange(m_eState, GC_SlotState.FREE, GC_SlotState.RESERVED);
		writer.WriteResourceName(m_Prefab);

		RplId groupRplId = RplId.Invalid();
		if (m_pGroup)
			groupRplId = Replication.FindItemId(m_pGroup);

		writer.WriteRplId(groupRplId);
		writer.WriteRplId(GetEntityRplId());
		return true;
	}

	//! Deserializes this slot, including its serialized ID.
	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iID))
			return false;

		return RplLoadData(reader);
	}

	//! Deserializes all slot data after the constructor has received the ID.
	bool RplLoadData(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iPlayerId))
			return false;

		string uuid;
		if (!reader.ReadString(uuid))
			return false;
		m_PlayerUUID = uuid;

		if (!reader.ReadString(m_sCachedPlayerName))
			return false;
		if (!reader.ReadIntRange(m_eState, GC_SlotState.FREE, GC_SlotState.RESERVED))
			return false;
		if (!reader.ReadResourceName(m_Prefab))
			return false;

		RplId groupRplId;
		if (!reader.ReadRplId(groupRplId))
			return false;

		m_pGroup = SCR_AIGroup.Cast(Replication.FindItem(groupRplId));

		RplId entityRplId;
		if (!reader.ReadRplId(entityRplId))
			return false;

		RplComponent entityRplComponent = RplComponent.Cast(Replication.FindItem(entityRplId));
		if (entityRplComponent)
			m_Entity = entityRplComponent.GetEntity();
		else
			m_Entity = null;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Extracts this slot into an EN Network snapshot.
	static bool Extract(GC_Slot instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iID);
		snapshot.SerializeInt(instance.m_iPlayerId);

		string uuid = instance.m_PlayerUUID;
		snapshot.SerializeString(uuid);
		snapshot.SerializeString(instance.m_sCachedPlayerName);
		snapshot.SerializeInt(instance.m_eState);

		ResourceName prefab = instance.m_Prefab;
		snapshot.SerializeString(prefab);

		RplId groupRplId = RplId.Invalid();
		if (instance.m_pGroup)
			groupRplId = Replication.FindItemId(instance.m_pGroup);
		RplId entityRplId = instance.GetEntityRplId();
		snapshot.SerializeBytes(groupRplId, 4);
		snapshot.SerializeBytes(entityRplId, 4);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Injects an EN Network snapshot into this slot.
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_Slot instance)
	{
		snapshot.SerializeInt(instance.m_iID);
		snapshot.SerializeInt(instance.m_iPlayerId);

		string uuid;
		snapshot.SerializeString(uuid);
		instance.m_PlayerUUID = uuid;
		snapshot.SerializeString(instance.m_sCachedPlayerName);

		int state;
		snapshot.SerializeInt(state);
		instance.m_eState = state;

		ResourceName prefab;
		snapshot.SerializeString(prefab);
		instance.m_Prefab = prefab;

		RplId groupRplId;
		snapshot.SerializeBytes(groupRplId, 4);
		instance.m_pGroup = SCR_AIGroup.Cast(Replication.FindItem(groupRplId));

		RplId entityRplId;
		snapshot.SerializeBytes(entityRplId, 4);
		RplComponent entityRplComponent = RplComponent.Cast(Replication.FindItem(entityRplId));
		if (entityRplComponent)
			instance.m_Entity = entityRplComponent.GetEntity();
		else
			instance.m_Entity = null;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Encodes the slot snapshot into an EN Network packet.
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);

		RplId groupRplId;
		snapshot.SerializeBytes(groupRplId, 4);
		packet.Serialize(groupRplId, 32);

		RplId entityRplId;
		snapshot.SerializeBytes(entityRplId, 4);
		packet.Serialize(entityRplId, 32);
	}

	//------------------------------------------------------------------------------------------------
	//! Decodes an EN Network packet into a slot snapshot.
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);

		RplId groupRplId;
		packet.Serialize(groupRplId, 32);
		snapshot.SerializeBytes(groupRplId, 4);

		RplId entityRplId;
		packet.Serialize(entityRplId, 32);
		snapshot.SerializeBytes(entityRplId, 4);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares two EN Network snapshots.
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares this slot's properties against an EN Network snapshot.
	static bool PropCompare(GC_Slot instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		string uuid = instance.m_PlayerUUID;
		ResourceName prefab = instance.m_Prefab;
		RplId groupRplId = RplId.Invalid();
		if (instance.m_pGroup)
			groupRplId = Replication.FindItemId(instance.m_pGroup);
		RplId entityRplId = instance.GetEntityRplId();

		return snapshot.CompareInt(instance.m_iID)
			&& snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareString(uuid)
			&& snapshot.CompareString(instance.m_sCachedPlayerName)
			&& snapshot.CompareInt(instance.m_eState)
			&& snapshot.CompareString(prefab)
			&& snapshot.Compare(groupRplId, 4)
			&& snapshot.Compare(entityRplId, 4);
	}

}
