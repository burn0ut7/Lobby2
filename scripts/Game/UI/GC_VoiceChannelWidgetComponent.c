//! Presentation binding for one dynamically-created voice-channel card.
class GC_VoiceChannelWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName PLAYER_ROW_LAYOUT = "{6A33000000000001}UI/layouts/Players/Lobby_Player_Row.layout";

	protected GC_VoiceChannelType m_ChannelType;
	protected Faction m_Faction;
	protected SCR_AIGroup m_Group;
	protected UUID m_PersonalOwnerIdentity;
	protected int m_iPersonalOwnerPlayerId = -1;
	protected Widget m_wPlayers;
	protected ref map<int, Widget> m_mPlayerRows = new map<int, Widget>();

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
	}

	void RefreshAdminNamePresentation(int playerId = -1)
	{
		if (!m_wRoot)
			return;

		int presentedPlayerId = -1;
		if (m_ChannelType == GC_VoiceChannelType.PERSONAL)
			presentedPlayerId = m_iPersonalOwnerPlayerId;
		GC_PlayerNamePresentation.Apply(m_wRoot.FindAnyWidget("ChannelName"), presentedPlayerId);

		if (playerId > 0)
		{
			Widget rowWidget;
			if (!m_mPlayerRows.Find(playerId, rowWidget) || !rowWidget)
				return;
			GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
			if (row)
				row.RefreshAdminNamePresentation();
			return;
		}

		foreach (int rowPlayerId, Widget rowWidget : m_mPlayerRows)
		{
			if (!rowWidget)
				continue;
			GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
			if (row)
				row.RefreshAdminNamePresentation();
		}
	}

	void Configure(GC_VoiceChannelType channelType, Faction faction = null, SCR_AIGroup group = null, UUID personalOwnerIdentity = "", int personalOwnerPlayerId = -1)
	{
		m_ChannelType = channelType;
		m_Faction = faction;
		m_Group = group;
		m_PersonalOwnerIdentity = personalOwnerIdentity;
		m_iPersonalOwnerPlayerId = personalOwnerPlayerId;
		Refresh();
	}

	void RefreshPresentation()
	{
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

	UUID GetPersonalOwnerIdentity()
	{
		return m_PersonalOwnerIdentity;
	}

	int GetPersonalOwnerPlayerId()
	{
		return m_iPersonalOwnerPlayerId;
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
			UpsertPlayer(playerId, lobbyManager);
	}

	void UpsertPlayer(int playerId, GC_LobbyManager lobbyManager)
	{
		if (!m_wPlayers || !lobbyManager || playerId <= 0)
			return;

		Widget rowWidget;
		if (!m_mPlayerRows.Find(playerId, rowWidget))
		{
			rowWidget = GetGame().GetWorkspace().CreateWidgets(PLAYER_ROW_LAYOUT, m_wPlayers);
			if (!rowWidget)
				return;
			m_mPlayerRows.Set(playerId, rowWidget);
		}

		GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
		if (!row)
		{
			Print("GC | Voice channel player row missing GC_VoicePlayerRowComponent", level: LogLevel.WARNING);
			RemovePlayer(playerId);
			return;
		}

		row.SetPlayer(playerId, lobbyManager.GetSlotByPlayerId(playerId));
	}

	void RemovePlayer(int playerId)
	{
		Widget rowWidget;
		if (!m_mPlayerRows.Find(playerId, rowWidget))
			return;
		if (rowWidget)
			rowWidget.RemoveFromHierarchy();
		m_mPlayerRows.Remove(playerId);
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
			{
				string factionName;
				if (m_Faction)
				{
					UIInfo factionUIInfo = m_Faction.GetUIInfo();
					if (factionUIInfo)
						factionName = WidgetManager.Translate(factionUIInfo.GetName());
					else
						factionName = WidgetManager.Translate(m_Faction.GetFactionName());
				}

				if (factionName.IsEmpty())
					channelName.SetText("FACTION HQ");
				else
					channelName.SetTextFormat("%1 HQ", factionName);
				break;
			}
			case GC_VoiceChannelType.GROUP:
				channelName.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));
				break;
			case GC_VoiceChannelType.PERSONAL:
			{
				string ownerName = GetGame().GetPlayerManager().GetPlayerName(m_iPersonalOwnerPlayerId);
				if (ownerName.IsEmpty())
					channelName.SetText("PERSONAL CHANNEL");
				else
					channelName.SetTextFormat("%1'S CHANNEL", ownerName);
				break;
			}
		}

		RefreshFactionColor();
		RefreshAdminNamePresentation();
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

		Widget background = m_wRoot.FindAnyWidget("ChannelHeaderBackground");
		if (background)
			background.SetColor(GC_LobbyUIColors.FactionBackground(m_Faction));
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

		m_mPlayerRows.Clear();
	}
}
