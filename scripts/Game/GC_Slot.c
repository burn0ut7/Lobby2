enum GC_SlotState
{
    FREE,
    LOCKED,
    KIA,
    DISCONNECTED,
    RESERVED
}

//! Controls how a slot's server-resolved random character variant is reused by future respawns.
enum GC_SlotVariantPolicy
{
	KEEP_VARIANT = 0,
	REROLL_ON_RESPAWN = 1
}

class GC_Slot
{
	//! Fired whenever lobby-owned slot state changes after the slot is registered.
	ref ScriptInvoker<GC_Slot> m_OnChanged = new ScriptInvoker<GC_Slot>();

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

	//! Character or spectator prefab authored on the group slot.
	protected ResourceName m_Prefab;

	//! Concrete character prefab selected once by the server for this slot lifetime.
	protected ResourceName m_ResolvedPrefab;

	//! Per-unit policy reserved for the future respawn system.
	protected GC_SlotVariantPolicy m_eVariantPolicy = GC_SlotVariantPolicy.KEEP_VARIANT;

	//! Runtime group assigned to this slot, if any.
	protected SCR_AIGroup m_pGroup;

	//! Replicated identity retained while the corresponding JIP group proxy streams in.
	protected RplId m_GroupRplId = RplId.Invalid();

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

	GC_SlotState GetState()
	{
		return m_eState;
	}

	void SetState(GC_SlotState state)
	{
		if (m_eState == state)
			return;

		m_eState = state;
		NotifyChanged();
	}

	ResourceName GetAuthoredPrefab()
	{
		return m_Prefab;
	}

	void SetAuthoredPrefab(ResourceName prefab)
	{
		if (m_Prefab == prefab)
			return;

		m_Prefab = prefab;
		NotifyChanged();
	}

	//! Compatibility alias. Slot role metadata should treat this as the authored prefab.
	ResourceName GetPrefab()
	{
		return GetAuthoredPrefab();
	}

	//! Compatibility alias. New registration code should use SetAuthoredPrefab explicitly.
	void SetPrefab(ResourceName prefab)
	{
		SetAuthoredPrefab(prefab);
	}

	ResourceName GetResolvedPrefab()
	{
		return m_ResolvedPrefab;
	}

	void SetResolvedPrefab(ResourceName prefab)
	{
		if (m_ResolvedPrefab == prefab)
			return;

		m_ResolvedPrefab = prefab;
		NotifyChanged();
	}

	GC_SlotVariantPolicy GetVariantPolicy()
	{
		return m_eVariantPolicy;
	}

	void SetVariantPolicy(GC_SlotVariantPolicy policy)
	{
		if (m_eVariantPolicy == policy)
			return;

		m_eVariantPolicy = policy;
		NotifyChanged();
	}

	SCR_AIGroup GetGroup()
	{
		return m_pGroup;
	}

	void SetGroup(SCR_AIGroup group)
	{
		if (m_pGroup == group)
			return;

		m_pGroup = group;
		if (group)
			m_GroupRplId = Replication.FindItemId(group);
		else
			m_GroupRplId = RplId.Invalid();
		NotifyChanged();
	}

	RplId GetGroupRplId()
	{
		if (m_pGroup)
			m_GroupRplId = Replication.FindItemId(m_pGroup);

		return m_GroupRplId;
	}

	//! Resolves a group reference that was unavailable when JIP data first loaded.
	bool ResolveReplicatedGroup()
	{
		if (m_pGroup)
			return true;

		if (m_GroupRplId == RplId.Invalid())
			return false;

		m_pGroup = SCR_AIGroup.Cast(Replication.FindItem(m_GroupRplId));
		return m_pGroup != null;
	}

	IEntity GetEntity()
	{
		return m_Entity;
	}

	void SetEntity(IEntity entity)
	{
		if (m_Entity == entity)
			return;

		m_Entity = entity;
		NotifyChanged();
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
		int previousPlayerId = m_iPlayerId;
		UUID previousUUID = m_PlayerUUID;
		string previousPlayerName = m_sCachedPlayerName;

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

		if (previousPlayerId != m_iPlayerId || previousUUID != m_PlayerUUID || previousPlayerName != m_sCachedPlayerName)
			NotifyChanged();
	}

	protected void NotifyChanged()
	{
		m_OnChanged.Invoke(this);
	}

	//! Copies a complete server-originated slot state into this local slot and notifies local listeners.
	//! The lobby manager only rebroadcasts changes on the server authority, so proxy notification is safe.
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
		m_ResolvedPrefab = source.m_ResolvedPrefab;
		m_eVariantPolicy = source.m_eVariantPolicy;
		m_pGroup = source.m_pGroup;
		m_GroupRplId = source.m_GroupRplId;
		m_Entity = source.m_Entity;
		NotifyChanged();
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
		writer.WriteResourceName(m_ResolvedPrefab);
		writer.WriteIntRange(m_eVariantPolicy, GC_SlotVariantPolicy.KEEP_VARIANT, GC_SlotVariantPolicy.REROLL_ON_RESPAWN);

		writer.WriteRplId(GetGroupRplId());
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
		if (!reader.ReadResourceName(m_ResolvedPrefab))
			return false;
		if (!reader.ReadIntRange(m_eVariantPolicy, GC_SlotVariantPolicy.KEEP_VARIANT, GC_SlotVariantPolicy.REROLL_ON_RESPAWN))
			return false;

		if (!reader.ReadRplId(m_GroupRplId))
			return false;

		m_pGroup = null;
		bool groupResolved = ResolveReplicatedGroup();
		PrintFormat("GC | JIP slot load: slot=%1 groupRplId=%2 resolved=%3", m_iID, m_GroupRplId, groupResolved);

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
		ResourceName resolvedPrefab = instance.m_ResolvedPrefab;
		snapshot.SerializeString(resolvedPrefab);
		snapshot.SerializeInt(instance.m_eVariantPolicy);

		RplId groupRplId = instance.GetGroupRplId();
		RplId entityRplId = instance.GetEntityRplId();
		snapshot.SerializeBytes(groupRplId, 4);
		snapshot.SerializeBytes(entityRplId, 4);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Injects an snapshot into this slot.
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
		ResourceName resolvedPrefab;
		snapshot.SerializeString(resolvedPrefab);
		instance.m_ResolvedPrefab = resolvedPrefab;

		int variantPolicy;
		snapshot.SerializeInt(variantPolicy);
		instance.m_eVariantPolicy = variantPolicy;

		RplId groupRplId;
		snapshot.SerializeBytes(groupRplId, 4);
		instance.m_GroupRplId = groupRplId;
		instance.m_pGroup = null;
		instance.ResolveReplicatedGroup();

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
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);

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
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);

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
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares this slot's properties against an EN Network snapshot.
	static bool PropCompare(GC_Slot instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		string uuid = instance.m_PlayerUUID;
		ResourceName prefab = instance.m_Prefab;
		ResourceName resolvedPrefab = instance.m_ResolvedPrefab;
		RplId groupRplId = instance.GetGroupRplId();
		RplId entityRplId = instance.GetEntityRplId();

		return snapshot.CompareInt(instance.m_iID)
			&& snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareString(uuid)
			&& snapshot.CompareString(instance.m_sCachedPlayerName)
			&& snapshot.CompareInt(instance.m_eState)
			&& snapshot.CompareString(prefab)
			&& snapshot.CompareString(resolvedPrefab)
			&& snapshot.CompareInt(instance.m_eVariantPolicy)
			&& snapshot.Compare(groupRplId, 4)
			&& snapshot.Compare(entityRplId, 4);
	}

}
