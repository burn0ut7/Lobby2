//! Complete authored/runtime marker payload replicated by GC_LobbyManager.
//! The owning world entity supplies the stable marker name, transform, and defaults.
[BaseContainerProps()]
class GC_BriefingMarker
{
	static const int MAX_FACTION_KEYS = 16;

	protected string m_sName;
	protected ResourceName m_sImage;
	protected string m_sQuadName;
	protected vector m_vWorldPosition;
	protected float m_fRotation;
	protected bool m_bWorldScale;
	protected float m_fSize;
	protected int m_iColor = Color.WHITE;
	protected string m_sDescription;
	protected ref array<string> m_aFactionKeys = {};
	protected SCR_EGameModeState m_eShowAtGameModeState;
	protected bool m_bInvertGameModeState;
	protected bool m_bVisible = true;
	protected int m_iRevision;

	string GetName() { return m_sName; }
	ResourceName GetImage() { return m_sImage; }
	string GetQuadName() { return m_sQuadName; }
	vector GetWorldPosition() { return m_vWorldPosition; }
	float GetRotation() { return m_fRotation; }
	bool IsWorldScale() { return m_bWorldScale; }
	float GetSize() { return m_fSize; }
	int GetColor() { return m_iColor; }
	string GetDescription() { return m_sDescription; }
	array<string> GetFactionKeys() { return m_aFactionKeys; }
	SCR_EGameModeState GetShowAtGameModeState() { return m_eShowAtGameModeState; }
	bool IsGameModeStateInverted() { return m_bInvertGameModeState; }
	bool IsVisible() { return m_bVisible; }
	int GetRevision() { return m_iRevision; }
	void SetRevision(int revision) { m_iRevision = revision; }

