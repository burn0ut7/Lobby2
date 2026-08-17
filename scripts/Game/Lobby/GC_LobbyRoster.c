//! Lobby2's derived index of base-game factions, groups, and lobby-only slots.
//! Group membership itself remains owned by SCR_AIGroup and the groups manager.
class GC_LobbyRoster
{
	protected ref array<SCR_AIGroup> m_aGroups = {};
	protected ref array<Faction> m_aFactions = {};
	protected ref array<ref GC_Slot> m_aSlots = {};
	protected ref array<RplId> m_aPendingGroupIds = {};
	protected int m_iNextSlotId = 1;
	protected int m_iJipResolveAttempt;

	bool AddGroup(SCR_AIGroup group, out bool wasEmpty)
	{
		wasEmpty = m_aGroups.IsEmpty();
		if (!group || m_aGroups.Contains(group))
			return false;

		m_aGroups.Insert(group);
		return true;
	}

	bool RemoveGroup(SCR_AIGroup group)
	{
		int index = m_aGroups.Find(group);
		if (index < 0)
			return false;

		m_aGroups.RemoveOrdered(index);
		return true;
	}

	bool ContainsGroup(SCR_AIGroup group) { return m_aGroups.Contains(group); }
	bool HasGroups() { return !m_aGroups.IsEmpty(); }
	array<SCR_AIGroup> GetGroups() { return m_aGroups; }

	bool AddFaction(Faction faction)
	{
		if (!faction || m_aFactions.Contains(faction))
			return false;

		m_aFactions.Insert(faction);
		return true;
	}

	bool RemoveFaction(Faction faction)
	{
		int index = m_aFactions.Find(faction);
		if (index < 0)
			return false;

		m_aFactions.RemoveOrdered(index);
		return true;
	}

	bool ContainsFaction(Faction faction) { return m_aFactions.Contains(faction); }
	array<Faction> GetFactions() { return m_aFactions; }

	Faction GetFaction(FactionKey factionKey)
	{
		foreach (Faction faction : m_aFactions)
		{
			if (faction && faction.GetFactionKey() == factionKey)
				return faction;
		}

		return null;
	}

	int AllocateSlotId() { return m_iNextSlotId++; }
	void AddSlot(GC_Slot slot) { if (slot && !m_aSlots.Contains(slot)) m_aSlots.Insert(slot); }
	bool RemoveSlot(GC_Slot slot)
	{
		int index = m_aSlots.Find(slot);
		if (index < 0)
			return false;

		m_aSlots.RemoveOrdered(index);
		return true;
	}
	bool ContainsSlot(GC_Slot slot) { return m_aSlots.Contains(slot); }
	array<ref GC_Slot> GetSlots() { return m_aSlots; }

	GC_Slot GetSlot(int id)
	{
		foreach (GC_Slot slot : m_aSlots)
		{
			if (slot && slot.GetID() == id)
				return slot;
		}

		return null;
	}

	GC_Slot GetSlotByPlayerId(int playerId)
	{
		if (playerId < 0)
			return null;

		foreach (GC_Slot slot : m_aSlots)
		{
			if (slot && slot.GetPlayerId() == playerId)
				return slot;
		}

		return null;
	}

	array<SCR_AIGroup> GetGroupsForFaction(FactionKey factionKey)
	{
		array<SCR_AIGroup> groups = {};
		foreach (SCR_AIGroup group : m_aGroups)
		{
			if (group && group.GetFaction() && group.GetFaction().GetFactionKey() == factionKey)
				groups.Insert(group);
		}

		return groups;
	}

	array<ref GC_Slot> GetSlotsForGroup(SCR_AIGroup group)
	{
		array<ref GC_Slot> slots = {};
		if (!group)
			return slots;

		foreach (GC_Slot slot : m_aSlots)
		{
			if (slot && slot.GetGroup() == group)
				slots.Insert(slot);
		}

		return slots;
	}

	SCR_AIGroup GetGroupByRplId(RplId groupRplId)
	{
		if (groupRplId == RplId.Invalid())
			return null;

		foreach (SCR_AIGroup group : m_aGroups)
		{
			if (group && Replication.FindItemId(group) == groupRplId)
				return group;
		}

		return null;
	}

	SCR_AIGroup GetPlayerGroup(int playerId)
	{
		foreach (SCR_AIGroup group : m_aGroups)
		{
			if (group && group.GetPlayerIDs().Contains(playerId))
				return group;
		}

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (groupsManager)
			return groupsManager.GetPlayerGroup(playerId);

		return null;
	}

	Faction GetPlayerFaction(int playerId)
	{
		SCR_AIGroup group = GetPlayerGroup(playerId);
		if (group && group.GetFaction())
			return group.GetFaction();

		GC_Slot slot = GetSlotByPlayerId(playerId);
		if (slot && slot.GetGroup())
			return slot.GetGroup().GetFaction();

		return null;
	}

	int GetGroupPlayerCount(SCR_AIGroup group)
	{
		array<int> playerIds = {};
		foreach (GC_Slot slot : GetSlotsForGroup(group))
		{
			if (slot && slot.GetPlayerId() > 0 && !playerIds.Contains(slot.GetPlayerId()))
				playerIds.Insert(slot.GetPlayerId());
		}
		return playerIds.Count();
	}

	int GetFactionPlayerCount(FactionKey factionKey)
	{
		array<int> playerIds = {};
		foreach (SCR_AIGroup group : GetGroupsForFaction(factionKey))
		{
			foreach (GC_Slot slot : GetSlotsForGroup(group))
			{
				if (slot && slot.GetPlayerId() > 0 && !playerIds.Contains(slot.GetPlayerId()))
					playerIds.Insert(slot.GetPlayerId());
			}
		}
		return playerIds.Count();
	}

	int GetFactionSlotCount(FactionKey factionKey)
	{
		int count;
		foreach (SCR_AIGroup group : GetGroupsForFaction(factionKey))
			count += GetSlotsForGroup(group).Count();
		return count;
	}

	array<RplId> GetPendingGroupIds() { return m_aPendingGroupIds; }
	void ResetJipAttempts() { m_iJipResolveAttempt = 0; }
	int IncrementJipAttempt() { return ++m_iJipResolveAttempt; }
	int GetJipAttempt() { return m_iJipResolveAttempt; }

	void Clear()
	{
		m_aGroups.Clear();
		m_aFactions.Clear();
		m_aSlots.Clear();
		m_aPendingGroupIds.Clear();
		m_iJipResolveAttempt = 0;
	}
}
