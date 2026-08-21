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

	//! Stable index into the owning group's authored unit-prefab array.
	protected int m_iGroupSlotIndex = -1;

	//! Per-unit policy reserved for the future respawn system.
	protected GC_SlotVariantPolicy m_eVariantPolicy = GC_SlotVariantPolicy.KEEP_VARIANT;

	//! Runtime group assigned to this slot, if any.
	protected SCR_AIGroup m_pGroup;

	//! Replicated identity retained while the corresponding JIP group proxy streams in.
	protected RplId m_GroupRplId = RplId.Invalid();

	//! Entity spawned for and currently represented by this slot.
	protected IEntity m_Entity;

	//! Replicated entity reference retained while the proxy streams in.
	protected RplId m_EntityRplId = RplId.Invalid();

	int GetID()
	{
		return m_iID;
	}

	void SetID(int id)
	{
		m_iID = id;
	}

	int GetGroupSlotIndex()
	{
		return m_iGroupSlotIndex;
	}

	void SetGroupSlotIndex(int index)
	{
		if (m_iGroupSlotIndex == index)
			return;

		m_iGroupSlotIndex = index;
		NotifyChanged();
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

	bool HasClaim()
	{
		return !m_PlayerUUID.IsEmpty();
	}

	bool IsKIA()
	{
		return m_eState == GC_SlotState.KIA;
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
		// The RplId is the stable association for this slot. Do not refresh it
		// from the live pointer here: during phase transitions a transient group
		// reference can be present while the authored group is still the slot's
		// authoritative owner. Recomputing here would overwrite the association
		// during replication/save and make queued spawns use the wrong origin.
		if (m_GroupRplId == RplId.Invalid() && m_pGroup)
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
		if (entity)
		{
			RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(entity);
			if (rplComponent)
				m_EntityRplId = rplComponent.Id();
		}
		NotifyChanged();
	}

	//! Clears the live entity reference while optionally preserving its RplId for diagnostics.
	void ClearEntityReference(bool preserveRplId = true)
	{
		m_Entity = null;
		if (!preserveRplId)
			m_EntityRplId = RplId.Invalid();
		NotifyChanged();
	}

	//! Returns the RPL ID of the slot entity's replication component.
	RplId GetEntityRplId()
	{
		if (!m_Entity)
			return m_EntityRplId;

		RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(m_Entity);
		if (!rplComponent)
			return m_EntityRplId;

		m_EntityRplId = rplComponent.Id();
		return m_EntityRplId;
	}

	//! Resolves a slot entity that was unavailable during the first JIP pass.
	bool ResolveReplicatedEntity()
	{
		if (m_Entity)
			return true;

		if (m_EntityRplId == RplId.Invalid())
			return false;

		RplComponent entityRplComponent = RplComponent.Cast(Replication.FindItem(m_EntityRplId));
		if (!entityRplComponent)
			return false;

		m_Entity = entityRplComponent.GetEntity();
		return m_Entity != null;
	}

	//! Assigns a connected player and persists the identity used for reconnects.
	void SetPlayer(int playerId)
	{
		int previousPlayerId = m_iPlayerId;
		UUID previousUUID = m_PlayerUUID;
		string previousPlayerName = m_sCachedPlayerName;

		if (playerId < 0)
		{
			m_iPlayerId = -1;
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

	//! Clears only the live session identifier while preserving the reservation identity.
	void MarkDisconnected()
	{
		if (m_eState != GC_SlotState.KIA)
			m_eState = GC_SlotState.DISCONNECTED;

		m_iPlayerId = -1;
		NotifyChanged();
	}

	//! Rebinds a reconnecting player and restores the state implied by the entity.
	void ReconnectPlayer(int playerId)
	{
		if (playerId < 0)
			return;

		m_iPlayerId = playerId;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (!identity.IsEmpty())
			m_PlayerUUID = identity;

		m_sCachedPlayerName = GetGame().GetPlayerManager().GetPlayerNameByIdentity(m_PlayerUUID);
		if (!m_sCachedPlayerName)
			m_sCachedPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);

		if (m_eState == GC_SlotState.DISCONNECTED)
		{
			if (m_Entity)
				m_eState = GC_SlotState.LOCKED;
			else
				m_eState = GC_SlotState.RESERVED;
		}

		NotifyChanged();
	}

	//! Releases a reservation while retaining KIA state and the dead entity reference.
	void ReleaseClaim()
	{
		m_iPlayerId = -1;
		m_PlayerUUID = "";
		m_sCachedPlayerName = "";
		if (m_eState != GC_SlotState.KIA)
			m_eState = GC_SlotState.FREE;
		NotifyChanged();
	}

	//! Marks the represented character as dead for the current match.
	void MarkKIA()
	{
		m_eState = GC_SlotState.KIA;
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
		m_iGroupSlotIndex = source.m_iGroupSlotIndex;
		m_eVariantPolicy = source.m_eVariantPolicy;
		m_pGroup = source.m_pGroup;
		m_GroupRplId = source.m_GroupRplId;
		m_Entity = source.m_Entity;
		m_EntityRplId = source.m_EntityRplId;
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
		writer.WriteInt(m_iGroupSlotIndex);
		writer.WriteIntRange(m_eVariantPolicy, GC_SlotVariantPolicy.KEEP_VARIANT, GC_SlotVariantPolicy.REROLL_ON_RESPAWN);

		writer.WriteRplId(GetGroupRplId());
		writer.WriteRplId(GetEntityRplId());
		return true;
	}

	//! Deserializes this slot, including its serialized ID.
	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iID))
		{
			Print("GC | JIP load slot failed: field=id", level: LogLevel.ERROR);
			return false;
		}

		return RplLoadData(reader);
	}

	//! Deserializes all slot data after the constructor has received the ID.
	bool RplLoadData(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iPlayerId))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=playerId", m_iID, level: LogLevel.ERROR);
			return false;
		}

		string uuid;
		if (!reader.ReadString(uuid))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=uuid", m_iID, level: LogLevel.ERROR);
			return false;
		}
		m_PlayerUUID = uuid;

		if (!reader.ReadString(m_sCachedPlayerName))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=cachedPlayerName", m_iID, level: LogLevel.ERROR);
			return false;
		}
		if (!reader.ReadIntRange(m_eState, GC_SlotState.FREE, GC_SlotState.RESERVED))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=state", m_iID, level: LogLevel.ERROR);
			return false;
		}
		if (!reader.ReadResourceName(m_Prefab))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=authoredPrefab", m_iID, level: LogLevel.ERROR);
			return false;
		}
		if (!reader.ReadResourceName(m_ResolvedPrefab))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=resolvedPrefab", m_iID, level: LogLevel.ERROR);
			return false;
		}
		if (!reader.ReadInt(m_iGroupSlotIndex))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=groupSlotIndex", m_iID, level: LogLevel.ERROR);
			return false;
		}
		if (!reader.ReadIntRange(m_eVariantPolicy, GC_SlotVariantPolicy.KEEP_VARIANT, GC_SlotVariantPolicy.REROLL_ON_RESPAWN))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=variantPolicy", m_iID, level: LogLevel.ERROR);
			return false;
		}

		if (!reader.ReadRplId(m_GroupRplId))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=groupRplId", m_iID, level: LogLevel.ERROR);
			return false;
		}

		m_pGroup = null;
		ResolveReplicatedGroup();

		if (!reader.ReadRplId(m_EntityRplId))
		{
			PrintFormat("GC | JIP load slot failed: slot=%1 field=entityRplId", m_iID, level: LogLevel.ERROR);
			return false;
		}

		m_Entity = null;
		ResolveReplicatedEntity();

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
		snapshot.SerializeInt(instance.m_iGroupSlotIndex);
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
		snapshot.SerializeInt(instance.m_iGroupSlotIndex);

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
		instance.m_EntityRplId = entityRplId;
		instance.m_Entity = null;
		instance.ResolveReplicatedEntity();

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
		int groupSlotIndex = instance.m_iGroupSlotIndex;
		RplId groupRplId = instance.GetGroupRplId();
		RplId entityRplId = instance.GetEntityRplId();

		return snapshot.CompareInt(instance.m_iID)
			&& snapshot.CompareInt(instance.m_iPlayerId)
			&& snapshot.CompareString(uuid)
			&& snapshot.CompareString(instance.m_sCachedPlayerName)
			&& snapshot.CompareInt(instance.m_eState)
			&& snapshot.CompareString(prefab)
			&& snapshot.CompareString(resolvedPrefab)
			&& snapshot.CompareInt(groupSlotIndex)
			&& snapshot.CompareInt(instance.m_eVariantPolicy)
			&& snapshot.Compare(groupRplId, 4)
			&& snapshot.Compare(entityRplId, 4);
	}

}
