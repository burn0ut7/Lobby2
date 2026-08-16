enum GC_BriefingTextFormat
{
	PLAIN = 0,
	RICH = 1
}

enum GC_BriefingAudience
{
	ALL_FACTIONS = 0,
	SPECIFIC_FACTION = 1,
	NO_FACTION = 2
}

//! Authored briefing definition and complete replicated runtime snapshot.
//! Runtime changes must go through GC_LobbyManager so the authority can
//! broadcast them and include the latest value in future JIP snapshots.
[BaseContainerProps()]
class GC_Briefing
{
	[Attribute("", UIWidgets.EditBox, "Stable briefing identifier used for updates and selection")]
	protected string m_sId;

	[Attribute("0", UIWidgets.EditBox, "Display order in the briefing index")]
	protected int m_iOrder;

	[Attribute("BRIEFING", UIWidgets.EditBox, "Player-facing briefing title")]
	protected string m_sTitle;

	[Attribute("", UIWidgets.EditBox, "Plain or Reforger rich-text briefing body")]
	protected string m_sContent;

	[Attribute("0", UIWidgets.ComboBox, "Whether the body is interpreted as plain or rich text", enumType: GC_BriefingTextFormat)]
	protected GC_BriefingTextFormat m_eTextFormat;

	[Attribute("0", UIWidgets.ComboBox, "Players allowed to see this briefing", enumType: GC_BriefingAudience)]
	protected GC_BriefingAudience m_eAudience;

	[Attribute("", UIWidgets.EditBox, "Authoritative base-game faction key used by Specific Faction audience")]
	protected string m_sFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Whether this briefing is currently shown to eligible players")]
	protected bool m_bVisible;

	//! Authority-assigned monotonic version used to reject stale runtime updates.
	protected int m_iRevision;

	string GetId()
	{
		return m_sId;
	}

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

	GC_BriefingTextFormat GetTextFormat()
	{
		return m_eTextFormat;
	}

	GC_BriefingAudience GetAudience()
	{
		return m_eAudience;
	}

	string GetFactionKey()
	{
		return m_sFactionKey;
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
	void Set(string id, int order, string title, string content, GC_BriefingTextFormat textFormat = GC_BriefingTextFormat.PLAIN, GC_BriefingAudience audience = GC_BriefingAudience.ALL_FACTIONS, string factionKey = "", bool visible = true)
	{
		m_sId = id;
		m_iOrder = order;
		m_sTitle = title;
		m_sContent = content;
		m_eTextFormat = textFormat;
		m_eAudience = audience;
		m_sFactionKey = factionKey;
		m_bVisible = visible;
	}

	bool IsVisibleToFaction(Faction faction)
	{
		if (!m_bVisible)
			return false;

		switch (m_eAudience)
		{
			case GC_BriefingAudience.SPECIFIC_FACTION:
				return faction && faction.GetFactionKey() == m_sFactionKey;

			case GC_BriefingAudience.NO_FACTION:
				return !faction;
		}

		return true;
	}

	void CopyFrom(GC_Briefing source)
	{
		if (!source)
			return;

		m_sId = source.m_sId;
		m_iOrder = source.m_iOrder;
		m_sTitle = source.m_sTitle;
		m_sContent = source.m_sContent;
		m_eTextFormat = source.m_eTextFormat;
		m_eAudience = source.m_eAudience;
		m_sFactionKey = source.m_sFactionKey;
		m_bVisible = source.m_bVisible;
		m_iRevision = source.m_iRevision;
	}

	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteString(m_sId);
		writer.WriteInt(m_iOrder);
		writer.WriteString(m_sTitle);
		writer.WriteString(m_sContent);
		writer.WriteIntRange(m_eTextFormat, GC_BriefingTextFormat.PLAIN, GC_BriefingTextFormat.RICH);
		writer.WriteIntRange(m_eAudience, GC_BriefingAudience.ALL_FACTIONS, GC_BriefingAudience.NO_FACTION);
		writer.WriteString(m_sFactionKey);
		writer.WriteBool(m_bVisible);
		writer.WriteInt(m_iRevision);
		return true;
	}

	bool RplLoad(ScriptBitReader reader)
	{
		if (!reader.ReadString(m_sId))
			return false;
		if (!reader.ReadInt(m_iOrder))
			return false;
		if (!reader.ReadString(m_sTitle))
			return false;
		if (!reader.ReadString(m_sContent))
			return false;
		if (!reader.ReadIntRange(m_eTextFormat, GC_BriefingTextFormat.PLAIN, GC_BriefingTextFormat.RICH))
			return false;
		if (!reader.ReadIntRange(m_eAudience, GC_BriefingAudience.ALL_FACTIONS, GC_BriefingAudience.NO_FACTION))
			return false;
		if (!reader.ReadString(m_sFactionKey))
			return false;
		if (!reader.ReadBool(m_bVisible))
			return false;
		if (!reader.ReadInt(m_iRevision))
			return false;

		return true;
	}

	static bool Extract(GC_Briefing instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeString(instance.m_sId);
		snapshot.SerializeInt(instance.m_iOrder);
		snapshot.SerializeString(instance.m_sTitle);
		snapshot.SerializeString(instance.m_sContent);
		snapshot.SerializeInt(instance.m_eTextFormat);
		snapshot.SerializeInt(instance.m_eAudience);
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeBool(instance.m_bVisible);
		snapshot.SerializeInt(instance.m_iRevision);
		return true;
	}

	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_Briefing instance)
	{
		snapshot.SerializeString(instance.m_sId);
		snapshot.SerializeInt(instance.m_iOrder);
		snapshot.SerializeString(instance.m_sTitle);
		snapshot.SerializeString(instance.m_sContent);

		int textFormat;
		snapshot.SerializeInt(textFormat);
		instance.m_eTextFormat = textFormat;

		int audience;
		snapshot.SerializeInt(audience);
		instance.m_eAudience = audience;

		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeBool(instance.m_bVisible);
		snapshot.SerializeInt(instance.m_iRevision);
		return true;
	}

	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeInt(packet);
	}

	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeInt(packet);
		return true;
	}

	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 4)
			&& lhs.CompareSnapshots(rhs, 4);
	}

	static bool PropCompare(GC_Briefing instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareString(instance.m_sId)
			&& snapshot.CompareInt(instance.m_iOrder)
			&& snapshot.CompareString(instance.m_sTitle)
			&& snapshot.CompareString(instance.m_sContent)
			&& snapshot.CompareInt(instance.m_eTextFormat)
			&& snapshot.CompareInt(instance.m_eAudience)
			&& snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.CompareBool(instance.m_bVisible)
			&& snapshot.CompareInt(instance.m_iRevision);
	}
}
