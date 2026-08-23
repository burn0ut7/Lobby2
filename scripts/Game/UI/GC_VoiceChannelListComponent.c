//! Builds the voice-channel cards from the authoritative lobby registry.
//! This is the UI bridge only; actual voice routing remains a later slice.
class GC_VoiceChannelListComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName CHANNEL_LAYOUT = "{6A34000000000001}UI/layouts/Voice/Lobby_Voice_Channel.layout";

	protected Widget m_wChannels;
	protected GC_LobbyManager m_LobbyManager;
	protected ref map<string, Widget> m_mChannelWidgets = new map<string, Widget>();

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

		m_LobbyManager.m_OnRegistryReset.Insert(OnRegistryReset);
		m_LobbyManager.m_OnPlayerConnected.Insert(OnPlayerConnected);
		m_LobbyManager.m_OnPlayerDisconnected.Insert(OnPlayerDisconnected);
		m_LobbyManager.m_OnPlayerAffiliationChanged.Insert(OnPlayerAffiliationChanged);
		m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnFactionTopologyChanged);
		m_LobbyManager.m_OnFactionPresentationChanged.Insert(OnFactionPresentationChanged);
		m_LobbyManager.m_OnVoiceAssignmentChanged.Insert(OnVoiceAssignmentChanged);
		m_LobbyManager.m_OnVoiceTopologyChanged.Insert(OnVoiceTopologyChanged);
		m_LobbyManager.m_OnSlotPresentationChanged.Insert(OnSlotPresentationChanged);
		m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);
		m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);
		Refresh();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnRegistryReset.Remove(OnRegistryReset);
			m_LobbyManager.m_OnPlayerConnected.Remove(OnPlayerConnected);
			m_LobbyManager.m_OnPlayerDisconnected.Remove(OnPlayerDisconnected);
			m_LobbyManager.m_OnPlayerAffiliationChanged.Remove(OnPlayerAffiliationChanged);
			m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnFactionTopologyChanged);
			m_LobbyManager.m_OnFactionPresentationChanged.Remove(OnFactionPresentationChanged);
			m_LobbyManager.m_OnVoiceAssignmentChanged.Remove(OnVoiceAssignmentChanged);
			m_LobbyManager.m_OnVoiceTopologyChanged.Remove(OnVoiceTopologyChanged);
			m_LobbyManager.m_OnSlotPresentationChanged.Remove(OnSlotPresentationChanged);
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);
			m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
		}

		ClearChannelWidgets();
		m_LobbyManager = null;
		m_wChannels = null;
		super.HandlerDeattached(w);
	}

	protected void RefreshAdminNamePresentation(int playerId)
	{
		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			if (!channelWidget)
				continue;

			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (channel)
				channel.RefreshAdminNamePresentation(playerId);
		}
	}

	protected void OnPlayerRoleChanged(int playerId)
	{
		RefreshAdminNamePresentation(playerId);
	}

	//! Rebuilds shared channels first, followed by discoverable personal channels.
	void Refresh()
	{
		if (!m_wChannels || !m_LobbyManager)
			return;

		ClearChannelWidgets();
		ReconcileChannels();
		RefreshPlayers();
	}

	//! Preserves existing cards and creates/removes only genuine topology changes.
	protected bool ReconcileChannels(int excludedPlayerId = -1)
	{
		if (!m_wChannels || !m_LobbyManager)
			return false;

		array<string> desiredKeys = {};
		bool changed;
		Faction localFaction = GetLocalPlayerFaction();
		if (m_LobbyManager.GetLobbyPhase() != SCR_EGameModeState.BRIEFING || !localFaction)
			changed = EnsureChannel(desiredKeys, GC_VoiceChannelType.GLOBAL) || changed;

		array<Faction> factions = m_LobbyManager.GetRegisteredFactions();
		array<SCR_AIGroup> groups = m_LobbyManager.GetRegisteredGroups();
		array<int> connectedPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(connectedPlayers);

		// Keep each faction's channel beside its group channels so the list reads
		// as one hierarchy: Global, faction, that faction's groups, next faction.
		foreach (Faction faction : factions)
		{
			if (!faction || !localFaction || faction != localFaction)
				continue;

			changed = EnsureChannel(desiredKeys, GC_VoiceChannelType.FACTION, faction) || changed;
			foreach (SCR_AIGroup group : groups)
			{
				if (group && group.GetFaction() == faction)
					changed = EnsureChannel(desiredKeys, GC_VoiceChannelType.GROUP, faction, group) || changed;
			}
		}

		int localPlayerId = GetLocalPlayerId();
		foreach (int ownerPlayerId : connectedPlayers)
		{
			if (ownerPlayerId == excludedPlayerId)
				continue;
			UUID ownerIdentity = m_LobbyManager.GetPlayerVoiceIdentity(ownerPlayerId);
			if (ownerPlayerId != localPlayerId && m_LobbyManager.CanAccessPersonalVoiceChannel(localPlayerId, ownerIdentity))
				changed = EnsureChannel(desiredKeys, GC_VoiceChannelType.PERSONAL, m_LobbyManager.GetPersonalVoiceChannelFaction(ownerIdentity), null, ownerIdentity, ownerPlayerId) || changed;
		}

		// The local player's unopened channel remains private and is always last.
		if (localPlayerId > 0)
		{
			UUID localIdentity = m_LobbyManager.GetPlayerVoiceIdentity(localPlayerId);
			if (!localIdentity.IsEmpty())
				changed = EnsureChannel(desiredKeys, GC_VoiceChannelType.PERSONAL, m_LobbyManager.GetPersonalVoiceChannelFaction(localIdentity), null, localIdentity, localPlayerId) || changed;
		}

		array<string> removedKeys = {};
		foreach (string existingKey, Widget channelWidget : m_mChannelWidgets)
		{
			if (!desiredKeys.Contains(existingKey))
				removedKeys.Insert(existingKey);
		}
		foreach (string removedKey : removedKeys)
		{
			Widget removedWidget;
			if (m_mChannelWidgets.Find(removedKey, removedWidget) && removedWidget)
				removedWidget.RemoveFromHierarchy();
			m_mChannelWidgets.Remove(removedKey);
			changed = true;
		}

		return changed;
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

	protected void OnRegistryReset(int revision)
	{
		Refresh();
	}

	protected void OnPlayerConnected(int playerId)
	{
		if (ReconcileChannels())
			RefreshPlayers();
		else
			RefreshPlayer(playerId);
	}

	protected void OnPlayerDisconnected(int playerId)
	{
		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			GC_VoiceChannelWidgetComponent channel;
			if (channelWidget)
				channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (channel)
				channel.RemovePlayer(playerId);
		}
		ReconcileChannels(playerId);
	}

	protected void OnPlayerAffiliationChanged(int playerId)
	{
		if (playerId == GetLocalPlayerId() && ReconcileChannels())
			RefreshPlayers();
		else
			RefreshPlayer(playerId);
	}

	protected void OnFactionTopologyChanged(Faction faction)
	{
		if (ReconcileChannels())
			RefreshPlayers();
	}

	protected void OnFactionPresentationChanged(Faction faction)
	{
		if (!faction)
			return;
		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			GC_VoiceChannelWidgetComponent channel;
			if (channelWidget)
				channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (channel && channel.GetFaction() == faction)
				channel.RefreshPresentation();
		}
	}

	protected void OnVoiceAssignmentChanged(int playerId)
	{
		if (ReconcileChannels())
			RefreshPlayers();
		else
			RefreshPlayer(playerId);
	}

	protected void OnVoiceTopologyChanged(int playerId)
	{
		if (ReconcileChannels())
			RefreshPlayers();
	}

	protected void OnSlotPresentationChanged(GC_Slot slot)
	{
		if (slot && slot.GetPlayerId() > 0)
			RefreshPlayer(slot.GetPlayerId());
	}

	protected void OnLobbyPhaseChanged(SCR_EGameModeState phase)
	{
		if (ReconcileChannels())
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

		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			if (!channelWidget)
				continue;

			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (!channel)
				continue;

			array<int> members = GetChannelMembers(channel.GetChannelType(), channel.GetFaction(), channel.GetGroup(), channel.GetPersonalOwnerIdentity(), connectedPlayers);
			channel.SetPlayers(members, m_LobbyManager);
		}

	}

	protected void RefreshPlayer(int playerId)
	{
		if (!m_LobbyManager || playerId <= 0)
			return;

		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			if (!channelWidget)
				continue;
			GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (!channel)
				continue;

			if (IsChannelMember(channel, playerId))
				channel.UpsertPlayer(playerId, m_LobbyManager);
			else
				channel.RemovePlayer(playerId);
		}
	}

	protected bool IsChannelMember(GC_VoiceChannelWidgetComponent channel, int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!channel || !playerManager || !playerManager.IsPlayerConnected(playerId))
			return false;

		GC_VoiceChannelAssignment assignment = m_LobbyManager.GetPlayerVoiceAssignment(playerId);
		GC_VoiceChannelType assignedType = GC_VoiceChannelType.GLOBAL;
		if (assignment)
			assignedType = assignment.GetChannelType();
		if (assignedType != channel.GetChannelType())
			return false;

		if (assignedType == GC_VoiceChannelType.GLOBAL)
			return true;
		if (!assignment)
			return false;
		if (assignedType == GC_VoiceChannelType.FACTION && channel.GetFaction())
			return assignment.GetFactionKey() == channel.GetFaction().GetFactionKey();
		if (assignedType == GC_VoiceChannelType.GROUP && channel.GetGroup())
			return assignment.GetGroupRplId() == Replication.FindItemId(channel.GetGroup());
		if (assignedType == GC_VoiceChannelType.PERSONAL)
			return assignment.GetPersonalOwnerIdentity() == channel.GetPersonalOwnerIdentity();

		return false;
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

	protected bool EnsureChannel(array<string> desiredKeys, GC_VoiceChannelType channelType, Faction faction = null, SCR_AIGroup group = null, UUID personalOwnerIdentity = "", int personalOwnerPlayerId = -1)
	{
		string channelKey = GetChannelKey(channelType, faction, group, personalOwnerIdentity);
		desiredKeys.Insert(channelKey);
		int channelOrder = desiredKeys.Count() - 1;
		if (m_mChannelWidgets.Contains(channelKey))
		{
			Widget existingWidget;
			m_mChannelWidgets.Find(channelKey, existingWidget);
			if (existingWidget)
				existingWidget.SetZOrder(channelOrder);
			GC_VoiceChannelWidgetComponent existingChannel;
			if (existingWidget)
				existingChannel = GC_VoiceChannelWidgetComponent.Cast(existingWidget.FindHandler(GC_VoiceChannelWidgetComponent));
			if (existingChannel && (existingChannel.GetFaction() != faction || existingChannel.GetGroup() != group || existingChannel.GetPersonalOwnerPlayerId() != personalOwnerPlayerId))
				existingChannel.Configure(channelType, faction, group, personalOwnerIdentity, personalOwnerPlayerId);
			return false;
		}

		Widget channelWidget = GetGame().GetWorkspace().CreateWidgets(CHANNEL_LAYOUT, m_wChannels);
		if (!channelWidget)
		{
			PrintFormat("GC | Failed to create voice channel widget type=%1", channelType, level: LogLevel.WARNING);
			return false;
		}

		GC_VoiceChannelWidgetComponent channel = GC_VoiceChannelWidgetComponent.Cast(channelWidget.FindHandler(GC_VoiceChannelWidgetComponent));
		if (!channel)
		{
			PrintFormat("GC | Voice channel widget missing GC_VoiceChannelWidgetComponent type=%1", channelType, level: LogLevel.WARNING);
			channelWidget.RemoveFromHierarchy();
			return false;
		}

		channel.Configure(channelType, faction, group, personalOwnerIdentity, personalOwnerPlayerId);
		channelWidget.SetZOrder(channelOrder);
		m_mChannelWidgets.Set(channelKey, channelWidget);
		return true;
	}

	protected string GetChannelKey(GC_VoiceChannelType channelType, Faction faction = null, SCR_AIGroup group = null, UUID personalOwnerIdentity = "")
	{
		if (channelType == GC_VoiceChannelType.GLOBAL)
			return "global";
		if (channelType == GC_VoiceChannelType.FACTION && faction)
			return "faction:" + faction.GetFactionKey();
		if (channelType == GC_VoiceChannelType.GROUP && group)
			return string.Format("group:%1", Replication.FindItemId(group));
		if (channelType == GC_VoiceChannelType.PERSONAL)
			return "personal:" + personalOwnerIdentity;
		return string.Format("invalid:%1", channelType);
	}

	protected void ClearChannelWidgets()
	{
		foreach (string channelKey, Widget channelWidget : m_mChannelWidgets)
		{
			if (channelWidget)
				channelWidget.RemoveFromHierarchy();
		}

		m_mChannelWidgets.Clear();

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