	void Set(string name, ResourceName image, string quadName, vector worldPosition, float rotation, bool worldScale,
		float size, string description, array<string> factionKeys = null,
		SCR_EGameModeState showAtGameModeState = SCR_EGameModeState.GAME,
		bool invertGameModeState = false, bool visible = true, int color = Color.WHITE)
	{
		m_sName = name;
		m_sImage = image;
		m_sQuadName = quadName;
		m_vWorldPosition = worldPosition;
		m_fRotation = rotation;
		m_bWorldScale = worldScale;
		m_fSize = size;
		m_iColor = color;
		m_sDescription = description;
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

	bool IsVisibleToFaction(Faction faction, SCR_EGameModeState gameModeState)
	{
		bool stateMatches = GC_LobbyPhase.IsAtOrAfter(gameModeState, m_eShowAtGameModeState);
		if (m_bInvertGameModeState)
			stateMatches = !stateMatches;

		if (!m_bVisible || !stateMatches)
			return false;

		if (m_aFactionKeys.IsEmpty())
			return true;

		FactionKey playerFactionKey;
		if (faction)
			playerFactionKey = faction.GetFactionKey();

		bool hasFactionRestriction;
		foreach (string allowedFactionKey : m_aFactionKeys)
		{
			if (allowedFactionKey.IsEmpty())
				continue;

			hasFactionRestriction = true;
			if (faction && allowedFactionKey == playerFactionKey)
				return true;
		}

		return !hasFactionRestriction;
	}

	void CopyFrom(GC_BriefingMarker source)
	{
		if (!source)
			return;

		Set(source.m_sName, source.m_sImage, source.m_sQuadName, source.m_vWorldPosition,
			source.m_fRotation, source.m_bWorldScale, source.m_fSize, source.m_sDescription,
			source.m_aFactionKeys, source.m_eShowAtGameModeState,
			source.m_bInvertGameModeState, source.m_bVisible, source.m_iColor);
		m_iRevision = source.m_iRevision;
	}

	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteString(m_sName);
		writer.WriteString(m_sImage);
		writer.WriteString(m_sQuadName);
		writer.WriteVector(m_vWorldPosition);
		writer.WriteFloat(m_fRotation);
		writer.WriteBool(m_bWorldScale);
		writer.WriteFloat(m_fSize);
		writer.WriteInt(m_iColor);
		writer.WriteString(m_sDescription);
		writer.WriteInt(m_aFactionKeys.Count());
		foreach (string factionKey : m_aFactionKeys)
			writer.WriteString(factionKey);

		writer.WriteInt(m_eShowAtGameModeState);
		writer.WriteBool(m_bInvertGameModeState);
		writer.WriteBool(m_bVisible);
		writer.WriteInt(m_iRevision);
		return true;
	}

	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadString(m_sName) || !reader.ReadString(m_sImage) || !reader.ReadString(m_sQuadName))
			return false;
		if (!reader.ReadVector(m_vWorldPosition) || !reader.ReadFloat(m_fRotation))
			return false;
		if (!reader.ReadBool(m_bWorldScale) || !reader.ReadFloat(m_fSize) || !reader.ReadInt(m_iColor)
			|| !reader.ReadString(m_sDescription))
			return false;

		int factionCount;
		if (!reader.ReadInt(factionCount) || factionCount < 0 || factionCount > MAX_FACTION_KEYS)
			return false;

		m_aFactionKeys.Clear();
		for (int factionIndex; factionIndex < factionCount; factionIndex++)
		{
			string factionKey;
			if (!reader.ReadString(factionKey))
				return false;
			m_aFactionKeys.Insert(factionKey);
		}

		int showAtGameModeState;
		if (!reader.ReadInt(showAtGameModeState) || !GC_LobbyPhase.IsValid(showAtGameModeState))
			return false;
		m_eShowAtGameModeState = showAtGameModeState;
		if (!reader.ReadBool(m_bInvertGameModeState) || !reader.ReadBool(m_bVisible) || !reader.ReadInt(m_iRevision))
			return false;

		return true;
	}

	static bool Extract(GC_BriefingMarker instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sImage);
		snapshot.SerializeString(instance.m_sQuadName);
		float positionX = instance.m_vWorldPosition[0];
		float positionY = instance.m_vWorldPosition[1];
		float positionZ = instance.m_vWorldPosition[2];
		snapshot.SerializeFloat(positionX);
		snapshot.SerializeFloat(positionY);
		snapshot.SerializeFloat(positionZ);
		snapshot.SerializeFloat(instance.m_fRotation);
		snapshot.SerializeBool(instance.m_bWorldScale);
		snapshot.SerializeFloat(instance.m_fSize);
		snapshot.SerializeInt(instance.m_iColor);
		snapshot.SerializeString(instance.m_sDescription);
		int factionCount = instance.m_aFactionKeys.Count();
		snapshot.SerializeInt(factionCount);
		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++)
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

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_BriefingMarker instance)
	{
		snapshot.SerializeString(instance.m_sName);
		snapshot.SerializeString(instance.m_sImage);
		snapshot.SerializeString(instance.m_sQuadName);
		float positionX;
		float positionY;
		float positionZ;
		snapshot.SerializeFloat(positionX);
		snapshot.SerializeFloat(positionY);
		snapshot.SerializeFloat(positionZ);
		instance.m_vWorldPosition = {positionX, positionY, positionZ};
		snapshot.SerializeFloat(instance.m_fRotation);
		snapshot.SerializeBool(instance.m_bWorldScale);
		snapshot.SerializeFloat(instance.m_fSize);
		snapshot.SerializeInt(instance.m_iColor);
		snapshot.SerializeString(instance.m_sDescription);
		int factionCount;
		snapshot.SerializeInt(factionCount);
		instance.m_aFactionKeys.Clear();
		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			string factionKey;
			snapshot.SerializeString(factionKey);
			if (factionIndex < factionCount)
				instance.m_aFactionKeys.Insert(factionKey);
		}
		int showAtGameModeState;
		snapshot.SerializeInt(showAtGameModeState);
		if (!GC_LobbyPhase.IsValid(showAtGameModeState))
			return false;
		instance.m_eShowAtGameModeState = showAtGameModeState;
		snapshot.SerializeBool(instance.m_bInvertGameModeState);
		snapshot.SerializeBool(instance.m_bVisible);
		snapshot.SerializeInt(instance.m_iRevision);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		for (int index; index < 4; index++) snapshot.EncodeFloat(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeFloat(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++) snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		for (int index; index < 4; index++) snapshot.DecodeFloat(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeFloat(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++) snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		if (!lhs.CompareStringSnapshots(rhs) || !lhs.CompareStringSnapshots(rhs) || !lhs.CompareStringSnapshots(rhs))
			return false;
		for (int index; index < 7; index++)
		{
			if (!lhs.CompareSnapshots(rhs, 4)) return false;
		}
		if (!lhs.CompareStringSnapshots(rhs) || !lhs.CompareSnapshots(rhs, 4))
			return false;
		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			if (!lhs.CompareStringSnapshots(rhs)) return false;
		}
		return lhs.CompareSnapshots(rhs, 4) && lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4) && lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(GC_BriefingMarker instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		if (!snapshot.CompareString(instance.m_sName) || !snapshot.CompareString(instance.m_sImage)
			|| !snapshot.CompareString(instance.m_sQuadName)
			|| !snapshot.CompareFloat(instance.m_vWorldPosition[0])
			|| !snapshot.CompareFloat(instance.m_vWorldPosition[1])
			|| !snapshot.CompareFloat(instance.m_vWorldPosition[2])
			|| !snapshot.CompareFloat(instance.m_fRotation)
			|| !snapshot.CompareBool(instance.m_bWorldScale)
			|| !snapshot.CompareFloat(instance.m_fSize)
			|| !snapshot.CompareInt(instance.m_iColor)
			|| !snapshot.CompareString(instance.m_sDescription)
			|| !snapshot.CompareInt(instance.m_aFactionKeys.Count()))
			return false;

		for (int factionIndex; factionIndex < MAX_FACTION_KEYS; factionIndex++)
		{
			string factionKey;
			if (factionIndex < instance.m_aFactionKeys.Count()) factionKey = instance.m_aFactionKeys[factionIndex];
			if (!snapshot.CompareString(factionKey)) return false;
		}

		return snapshot.CompareInt(instance.m_eShowAtGameModeState)
			&& snapshot.CompareBool(instance.m_bInvertGameModeState)
			&& snapshot.CompareBool(instance.m_bVisible)
			&& snapshot.CompareInt(instance.m_iRevision);
	}
}
