//! Presentation binding for one dynamically-created voice-channel card.
class GC_VoiceChannelWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName PLAYER_ROW_LAYOUT = "{6A33000000000001}UI/layouts/Players/Lobby_Player_Row.layout";

	protected GC_VoiceChannelType m_ChannelType;
	protected Faction m_Faction;
	protected SCR_AIGroup m_Group;
	protected Widget m_wPlayers;
	protected ref array<Widget> m_aPlayerRows = {};

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		m_wPlayers = m_wRoot.FindAnyWidget("Players");
		Widget headerWidget = m_wRoot.FindAnyWidget("ChannelHeader");
		if (headerWidget)
		{
			GC_VoiceChannelHeaderComponent header = GC_VoiceChannelHeaderComponent.Cast(headerWidget.FindHandler(GC_VoiceChannelHeaderComponent));
			if (header)
				header.SetChannel(this);
			else
				Print("GC | Voice channel header missing GC_VoiceChannelHeaderComponent", level: LogLevel.ERROR);
		}
		else
		{
			Print("GC | Voice channel root missing ChannelHeader widget", level: LogLevel.ERROR);
		}

		ClearPlayerRows();
		Refresh();
	}

	//! Binds this card to a channel type from the lobby channel registry.
	void SetChannelType(GC_VoiceChannelType channelType)
	{
		m_ChannelType = channelType;
		Refresh();
	}

	//! Assigns the authoritative faction whose color styles this card.
	void SetFaction(Faction faction)
	{
		m_Faction = faction;
		RefreshFactionColor();
	}

	//! Retains the authoritative group represented by this group channel.
	void SetGroup(SCR_AIGroup group)
	{
		m_Group = group;
		Refresh();
	}

	GC_VoiceChannelType GetChannelType()
	{
		return m_ChannelType;
	}

	Faction GetFaction()
	{
		return m_Faction;
	}

	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}

	//! Replaces the card's player rows from a fresh authoritative membership query.
	void SetPlayers(array<int> playerIds, GC_LobbyManager lobbyManager)
	{
		if (!m_wPlayers || !lobbyManager)
		{
			PrintFormat("GC | Voice channel player rows skipped: playersWidget=%1 lobbyManager=%2", m_wPlayers != null, lobbyManager != null, level: LogLevel.WARNING);
			return;
		}

		ClearPlayerRows();

		foreach (int playerId : playerIds)
		{
			Widget rowWidget = GetGame().GetWorkspace().CreateWidgets(PLAYER_ROW_LAYOUT, m_wPlayers);
			if (!rowWidget)
				continue;

			GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
			if (!row)
			{
				Print("GC | Voice channel player row missing GC_VoicePlayerRowComponent", level: LogLevel.WARNING);
				rowWidget.RemoveFromHierarchy();
				continue;
			}

			row.SetPlayer(playerId, lobbyManager.GetSlotByPlayerId(playerId), m_Group);
			m_aPlayerRows.Insert(rowWidget);
		}
	}

	protected void Refresh()
	{
		if (!m_wRoot)
			return;

		TextWidget channelName = TextWidget.Cast(m_wRoot.FindAnyWidget("ChannelName"));
		if (!channelName)
			return;

		switch (m_ChannelType)
		{
			case GC_VoiceChannelType.GLOBAL:
				channelName.SetText("GLOBAL");
				break;
			case GC_VoiceChannelType.FACTION:
				channelName.SetText("FACTION");
				break;
			case GC_VoiceChannelType.GROUP:
				channelName.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));
				break;
		}

		RefreshFactionColor();
	}

	//! Applies the faction manager's actual faction color to both channel marks.
	void RefreshFactionColor()
	{
		if (!m_Faction || !m_wRoot)
			return;

		Color factionColor = m_Faction.GetFactionColor();
		ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("ChannelAccent"));
		if (accent)
			accent.SetColor(factionColor);

		ImageWidget divider = ImageWidget.Cast(m_wRoot.FindAnyWidget("ChannelDivider"));
		if (divider)
			divider.SetColor(factionColor);
	}

	protected void ClearPlayerRows()
	{
		if (!m_wPlayers)
			return;

		Widget child = m_wPlayers.GetChildren();
		while (child)
		{
			Widget nextChild = child.GetSibling();
			child.RemoveFromHierarchy();
			child = nextChild;
		}

		m_aPlayerRows.Clear();
	}
}
