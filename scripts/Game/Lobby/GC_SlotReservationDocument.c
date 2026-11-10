//! A proposed import row, not an accepted claim. The manager validates all live
//! claims synchronously before applying any of these values.
class GC_SlotReservationClaim
{
	GC_Slot m_Slot;
	string m_sIdentity;
	string m_sPlayerName;
}

//! Versioned roster documents and stable authored keys. This object never changes
//! slots, assigns players, schedules catch-up or decides administration policy.
class GC_SlotReservationDocument
{
	protected string m_sWorld;
	protected ref map<string, GC_Slot> m_mSlots = new map<string, GC_Slot>();
	protected ref array<ref map<string, string>> m_aRows = {};
	protected int m_iExcluded;
	protected bool m_bAmbiguous;

	//------------------------------------------------------------------------------------------------
	void GC_SlotReservationDocument(string world)
	{
		m_sWorld = world;
	}

	//------------------------------------------------------------------------------------------------
	//! Use the same immutable authored anchor as spawning, never the current group origin.
	void AddSlot(GC_Slot slot, SCR_AIGroup group, GC_PlayerGroupSpawnTransform transform)
	{
		if (!group || !group.GetFaction() || !group.GetPrefabData() || slot.GetGroupSlotIndex() < 0
			|| slot.GetAdminSpawnPoint() || !transform)
		{
			m_iExcluded++;
			return;
		}
		string source = string.Format("%1|%2|%3|%4|%5", m_sWorld,
			group.GetPrefabData().GetPrefabName(), transform.GetOrigin(), transform.GetAngles(), group.GetFaction().GetFactionKey());
		string groupKey = UUID.GenV8(UUID.NAMESPACE_URL, source);
		string key = UUID.GenV8(UUID.NAMESPACE_URL, string.Format("%1|%2|%3", groupKey, slot.GetGroupSlotIndex(), slot.GetAuthoredPrefab()));
		if (m_mSlots.Contains(key))
		{
			m_bAmbiguous = true;
			return;
		}
		m_mSlots.Set(key, slot);
		map<string, string> row = new map<string, string>();
		row.Set("slotKey", key);
		row.Set("slotNumber", slot.GetID().ToString());
		row.Set("teamKey", group.GetFaction().GetFactionKey());
		row.Set("teamName", WidgetManager.Translate(group.GetFaction().GetFactionName()));
		row.Set("groupKey", groupKey);
		row.Set("groupName", GC_GroupCallsignHelper.GetDisplayName(group));
		row.Set("groupPrefab", group.GetPrefabData().GetPrefabName());
		row.Set("groupPosition", transform.GetOrigin().ToString());
		row.Set("unitIndex", slot.GetGroupSlotIndex().ToString());
		row.Set("rolePrefab", slot.GetAuthoredPrefab());
		SCR_UIInfo role = GC_Slot.GetPrefabRoleInfo(slot.GetAuthoredPrefab());
		if (role)
		{
			row.Set("roleNameKey", role.GetName());
			row.Set("roleName", WidgetManager.Translate(role.GetName()));
		}
		else
			row.Set("roleName", "Unit " + (slot.GetGroupSlotIndex() + 1).ToString());
		row.Set("playerName", slot.GetCachedPlayerName());
		row.Set("armaUid", slot.GetUUID());
		m_aRows.Insert(row);
	}

	//------------------------------------------------------------------------------------------------
	string Export(out string output)
	{
		output = "";
		if (m_bAmbiguous)
			return "Export rejected: authored slots have ambiguous locations and roles.";
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("version", 1) || !context.WriteValue("world", m_sWorld) || !context.WriteValue("slots", m_aRows))
			return "The roster could not be serialized.";
		output = context.SaveToString();
		if (output.Length() > 32768 || m_aRows.Count() > 128)
		{
			output = "";
			return "This roster exceeds the transfer limit of 128 slots / 32 KiB.";
		}
		return string.Format("Exported %1 authored slots. %2 runtime-only slots excluded. Copy the text to save it.", m_aRows.Count(), m_iExcluded);
	}

	//------------------------------------------------------------------------------------------------
	//! Blank UIDs are no-ops. Descriptive metadata never selects or changes a live slot.
	string Parse(string input, out array<ref GC_SlotReservationClaim> claims)
	{
		claims = {};
		if (input.IsEmpty() || input.Length() > 32768)
			return "Paste a reservation export of 32 KiB or less.";
		JsonLoadContext context = new JsonLoadContext(false);
		int version;
		string world;
		array<ref map<string, string>> rows;
		if (!context.LoadFromString(input) || !context.ReadValue("version", version) || version != 1
			|| !context.ReadValue("world", world) || !context.ReadValue("slots", rows) || !rows)
			return "Invalid reservation document. Use a version 1 Lobby2 export.";
		if (world != m_sWorld)
			return "This export belongs to a different scenario.";
		if (rows.Count() > 128)
			return "Import supports at most 128 slots.";
		if (m_bAmbiguous)
			return "Import rejected: authored slot locations are ambiguous.";
		array<string> seenKeys = {};
		array<string> seenIdentities = {};
		foreach (map<string, string> row : rows)
		{
			string key;
			string value;
			if (!row || !row.Find("slotKey", key) || !row.Find("armaUid", value))
				return "Each slot needs slotKey and armaUid fields. No reservations were changed.";
			GC_Slot slot = m_mSlots.Get(key);
			if (!slot || seenKeys.Contains(key))
				return "A slot is missing, changed, or repeated. Export the current scenario again.";
			seenKeys.Insert(key);
			value.TrimInPlace();
			value.ToLower();
			if (value.IsEmpty())
				continue;
			if (seenIdentities.Contains(value))
				return "The same Arma UID appears more than once. No reservations were changed.";
			seenIdentities.Insert(value);
			string playerName = row.Get("playerName");
			playerName.TrimInPlace();
			if (playerName.Length() > 128 || playerName.Contains("\n") || playerName.Contains("\r"))
				return "Player names must be 128 characters or fewer on one line. No reservations were changed.";
			GC_SlotReservationClaim claim = new GC_SlotReservationClaim();
			claim.m_Slot = slot;
			claim.m_sIdentity = value;
			claim.m_sPlayerName = playerName;
			claims.Insert(claim);
		}
		return "";
	}
}
