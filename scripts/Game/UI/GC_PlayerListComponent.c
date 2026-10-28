//! Builds the reusable all-player list and applies the local name filter.
class GC_PlayerListComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName PLAYER_ROW_LAYOUT = "{6A33000000000001}UI/layouts/Players/Lobby_Player_Row.layout";

	protected Widget m_wPlayers;
	protected GC_LobbyManager m_LobbyManager;
	protected string m_sSearchText;
	protected ref map<int, Widget> m_mPlayerRows = new map<int, Widget>();

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		m_wPlayers = m_wRoot.FindAnyWidget("Players");
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_wPlayers || !m_LobbyManager)
		{
			PrintFormat("GC | Player list attach failed: players=%1 lobbyManager=%2", m_wPlayers != null, m_LobbyManager != null, level: LogLevel.WARNING);
			return;
		}

		m_LobbyManager.m_OnRegistryReset.Insert(OnRegistryReset);
		m_LobbyManager.m_OnPlayerConnected.Insert(OnPlayerConnected);
		m_LobbyManager.m_OnPlayerDisconnected.Insert(OnPlayerDisconnected);
		m_LobbyManager.m_OnPlayerAffiliationChanged.Insert(OnPlayerAffiliationChanged);
		m_LobbyManager.m_OnSlotPresentationChanged.Insert(OnSlotPresentationChanged);
		m_LobbyManager.m_OnVoiceAssignmentChanged.Insert(OnVoiceAssignmentChanged);
		m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);
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
			m_LobbyManager.m_OnSlotPresentationChanged.Remove(OnSlotPresentationChanged);
			m_LobbyManager.m_OnVoiceAssignmentChanged.Remove(OnVoiceAssignmentChanged);
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);
		}

		ClearPlayerRows();
		m_LobbyManager = null;
		m_wPlayers = null;
		super.HandlerDeattached(w);
	}

	protected void RefreshAdminNamePresentation(int playerId)
	{
		Widget rowWidget;
		if (!m_mPlayerRows.Find(playerId, rowWidget) || !rowWidget)
			return;

		GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
		if (row)
			row.RefreshAdminNamePresentation();
	}

	protected void OnPlayerRoleChanged(int playerId)
	{
		RefreshAdminNamePresentation(playerId);
	}

	void SetSearchText(string searchText)
	{
		if (m_sSearchText == searchText)
			return;

		m_sSearchText = searchText;
		ApplySearchFilter();
	}

	void Refresh()
	{
		if (!m_wPlayers || !m_LobbyManager)
			return;

		ClearPlayerRows();

		array<int> connectedPlayers = {};
		PlayerManager playerManager = GetGame().GetPlayerManager();
		playerManager.GetPlayers(connectedPlayers);

		foreach (int playerId : connectedPlayers)
			UpsertPlayerRow(playerId);

		ApplySearchFilter();
	}

	protected void UpsertPlayerRow(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
		{
			RemovePlayerRow(playerId);
			return;
		}

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
			Print("GC | Player list row missing GC_VoicePlayerRowComponent", level: LogLevel.WARNING);
			RemovePlayerRow(playerId);
			return;
		}

		row.SetPlayer(playerId, m_LobbyManager.GetSlotByPlayerId(playerId));
	}

	protected void RemovePlayerRow(int playerId)
	{
		Widget rowWidget;
		if (!m_mPlayerRows.Find(playerId, rowWidget))
			return;
		if (rowWidget)
			rowWidget.RemoveFromHierarchy();
		m_mPlayerRows.Remove(playerId);
	}

	protected void ApplySearchFilter()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		string searchNeedle = m_sSearchText;
		searchNeedle.ToLower();
		foreach (int playerId, Widget rowWidget : m_mPlayerRows)
		{
			if (rowWidget)
				rowWidget.SetVisible(MatchesSearch(playerManager.GetPlayerName(playerId), searchNeedle));
		}
	}

	protected bool MatchesSearch(string playerName, string searchNeedle)
	{
		if (searchNeedle.IsEmpty())
			return true;

		playerName.ToLower();
		return playerName.Contains(searchNeedle);
	}

	protected void OnRegistryReset(int revision)
	{
		Refresh();
	}

	protected void OnPlayerConnected(int playerId)
	{
		UpsertPlayerRow(playerId);
		ApplySearchFilter();
	}

	protected void OnPlayerDisconnected(int playerId)
	{
		RemovePlayerRow(playerId);
	}

	protected void OnPlayerAffiliationChanged(int playerId)
	{
		UpsertPlayerRow(playerId);
	}

	protected void OnSlotPresentationChanged(GC_Slot slot)
	{
		if (slot && slot.GetPlayerId() > 0)
			UpsertPlayerRow(slot.GetPlayerId());
	}

	protected void OnVoiceAssignmentChanged(int playerId)
	{
		// The same row layout is used by the voice and player lists, but each
		// collection owns its own widget instance. Refresh only this list's row.
		UpsertPlayerRow(playerId);
	}

	protected void ClearPlayerRows()
	{
		foreach (int playerId, Widget rowWidget : m_mPlayerRows)
		{
			if (rowWidget)
				rowWidget.RemoveFromHierarchy();
		}

		m_mPlayerRows.Clear();

		if (!m_wPlayers)
			return;

		Widget previewRow = m_wPlayers.GetChildren();
		while (previewRow)
		{
			Widget nextRow = previewRow.GetSibling();
			previewRow.RemoveFromHierarchy();
			previewRow = nextRow;
		}
	}
}

