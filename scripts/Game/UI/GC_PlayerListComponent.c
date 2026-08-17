//! Builds the reusable all-player list and applies the local name filter.
class GC_PlayerListComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName PLAYER_ROW_LAYOUT = "{6A33000000000001}UI/layouts/Players/Lobby_Player_Row.layout";

	protected Widget m_wPlayers;
	protected GC_LobbyManager m_LobbyManager;
	protected string m_sSearchText;
	protected ref array<Widget> m_aPlayerRows = {};

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

		m_LobbyManager.m_OnPlayerRegistryUpdated.Insert(OnPlayerRegistryUpdated);
		Refresh();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnPlayerRegistryUpdated);

		ClearPlayerRows();
		m_LobbyManager = null;
		m_wPlayers = null;
		super.HandlerDeattached(w);
	}

	void SetSearchText(string searchText)
	{
		if (m_sSearchText == searchText)
			return;

		m_sSearchText = searchText;
		Refresh();
	}

	void Refresh()
	{
		if (!m_wPlayers || !m_LobbyManager)
			return;

		ClearPlayerRows();

		array<int> connectedPlayers = {};
		PlayerManager playerManager = GetGame().GetPlayerManager();
		playerManager.GetPlayers(connectedPlayers);

		string searchNeedle = m_sSearchText;
		searchNeedle.ToLower();

		foreach (int playerId : connectedPlayers)
		{
			if (!playerManager.IsPlayerConnected(playerId))
				continue;

			string playerName = playerManager.GetPlayerName(playerId);
			if (!MatchesSearch(playerName, searchNeedle))
				continue;

			Widget rowWidget = GetGame().GetWorkspace().CreateWidgets(PLAYER_ROW_LAYOUT, m_wPlayers);
			if (!rowWidget)
				continue;

			GC_VoicePlayerRowComponent row = GC_VoicePlayerRowComponent.Cast(rowWidget.FindHandler(GC_VoicePlayerRowComponent));
			if (!row)
			{
				Print("GC | Player list row missing GC_VoicePlayerRowComponent", level: LogLevel.WARNING);
				rowWidget.RemoveFromHierarchy();
				continue;
			}

			row.SetPlayer(playerId, m_LobbyManager.GetSlotByPlayerId(playerId));
			m_aPlayerRows.Insert(rowWidget);
		}

	}

	protected bool MatchesSearch(string playerName, string searchNeedle)
	{
		if (searchNeedle.IsEmpty())
			return true;

		playerName.ToLower();
		return playerName.Contains(searchNeedle);
	}

	protected void OnPlayerRegistryUpdated(int playerId)
	{
		Refresh();
	}

	protected void ClearPlayerRows()
	{
		foreach (Widget rowWidget : m_aPlayerRows)
		{
			if (rowWidget)
				rowWidget.RemoveFromHierarchy();
		}

		m_aPlayerRows.Clear();

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
	protected SCR_ButtonBaseComponent m_VoiceChannelsButton;
	protected SCR_ButtonBaseComponent m_PlayerListButton;
	protected SCR_ButtonBaseComponent m_DeafenButton;
	protected GC_LobbyManager m_LobbyManager;
	protected Widget m_wVoiceChannelList;
	protected Widget m_wPlayerList;
	protected bool m_bInitialized;

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

		if (m_VoiceChannelsButton)
			m_VoiceChannelsButton.m_OnClicked.Remove(OnTabClicked);
		if (m_PlayerListButton)
			m_PlayerListButton.m_OnClicked.Remove(OnTabClicked);
		if (m_DeafenButton)
			m_DeafenButton.m_OnClicked.Remove(OnDeafenClicked);
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnVoiceStateUpdated);

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

		m_bInitialized = true;
		m_VoiceChannelsButton.m_OnClicked.Insert(OnTabClicked);
		m_PlayerListButton.m_OnClicked.Insert(OnTabClicked);
		m_DeafenButton.m_OnClicked.Insert(OnDeafenClicked);
		m_LobbyManager.m_OnPlayerRegistryUpdated.Insert(OnVoiceStateUpdated);
		SetPlayerListActive(false);
		RefreshDeafenVisuals();
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

	protected void SetPlayerListActive(bool playerListActive)
	{
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
