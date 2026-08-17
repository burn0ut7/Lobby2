//! Authored briefing definition and complete replicated runtime snapshot.
//! Runtime changes must go through GC_LobbyManager so the authority can
//! broadcast them and include the latest value in future JIP snapshots.
[BaseContainerProps()]
class GC_Briefing
{
	static const int MAX_FACTION_KEYS = 16;

	[Attribute("0", UIWidgets.SpinBox, "One-based briefing index used for display, updates, and selection")]
	protected int m_iOrder;

	[Attribute("BRIEFING", UIWidgets.EditBox, "Player-facing briefing title")]
	protected string m_sTitle;

	[Attribute("", UIWidgets.EditBoxMultiline, "Plain or Reforger rich-text briefing body")]
	protected string m_sContent;

	[Attribute("", UIWidgets.EditBox, "Authoritative base-game faction keys. Empty shows the briefing to everyone")]
	protected ref array<string> m_aFactionKeys;

	[Attribute("0", UIWidgets.ComboBox, "Lobby2 phase in which this briefing is shown", enumType: GC_ELobbyPhase)]
	protected GC_ELobbyPhase m_eShowAtGameModeState;

	[Attribute("0", UIWidgets.CheckBox, "Invert the selected state rule. Off: show only during the selected state. On: hide during the selected state and show during all other states")]
	protected bool m_bInvertGameModeState;

	//! Runtime-only visibility gate. Mission scripts mutate this through
	//! GC_LobbyManager.SetBriefingVisible; it is not exposed to prefab authors.
	protected bool m_bVisible = true;

	//! Authority-assigned monotonic version used to reject stale runtime updates.
	protected int m_iRevision;

	int GetOrder()
	{
		return m_iOrder;
	}

	string GetTitle()
	{
		return m_sTitle;
	}

	string GetContent()
	{
		return m_sContent;
	}

	array<string> GetFactionKeys()
	{
		if (!m_aFactionKeys)
			m_aFactionKeys = {};

		return m_aFactionKeys;
	}

	GC_ELobbyPhase GetShowAtGameModeState()
	{
		return m_eShowAtGameModeState;
	}

	bool IsGameModeStateInverted()
	{
		return m_bInvertGameModeState;
	}

	bool IsVisible()
	{
		return m_bVisible;
	}

	int GetRevision()
	{
		return m_iRevision;
	}

	void SetRevision(int revision)
	{
		m_iRevision = revision;
	}

	//! Creates or replaces the editable payload before it is submitted to the authority.
	void Set(int order, string title, string content, array<string> factionKeys = null, GC_ELobbyPhase showAtGameModeState = GC_ELobbyPhase.GAME, bool invertGameModeState = false, bool visible = true)
	{
		m_iOrder = order;
		m_sTitle = title;
		m_sContent = content;
		if (!m_aFactionKeys)
			m_aFactionKeys = {};

		m_aFactionKeys.Clear();
		if (factionKeys)
		{
			foreach (string factionKey : factionKeys)
				m_aFactionKeys.Insert(factionKey);
		}

		m_eShowAtGameModeState = showAtGameModeState;
		m_bInvertGameModeState = invertGameModeState;
		m_bVisible = visible;
	}

	bool IsVisibleToFaction(Faction faction, GC_ELobbyPhase gameModeState)
	{
		bool stateMatches = gameModeState == m_eShowAtGameModeState;
		if (m_bInvertGameModeState)
			stateMatches = !stateMatches;

		if (!m_bVisible || !stateMatches)
			return false;

		if (!m_aFactionKeys || m_aFactionKeys.Count() == 0)
			return true;

		return faction && m_aFactionKeys.Contains(faction.GetFactionKey());
	}