//! Forwards live edit-box changes to the player-list owner.
class GC_PlayerSearchComponent : SCR_ScriptedWidgetComponent
{
	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget editBox = EditBoxWidget.Cast(w);
		if (!editBox)
			return false;

		Widget parent = m_wRoot.GetParent();
		while (parent)
		{
			GC_PlayerListComponent playerList = GC_PlayerListComponent.Cast(parent.FindHandler(GC_PlayerListComponent));
			if (playerList)
			{
				playerList.SetSearchText(editBox.GetText());
				return true;
			}

			parent = parent.GetParent();
		}

		return false;
	}
}

//! Owns the Voice Channels / Player List tab selection and presentation.
class GC_PlayerListTabsComponent : SCR_ScriptedWidgetComponent
{
	[Attribute("0")]
	protected bool m_bSpectatorFactions;
	protected SCR_ButtonTextComponent m_FactionListButton;
	protected GC_SpectatorFactionListComponent m_FactionList;
	protected SCR_ButtonBaseComponent m_VoiceChannelsButton;
	protected SCR_ButtonBaseComponent m_PlayerListButton;
	protected SCR_ButtonBaseComponent m_DeafenButton;
	protected GC_LobbyManager m_LobbyManager;
	protected Widget m_wVoiceChannelList;
	protected Widget m_wPlayerList;
	protected bool m_bInitialized;
	protected bool m_bPlayerListDefault;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Initialize);

		if (m_FactionListButton)
			m_FactionListButton.m_OnClicked.Remove(OnFactionTab);
		if (m_VoiceChannelsButton)
			m_VoiceChannelsButton.m_OnClicked.Remove(OnTabClicked);
		if (m_PlayerListButton)
			m_PlayerListButton.m_OnClicked.Remove(OnTabClicked);
		if (m_DeafenButton)
			m_DeafenButton.m_OnClicked.Remove(OnDeafenClicked);
		if (m_LobbyManager)
			m_LobbyManager.m_OnVoiceAssignmentChanged.Remove(OnVoiceStateUpdated);

		m_VoiceChannelsButton = null;
		m_PlayerListButton = null;
		m_DeafenButton = null;
		m_LobbyManager = null;
		m_wVoiceChannelList = null;
		m_wPlayerList = null;
		super.HandlerDeattached(w);
	}

	protected void Initialize()
	{
		if (m_bInitialized)
			return;

		Widget voiceButtonWidget = m_wRoot.FindAnyWidget("VoiceChannelsButton");
		Widget playerButtonWidget = m_wRoot.FindAnyWidget("PlayerListButton");
		Widget deafenButtonWidget = m_wRoot.FindAnyWidget("DeafenButton");
		if (voiceButtonWidget)
			m_VoiceChannelsButton = SCR_ButtonBaseComponent.Cast(voiceButtonWidget.FindHandler(SCR_ButtonBaseComponent));
		if (playerButtonWidget)
			m_PlayerListButton = SCR_ButtonBaseComponent.Cast(playerButtonWidget.FindHandler(SCR_ButtonBaseComponent));
		if (deafenButtonWidget)
			m_DeafenButton = SCR_ButtonBaseComponent.Cast(deafenButtonWidget.FindHandler(SCR_ButtonBaseComponent));

		m_LobbyManager = GC_LobbyManager.GetManager();
		m_wVoiceChannelList = m_wRoot.FindAnyWidget("VoiceChannelList");
		m_wPlayerList = m_wRoot.FindAnyWidget("PlayerList");
		if (!m_VoiceChannelsButton || !m_PlayerListButton || !m_DeafenButton || !m_LobbyManager || !m_wVoiceChannelList || !m_wPlayerList)
		{
			PrintFormat("GC | Player list tabs attach failed: voiceButton=%1 playerButton=%2 deafenButton=%3 lobbyManager=%4 voicePanel=%5 playerPanel=%6", m_VoiceChannelsButton != null, m_PlayerListButton != null, m_DeafenButton != null, m_LobbyManager != null, m_wVoiceChannelList != null, m_wPlayerList != null, level: LogLevel.ERROR);
			return;
		}

		if (m_bSpectatorFactions)
		{
			m_FactionListButton = SCR_ButtonTextComponent.GetButtonText("FactionListButton", m_wRoot);
			m_FactionListButton.SetText("FACTIONS");
			m_FactionListButton.GetRootWidget().SetVisible(true);
			m_FactionListButton.m_OnClicked.Insert(OnFactionTab);
			Widget factionPanel = GetGame().GetWorkspace().CreateWidgets("{D389716BFE230B01}UI/layouts/Spectator/Lobby_Spectator_Factions.layout", m_wRoot.FindAnyWidget("PlayerListContent"));
			m_FactionList = GC_SpectatorFactionListComponent.Cast(factionPanel.FindHandler(GC_SpectatorFactionListComponent));
		}
		m_bInitialized = true;
		m_VoiceChannelsButton.m_OnClicked.Insert(OnTabClicked);
		m_PlayerListButton.m_OnClicked.Insert(OnTabClicked);
		m_DeafenButton.m_OnClicked.Insert(OnDeafenClicked);
		m_LobbyManager.m_OnVoiceAssignmentChanged.Insert(OnVoiceStateUpdated);
		SetPlayerListActive(m_bPlayerListDefault);
		RefreshDeafenVisuals();
	}

	void SetPlayerListDefault(bool playerListDefault)
	{
		m_bPlayerListDefault = playerListDefault;
		if (m_bInitialized)
			SetPlayerListActive(m_bPlayerListDefault);
	}

	void SetDarkBackground(bool darkBackground)
	{
		Widget voiceBackground = m_wRoot.FindAnyWidget("VoiceChannelListBackground");
		Widget playerBackground = m_wRoot.FindAnyWidget("PlayerListBackground");
		if (darkBackground)
		{
			if (voiceBackground)
				voiceBackground.SetColor(Color.FromRGBA(2, 4, 4, 245));
			if (playerBackground)
				playerBackground.SetColor(Color.FromRGBA(2, 4, 4, 245));
		}
		else
		{
			if (voiceBackground)
				voiceBackground.SetColor(Color.FromRGBA(2, 3, 4, 184));
			if (playerBackground)
				playerBackground.SetColor(Color.FromRGBA(2, 3, 4, 184));
		}
	}

	protected void OnTabClicked(SCR_ButtonBaseComponent button)
	{
		SetPlayerListActive(button == m_PlayerListButton);
	}

	protected void OnDeafenClicked(SCR_ButtonBaseComponent button)
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController || !m_LobbyManager)
			return;

		int playerId = playerController.GetPlayerId();
		playerController.RequestVoiceDeafened(!m_LobbyManager.IsPlayerVoiceDeafened(playerId));
	}

	protected void OnVoiceStateUpdated(int playerId)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController || (playerId >= 0 && playerId != playerController.GetPlayerId()))
			return;

		RefreshDeafenVisuals();
	}

	protected void RefreshDeafenVisuals()
	{
		Widget hearingIcon = m_wRoot.FindAnyWidget("DeafenHearingIcon");
		Widget deafenedIcon = m_wRoot.FindAnyWidget("DeafenDisabledIcon");
		if (!hearingIcon || !deafenedIcon || !m_LobbyManager)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		bool deafened;
		if (playerController)
			deafened = m_LobbyManager.IsPlayerVoiceDeafened(playerController.GetPlayerId());

		hearingIcon.SetVisible(!deafened);
		deafenedIcon.SetVisible(deafened);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionTab(SCR_ButtonBaseComponent button)
	{
		SetPlayerListActive(false);
		m_wVoiceChannelList.SetVisible(false);
		SetTabVisuals("VoiceChannelsButtonBackground", "VoiceChannelsButtonLabel", "VoiceChannelsButtonAccent", false);
		m_FactionListButton.SetToggled(true, false, false);
		m_FactionList.SetActive(true);
	}

	protected void SetPlayerListActive(bool playerListActive)
	{
		if (m_FactionList)
		{
			m_FactionList.SetActive(false);
			m_FactionListButton.SetToggled(false, false, false);
		}
		m_wVoiceChannelList.SetVisible(!playerListActive);
		m_wPlayerList.SetVisible(playerListActive);

		SetTabVisuals("VoiceChannelsButtonBackground", "VoiceChannelsButtonLabel", "VoiceChannelsButtonAccent", !playerListActive);
		SetTabVisuals("PlayerListButtonBackground", "PlayerListButtonLabel", "PlayerListButtonAccent", playerListActive);

		if (playerListActive)
		{
			GC_PlayerListComponent playerList = GC_PlayerListComponent.Cast(m_wPlayerList.FindHandler(GC_PlayerListComponent));
			if (playerList)
				playerList.Refresh();
		}
	}

	protected void SetTabVisuals(string backgroundName, string labelName, string accentName, bool selected)
	{
		ImageWidget background = ImageWidget.Cast(m_wRoot.FindAnyWidget(backgroundName));
		if (background)
		{
			if (selected)
				background.SetColor(Color.FromRGBA(92, 112, 120, 242));
			else
				background.SetColor(Color.FromRGBA(18, 26, 28, 242));
		}

		TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget(labelName));
		if (label)
		{
			if (selected)
				label.SetColor(Color.FromRGBA(255, 255, 255, 255));
			else
				label.SetColor(Color.FromRGBA(209, 214, 214, 255));
		}

		Widget accent = m_wRoot.FindAnyWidget(accentName);
		if (accent)
			accent.SetVisible(selected);
	}
}
