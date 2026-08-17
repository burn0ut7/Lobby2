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
		m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);
		Refresh();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnFactionUpdated.Remove(OnLobbyRegistryUpdated);
			m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnLobbyPlayerRegistryUpdated);
			m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
		}

		ClearChannelWidgets();
		m_LobbyManager = null;
		m_wChannels = null;
		super.HandlerDeattached(w);
	}

	//! Rebuilds shared channels first, followed by discoverable personal channels.
	void Refresh()
	{
		if (!m_wChannels || !m_LobbyManager)
			return;

		ClearChannelWidgets();
		if (m_LobbyManager.GetLobbyPhase() != GC_ELobbyPhase.BRIEFING)
			CreateChannel(GC_VoiceChannelType.GLOBAL);

		array<Faction> factions = m_LobbyManager.GetRegisteredFactions();
		array<SCR_AIGroup> groups = m_LobbyManager.GetRegisteredGroups();
		array<int> connectedPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(connectedPlayers);
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

		int localPlayerId = GetLocalPlayerId();
		foreach (int ownerPlayerId : connectedPlayers)
		{
			UUID ownerIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(ownerPlayerId);
			if (ownerPlayerId != localPlayerId && m_LobbyManager.CanAccessPersonalVoiceChannel(localPlayerId, ownerIdentity))
				CreateChannel(GC_VoiceChannelType.PERSONAL, m_LobbyManager.GetPersonalVoiceChannelFaction(ownerIdentity), null, ownerIdentity, ownerPlayerId);
		}

		// The local player's unopened channel remains private and is always last.
		if (localPlayerId > 0)
		{
			UUID localIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(localPlayerId);
			CreateChannel(GC_VoiceChannelType.PERSONAL, m_LobbyManager.GetPersonalVoiceChannelFaction(localIdentity), null, localIdentity, localPlayerId);
		}

		RefreshPlayers();

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

	protected int GetLocalPlayerId()
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return -1;

		return playerController.GetPlayerId();
	}

	protected void OnLobbyRegistryUpdated(Faction faction)
	{
		Refresh();
	}

	protected void OnLobbyPlayerRegistryUpdated(int playerId)
	{
		Refresh();
	}

	protected void OnLobbyPhaseChanged(GC_ELobbyPhase phase)
	{
		Refresh();
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

		int totalRows;
		foreach (Widget channelWidget : m_aChannelWidgets)
		{
			if (!channelWidget)
				continue;

			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (!channel)
				continue;

			array<int> members = GetChannelMembers(channel.GetChannelType(), channel.GetFaction(), channel.GetGroup(), channel.GetPersonalOwnerIdentity(), connectedPlayers);
			channel.SetPlayers(members, m_LobbyManager);
			totalRows += members.Count();
		}

	}

	protected array<int> GetChannelMembers(GC_VoiceChannelType channelType, Faction faction, SCR_AIGroup group, UUID personalOwnerIdentity, array<int> connectedPlayers)
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
			else if (channelType == GC_VoiceChannelType.PERSONAL && assignment.GetPersonalOwnerIdentity() == personalOwnerIdentity)
				members.Insert(playerId);
		}

		return members;
	}

	protected void CreateChannel(GC_VoiceChannelType channelType, Faction faction = null, SCR_AIGroup group = null, UUID personalOwnerIdentity = "", int personalOwnerPlayerId = -1)
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
		channel.SetPersonalOwner(personalOwnerIdentity, personalOwnerPlayerId);
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