	void CopyFrom(GC_Briefing source)
	{
		if (!source)
			return;

		m_iOrder = source.m_iOrder;
		m_sTitle = source.m_sTitle;
		m_sContent = source.m_sContent;
		if (!m_aFactionKeys)
			m_aFactionKeys = {};

		m_aFactionKeys.Clear();
		if (source.m_aFactionKeys)
		{
			foreach (string factionKey : source.m_aFactionKeys)
				m_aFactionKeys.Insert(factionKey);
		}

		m_eShowAtGameModeState = source.m_eShowAtGameModeState;
		m_bInvertGameModeState = source.m_bInvertGameModeState;
		m_bVisible = source.m_bVisible;
		m_iRevision = source.m_iRevision;
	}

	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iOrder);
		writer.WriteString(m_sTitle);
		writer.WriteString(m_sContent);
		int factionCount;
		if (m_aFactionKeys)
			factionCount = m_aFactionKeys.Count();

		writer.WriteInt(factionCount);
		if (m_aFactionKeys)
		{
			foreach (string factionKey : m_aFactionKeys)
				writer.WriteString(factionKey);
		}

		writer.WriteIntRange(m_eShowAtGameModeState, GC_ELobbyPhase.PREGAME, GC_ELobbyPhase.POSTGAME);
		writer.WriteBool(m_bInvertGameModeState);
		writer.WriteBool(m_bVisible);
		writer.WriteInt(m_iRevision);
		return true;
	}

	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadInt(m_iOrder))
			return false;
		if (!reader.ReadString(m_sTitle))
			return false;
		if (!reader.ReadString(m_sContent))
			return false;
		int factionCount;
		if (!reader.ReadInt(factionCount) || factionCount < 0 || factionCount > MAX_FACTION_KEYS)
			return false;

		if (!m_aFactionKeys)
			m_aFactionKeys = {};
		else
			m_aFactionKeys.Clear();
		for (int factionIndex = 0; factionIndex < factionCount; factionIndex++)
		{
			string factionKey;
			if (!reader.ReadString(factionKey))
				return false;

			m_aFactionKeys.Insert(factionKey);
		}

		if (!reader.ReadIntRange(m_eShowAtGameModeState, GC_ELobbyPhase.PREGAME, GC_ELobbyPhase.POSTGAME))
			return false;
		if (!reader.ReadBool(m_bInvertGameModeState))
			return false;
		if (!reader.ReadBool(m_bVisible))
			return false;
		if (!reader.ReadInt(m_iRevision))
			return false;

		return true;
	}

	static bool Extract(GC_Briefing instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iOrder);
		snapshot.SerializeString(instance.m_sTitle);
		snapshot.SerializeString(instance.m_sContent);
		int factionCount;
		if (instance.m_aFactionKeys)
			factionCount = instance.m_aFactionKeys.Count();
		snapshot.SerializeInt(factionCount);
		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			string factionKey;
			if (factionIndex < factionCount)
				factionKey = instance.m_aFactionKeys[factionIndex];

			snapshot.SerializeString(factionKey);
		}

		snapshot.SerializeInt(instance.m_eShowAtGameModeState);
		snapshot.SerializeBool(instance.m_bInvertGameModeState);
		snapshot.SerializeBool(instance.m_bVisible);
		snapshot.SerializeInt(instance.m_iRevision);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_Briefing instance)
	{
		snapshot.SerializeInt(instance.m_iOrder);
		snapshot.SerializeString(instance.m_sTitle);
		snapshot.SerializeString(instance.m_sContent);

		int factionCount;
		snapshot.SerializeInt(factionCount);
		if (!instance.m_aFactionKeys)
			instance.m_aFactionKeys = {};
		else
			instance.m_aFactionKeys.Clear();
		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			string factionKey;
			snapshot.SerializeString(factionKey);
			if (factionIndex < factionCount)
				instance.m_aFactionKeys.Insert(factionKey);
		}

		int showAtGameModeState;
		snapshot.SerializeInt(showAtGameModeState);
		instance.m_eShowAtGameModeState = showAtGameModeState;

		snapshot.SerializeBool(instance.m_bInvertGameModeState);
		snapshot.SerializeBool(instance.m_bVisible);
		snapshot.SerializeInt(instance.m_iRevision);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
			snapshot.EncodeString(packet);

		snapshot.EncodeInt(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
			snapshot.DecodeString(packet);

		snapshot.DecodeInt(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		if (!(lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)))
			return false;

		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			if (!lhs.CompareStringSnapshots(rhs))
				return false;
		}

		return lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(GC_Briefing instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		int factionCount;
		if (instance.m_aFactionKeys)
			factionCount = instance.m_aFactionKeys.Count();
		if (!(snapshot.CompareInt(instance.m_iOrder)
			&& snapshot.CompareString(instance.m_sTitle)
			&& snapshot.CompareString(instance.m_sContent)
			&& snapshot.CompareInt(factionCount)))
			return false;

		for (int factionIndex = 0; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			string factionKey;
			if (factionIndex < factionCount)
				factionKey = instance.m_aFactionKeys[factionIndex];

			if (!snapshot.CompareString(factionKey))
				return false;
		}

		return snapshot.CompareInt(instance.m_eShowAtGameModeState)
			&& snapshot.CompareBool(instance.m_bInvertGameModeState)
			&& snapshot.CompareBool(instance.m_bVisible)
			&& snapshot.CompareInt(instance.m_iRevision);
	}
}
