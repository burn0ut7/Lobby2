//! Builds the voice-channel cards from the authoritative lobby registry.
//! This is the UI bridge only; actual voice routing remains a later slice.
class GC_VoiceChannelListComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName CHANNEL_LAYOUT = "{6A34000000000001}UI/layouts/Voice/Lobby_Voice_Channel.layout";

	protected Widget m_wChannels;
	protected GC_LobbyManager m_LobbyManager;
	protected ref array<Widget> m_aChannelWidgets = {};

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		m_wChannels = m_wRoot.FindAnyWidget("Channels");
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_wChannels || !m_LobbyManager)
		{
			PrintFormat("GC | Voice channel list attach failed: channels=%1 lobbyManager=%2", m_wChannels != null, m_LobbyManager != null, level: LogLevel.WARNING);
			return;
		}

		m_LobbyManager.m_OnFactionUpdated.Insert(OnLobbyRegistryUpdated);
		m_LobbyManager.m_OnPlayerRegistryUpdated.Insert(OnLobbyPlayerRegistryUpdated);
		PrintFormat("GC | Voice channel list attached: factions=%1 groups=%2", m_LobbyManager.GetRegisteredFactions().Count(), m_LobbyManager.GetRegisteredGroups().Count());
		Refresh();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnFactionUpdated.Remove(OnLobbyRegistryUpdated);
			m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnLobbyPlayerRegistryUpdated);
		}

		ClearChannelWidgets();
		m_LobbyManager = null;
		m_wChannels = null;
		super.HandlerDeattached(w);
	}

	//! Rebuilds Global, faction, and group channel cards from current registry state.
	void Refresh()
	{
		if (!m_wChannels || !m_LobbyManager)
			return;

		ClearChannelWidgets();
		CreateChannel(GC_VoiceChannelType.GLOBAL);

		array<Faction> factions = m_LobbyManager.GetRegisteredFactions();
		array<SCR_AIGroup> groups = m_LobbyManager.GetRegisteredGroups();
		Faction localFaction = GetLocalPlayerFaction();

		// Keep each faction's channel beside its group channels so the list reads
		// as one hierarchy: Global, faction, that faction's groups, next faction.
		foreach (Faction faction : factions)
		{
			if (!faction || !localFaction || faction != localFaction)
				continue;

			CreateChannel(GC_VoiceChannelType.FACTION, faction);
			foreach (SCR_AIGroup group : groups)
			{
				if (group && group.GetFaction() == faction)
					CreateChannel(GC_VoiceChannelType.GROUP, faction, group);
			}
		}

		RefreshPlayers();

		PrintFormat("GC | Voice channel list refreshed: localFaction=%1 visibleChannels=%2", localFaction, m_aChannelWidgets.Count());
	}

	protected Faction GetLocalPlayerFaction()
	{
		if (!m_LobbyManager)
			return null;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return null;

		return m_LobbyManager.GetPlayerFaction(playerController.GetPlayerId());
	}

	protected void OnLobbyRegistryUpdated(Faction faction)
	{
		if (!HasCurrentTopology())
		{
			Refresh();
			return;
		}

		foreach (Widget channelWidget : m_aChannelWidgets)
		{
			if (!channelWidget)
				continue;

			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (channel)
				channel.RefreshFactionColor();
		}

		RefreshPlayers();
	}

	protected void OnLobbyPlayerRegistryUpdated(int playerId)
	{
		if (!HasCurrentTopology())
		{
			Refresh();
			return;
		}

		RefreshPlayers();
	}

	//! Global contains every connected player. Faction and group membership are
	//! derived from the base-game group manager and therefore do not require a
	//! Lobby2 slot record to exist.
	protected void RefreshPlayers()
	{
		if (!m_LobbyManager)
		{
			Print("GC | Voice channel player refresh skipped: lobby manager unavailable", level: LogLevel.WARNING);
			return;
		}

		array<int> connectedPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
		PrintFormat("GC | Voice channel player refresh begin: connected=%1 channelWidgets=%2", connectedPlayers.Count(), m_aChannelWidgets.Count());

		int totalRows;
		foreach (Widget channelWidget : m_aChannelWidgets)
		{
			if (!channelWidget)
				continue;

			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (!channel)
				continue;

			array<int> members = GetChannelMembers(channel.GetChannelType(), channel.GetFaction(), channel.GetGroup(), connectedPlayers);
			channel.SetPlayers(members, m_LobbyManager);
			totalRows += members.Count();
		}

		PrintFormat("GC | Voice channel players refreshed: connected=%1 channels=%2 rows=%3", connectedPlayers.Count(), m_aChannelWidgets.Count(), totalRows);
	}

	protected array<int> GetChannelMembers(GC_VoiceChannelType channelType, Faction faction, SCR_AIGroup group, array<int> connectedPlayers)
	{
		array<int> members = {};

		foreach (int playerId : connectedPlayers)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;

			GC_VoiceChannelAssignment assignment = m_LobbyManager.GetPlayerVoiceAssignment(playerId);
			GC_VoiceChannelType assignedType = GC_VoiceChannelType.GLOBAL;
			if (assignment)
				assignedType = assignment.GetChannelType();

			if (assignedType != channelType)
				continue;

			if (channelType == GC_VoiceChannelType.GLOBAL)
				members.Insert(playerId);
			else if (channelType == GC_VoiceChannelType.FACTION && faction && assignment.GetFactionKey() == faction.GetFactionKey())
				members.Insert(playerId);
			else if (channelType == GC_VoiceChannelType.GROUP && group && assignment.GetGroupRplId() == Replication.FindItemId(group))
				members.Insert(playerId);
		}

		return members;
	}

	protected bool HasCurrentTopology()
	{
		if (!m_LobbyManager)
			return m_aChannelWidgets.IsEmpty();

		Faction localFaction = GetLocalPlayerFaction();
		int expectedCount = 1;
		if (localFaction)
		{
			expectedCount++;
			foreach (SCR_AIGroup group : m_LobbyManager.GetRegisteredGroups())
			{
				if (group && group.GetFaction() == localFaction)
					expectedCount++;
			}
		}
		if (expectedCount != m_aChannelWidgets.Count())
			return false;

		int index;
		array<SCR_AIGroup> groups = m_LobbyManager.GetRegisteredGroups();

		Widget globalWidget = m_aChannelWidgets[index];
		GC_VoiceChannelWidgetComponent globalChannel = GC_VoiceChannelWidgetComponent.Cast(globalWidget.FindHandler(GC_VoiceChannelWidgetComponent));
		if (!globalChannel || globalChannel.GetChannelType() != GC_VoiceChannelType.GLOBAL)
			return false;
		index++;
		if (!localFaction)
			return index == m_aChannelWidgets.Count();

		Widget factionWidget = m_aChannelWidgets[index];
		GC_VoiceChannelWidgetComponent factionChannel = GC_VoiceChannelWidgetComponent.Cast(factionWidget.FindHandler(GC_VoiceChannelWidgetComponent));
		if (!factionChannel || factionChannel.GetChannelType() != GC_VoiceChannelType.FACTION || factionChannel.GetFaction() != localFaction || factionChannel.GetGroup())
			return false;
		index++;

		foreach (SCR_AIGroup group : groups)
		{
			if (!group || group.GetFaction() != localFaction)
				continue;
			if (index >= m_aChannelWidgets.Count())
				return false;

			Widget groupWidget = m_aChannelWidgets[index];
			GC_VoiceChannelWidgetComponent groupChannel = GC_VoiceChannelWidgetComponent.Cast(groupWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (!groupChannel || groupChannel.GetChannelType() != GC_VoiceChannelType.GROUP || groupChannel.GetGroup() != group || groupChannel.GetFaction() != localFaction)
				return false;
			index++;
		}

		return index == m_aChannelWidgets.Count();
	}

	protected void CreateChannel(GC_VoiceChannelType channelType, Faction faction = null, SCR_AIGroup group = null)
	{
		Widget channelWidget = GetGame().GetWorkspace().CreateWidgets(CHANNEL_LAYOUT, m_wChannels);
		if (!channelWidget)
		{
			PrintFormat("GC | Failed to create voice channel widget type=%1", channelType, level: LogLevel.WARNING);
			return;
		}

		GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
		if (!channel)
		{
			PrintFormat("GC | Voice channel widget missing GC_VoiceChannelWidgetComponent type=%1", channelType, level: LogLevel.WARNING);
			channelWidget.RemoveFromHierarchy();
			return;
		}

		channel.SetChannelType(channelType);
		channel.SetFaction(faction);
		channel.SetGroup(group);
		m_aChannelWidgets.Insert(channelWidget);
	}

	protected void ClearChannelWidgets()
	{
		foreach (Widget channelWidget : m_aChannelWidgets)
		{
			if (channelWidget)
				channelWidget.RemoveFromHierarchy();
		}

		m_aChannelWidgets.Clear();

		if (!m_wChannels)
			return;

		Widget previewChannel = m_wChannels.GetChildren();
		while (previewChannel)
		{
			Widget nextChannel = previewChannel.GetSibling();
			previewChannel.RemoveFromHierarchy();
			previewChannel = nextChannel;
		}
	}
}
