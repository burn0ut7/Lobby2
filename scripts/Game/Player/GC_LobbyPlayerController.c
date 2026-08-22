enum GC_ELobbyPlayerState
{
	SPECTATOR,
	PLAYING
}

class GC_LobbyPlayerControllerClass : SCR_PlayerControllerClass
{
}

//! Lobby2-only owning controller. The Lobby2 game-mode prefab selects a
//! dedicated player-controller prefab using this class.
class GC_LobbyPlayerController : SCR_PlayerController
{
	protected static const ResourceName SPECTATOR_CAMERA_PREFAB = "{0C428AB427BB44F5}Prefabs/Editor/Camera/GC_SpectatorCamera.et";
	protected static const ResourceName PAUSE_ACCESS_BUTTON_LAYOUT = "{6A4D00000000001A}UI/layouts/Briefing/Lobby_Briefing_Parameter_Option.layout";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;

	protected GC_ELobbyPlayerState m_eLobbyPlayerState;
	protected GC_SpectatorManualCamera m_SpectatorCamera;
	protected ref GC_SpectatorCameraContextAction m_ShoulderCameraAction;
	protected ref GC_SpectatorCameraContextAction m_EyeCameraAction;
	protected ref GC_SpectatorCameraContextAction m_FollowCameraAction;
	protected MenuBase m_SpectatorMenu;
	protected ref GC_SpectatorMarkerOverlay m_SpectatorMarkerOverlay;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected int m_iEditorManagerAttachAttempts;
	protected bool m_bEditorOpen;
	//! Remains true while gameplay expects the local spectator presentation.
	//! Temporary possession suspends the presentation without losing this intent.
	protected bool m_bSpectatorPresentationRequested;
	protected PauseMenuUI m_PauseMenu;
	protected Widget m_wPauseSlottingButton;
	protected Widget m_wPauseBriefingButton;
	protected SCR_ButtonTextComponent m_PauseSlottingButton;
	protected SCR_ButtonTextComponent m_PauseBriefingButton;

	void ~GC_LobbyPlayerController()
	{
		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);
		GetGame().GetCallqueue().Remove(OpenSlottingFromPause);
		GetGame().GetCallqueue().Remove(OpenBriefingFromPause);
		PauseMenuUI.m_OnPauseMenuOpened.Remove(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Remove(OnPauseMenuClosed);
		RemovePauseAccessButtons();
		DestroySpectatorCamera();

		if (m_EditorManager)
		{
			m_EditorManager.GetOnOpened().Remove(OnEditorOpened);
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);
		}
	}

	//! The owning client controller is the local UI lifecycle boundary. As soon
	//! as it loads, apply the menu that corresponds to the replicated game state.
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		PrintFormat("GC | Phase UI Trace | player controller initialized: this=%1 player=%2 server=%3 rplMode=%4 publishedController=%5 gameMode=%6 menuManager=%7", this, GetPlayerId(), Replication.IsServer(), RplSession.Mode(), GetGame().GetPlayerController(), GC_LobbyGameMode.GetGameMode(), GetGame().GetMenuManager());
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
		{
			PrintFormat("GC | Phase UI Trace | controller presentation stopped: player=%1 inPlayMode=%2 rplMode=%3", GetPlayerId(), GetGame().InPlayMode(), RplSession.Mode());
			return;
		}

		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		if (!gameMode)
		{
			PrintFormat("GC | Phase UI Trace | controller presentation stopped: player=%1 reason=no-lobby-game-mode", GetPlayerId(), level: LogLevel.ERROR);
			return;
		}

		PauseMenuUI.m_OnPauseMenuOpened.Insert(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Insert(OnPauseMenuClosed);
		gameMode.UpdateLocalPhasePresentation(this, "controller-init");
	}

	//! Adds Lobby2-owned access points to the active pause menu without
	//! modifying the shared pause-menu class or layout for unrelated modes.
	protected void OnPauseMenuOpened()
	{
		RemovePauseAccessButtons();

		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager || !GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.SLOTTING))
			return;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		m_PauseMenu = PauseMenuUI.Cast(menuManager.GetTopMenu());
		if (!m_PauseMenu)
			return;

		SCR_ButtonTextComponent continueButton = SCR_ButtonTextComponent.GetButtonText("Continue", m_PauseMenu.GetRootWidget());
		if (!continueButton || !continueButton.GetRootWidget())
			return;

		Widget buttonParent = continueButton.GetRootWidget().GetParent();
		if (!buttonParent)
			return;

		m_wPauseSlottingButton = GetGame().GetWorkspace().CreateWidgets(PAUSE_ACCESS_BUTTON_LAYOUT, buttonParent);
		if (m_wPauseSlottingButton)
		{
			m_PauseSlottingButton = SCR_ButtonTextComponent.Cast(m_wPauseSlottingButton.FindHandler(SCR_ButtonTextComponent));
			if (m_PauseSlottingButton)
			{
				m_PauseSlottingButton.SetText("SLOTTING");
				m_PauseSlottingButton.m_OnClicked.Insert(RequestOpenSlottingFromPause);
			}
		}

		Faction playerFaction = lobbyManager.GetPlayerFaction(GetPlayerId());
		if (!playerFaction || !GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.BRIEFING))
			return;

		m_wPauseBriefingButton = GetGame().GetWorkspace().CreateWidgets(PAUSE_ACCESS_BUTTON_LAYOUT, buttonParent);
		if (m_wPauseBriefingButton)
		{
			m_PauseBriefingButton = SCR_ButtonTextComponent.Cast(m_wPauseBriefingButton.FindHandler(SCR_ButtonTextComponent));
			if (m_PauseBriefingButton)
			{
				m_PauseBriefingButton.SetText("BRIEFING");
				m_PauseBriefingButton.m_OnClicked.Insert(RequestOpenBriefingFromPause);
			}
		}
	}

	protected void OnPauseMenuClosed()
	{
		RemovePauseAccessButtons();
		m_PauseMenu = null;
	}

	protected void RemovePauseAccessButtons()
	{
		if (m_PauseSlottingButton)
			m_PauseSlottingButton.m_OnClicked.Remove(RequestOpenSlottingFromPause);
		if (m_PauseBriefingButton)
			m_PauseBriefingButton.m_OnClicked.Remove(RequestOpenBriefingFromPause);
		if (m_wPauseSlottingButton)
			m_wPauseSlottingButton.RemoveFromHierarchy();
		if (m_wPauseBriefingButton)
			m_wPauseBriefingButton.RemoveFromHierarchy();

		m_PauseSlottingButton = null;
		m_PauseBriefingButton = null;
		m_wPauseSlottingButton = null;
		m_wPauseBriefingButton = null;
	}

	protected void RequestOpenSlottingFromPause()
	{
		if (m_PauseMenu)
			m_PauseMenu.Close();
		GetGame().GetCallqueue().CallLater(OpenSlottingFromPause, 0);
	}

	protected void RequestOpenBriefingFromPause()
	{
		if (m_PauseMenu)
			m_PauseMenu.Close();
		GetGame().GetCallqueue().CallLater(OpenBriefingFromPause, 0);
	}

	protected void OpenSlottingFromPause()
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager || !GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.SLOTTING))
			return;

		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_BriefingMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
	}

	protected void OpenBriefingFromPause()
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager || !GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.BRIEFING) || !lobbyManager.GetPlayerFaction(GetPlayerId()))
			return;

		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_SlottingMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_BriefingMenu, 0, true);
	}

	override protected void OnOwnershipChanged(bool changing, bool becameOwner)
	{
		super.OnOwnershipChanged(changing, becameOwner);
		PrintFormat("GC | Phase UI Trace | player controller ownership: this=%1 player=%2 changing=%3 becameOwner=%4 server=%5 publishedController=%6", this, GetPlayerId(), changing, becameOwner, Replication.IsServer(), GetGame().GetPlayerController());
	}

	void RequestLobbySlot(int slotId)
	{
		if (slotId < 0)
			return;

		Rpc(RpcAsk_RequestLobbySlot, slotId);
	}

	void RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		Rpc(RpcAsk_RequestVoiceChannel, channelType, factionKey, groupRplId, personalOwnerIdentity);
	}

	void RequestVoiceDeafened(bool deafened)
	{
		Rpc(RpcAsk_RequestVoiceDeafened, deafened);
	}

	void RequestLobbyPhaseAdvance(SCR_EGameModeState requestedPhase)
	{
		PrintFormat("GC | Phase RPC | request queued: player=%1 target=%2 value=%3", GetPlayerId(), GC_LobbyManager.GetPhaseName(requestedPhase), requestedPhase);
		Rpc(RpcAsk_RequestLobbyPhaseAdvance, requestedPhase);
	}

	//! Sends one complete local parameter draft. The server derives the player
	//! identity from this owning controller and never trusts a client id.
	void RequestApplyScenarioParameters(int baseRevision, array<ref GC_LobbyParameterValue> values)
	{
		if (!values)
			return;

		PrintFormat("GC | Parameters RPC | request queued: player=%1 revision=%2 values=%3", GetPlayerId(), baseRevision, values.Count());
		Rpc(RpcAsk_RequestScenarioParameters, baseRevision, values);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbySlot(int slotId)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TryAssignPlayerToSlot(GetPlayerId(), slotId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey, RplId groupRplId, string personalOwnerIdentity)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TrySetPlayerVoiceChannel(GetPlayerId(), channelType, factionKey, groupRplId, personalOwnerIdentity);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVoiceDeafened(bool deafened)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TrySetPlayerVoiceDeafened(GetPlayerId(), deafened);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLobbyPhaseAdvance(SCR_EGameModeState requestedPhase)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TryAdvanceLobbyPhase(GetPlayerId(), requestedPhase);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestScenarioParameters(int baseRevision, array<ref GC_LobbyParameterValue> values)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			lobbyManager.TryApplyScenarioParameters(GetPlayerId(), baseRevision, values);
	}

	bool SetLobbySpectator(IEntity entity, bool activateCamera)
	{
		if (!SetLobbyControlledEntity(entity, true))
			return false;

		if (activateCamera)
			Rpc(RpcDo_EnterLobbySpectatorMode);
		else
			Rpc(RpcDo_LeaveLobbySpectatorMode, true);

		return true;
	}

	//! Assigns a Lobby2-owned main entity through the base controller lifecycle.
	//! This is authority-only and intentionally avoids temporary possession when
	//! the player has no pre-existing main entity.
	bool SetLobbyControlledEntity(IEntity entity, bool spectator)
	{
		if (!entity)
			return false;

		SetInitialMainEntity(entity);
		if (GetControlledEntity() != entity)
		{
			PrintFormat("GC | Lifecycle | controller assignment failed: player=%1 entity=%2", GetPlayerId(), entity, level: LogLevel.WARNING);
			return false;
		}

		if (spectator)
			SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		else
		{
			SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);
			Rpc(RpcDo_LeaveLobbySpectatorMode, false);
		}

		return true;
	}

	GC_ELobbyPlayerState GetLobbyPlayerState()
	{
		return m_eLobbyPlayerState;
	}

	void SetLobbySpectatorState(bool activateCamera)
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		if (activateCamera)
			Rpc(RpcDo_EnterLobbySpectatorMode);
		else
			Rpc(RpcDo_LeaveLobbySpectatorMode, true);
	}

	void SetLobbyPlayingState()
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);
		Rpc(RpcDo_LeaveLobbySpectatorMode, false);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_EnterLobbySpectatorMode()
	{
		SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		m_bSpectatorPresentationRequested = true;
		m_iEditorManagerAttachAttempts = 0;
		AttachEditorManager();
		ReconcileSpectatorPresentation();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_LeaveLobbySpectatorMode(bool remainsSpectator)
	{
		m_bSpectatorPresentationRequested = false;

		if (remainsSpectator)
			SetLobbyPlayerState(GC_ELobbyPlayerState.SPECTATOR);
		else
			SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);

		DestroySpectatorCamera();
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		// Camera and menu presentation exists only on the owning client. The same
		// controller class also exists on dedicated servers and remote proxies.
		if (GetGame().GetPlayerController() != this)
			return;

		AttachEditorManager();
		ReconcileSpectatorPresentation();
	}

	protected void AttachEditorManager()
	{
		if (m_EditorManager || GetGame().GetPlayerController() != this)
			return;

		m_EditorManager = SCR_EditorManagerEntity.GetInstance();
		if (m_EditorManager)
		{
			GetGame().GetCallqueue().Remove(AttachEditorManager);
			m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);
			if (m_EditorManager.IsOpened())
				OnEditorOpened();
			return;
		}

		m_iEditorManagerAttachAttempts++;
		if (m_iEditorManagerAttachAttempts < EDITOR_MANAGER_ATTACH_RETRIES)
		{
			GetGame().GetCallqueue().Remove(AttachEditorManager);
			GetGame().GetCallqueue().CallLater(AttachEditorManager, 250);
		}
	}

	protected void OnEditorOpened()
	{
		m_bEditorOpen = true;
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);
		if (m_SpectatorCamera || m_SpectatorMenu || m_SpectatorMarkerOverlay)
		{
			DestroySpectatorCamera();
			PrintFormat("GC | Spectator | presentation suspended while Game Master is open: player=%1", GetPlayerId());
		}
	}

	protected void OnEditorClosed()
	{
		m_bEditorOpen = false;
		// The editor camera and temporary possession can settle at the end of the
		// close callback. Reconcile on the following frame against the controller's
		// authoritative current entity and the retained spectator presentation intent.
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);
		GetGame().GetCallqueue().CallLater(ReconcileSpectatorPresentation, 0);
	}

	protected void ReconcileSpectatorPresentation()
	{
		if (!m_bSpectatorPresentationRequested)
		{
			DestroySpectatorCamera();
			return;
		}

		if (m_bEditorOpen || (m_EditorManager && m_EditorManager.IsOpened()))
		{
			DestroySpectatorCamera();
			return;
		}

		IEntity controlledEntity = GetControlledEntity();
		if (controlledEntity && controlledEntity.FindComponent(GC_LobbySpectatorBodyComponent))
		{
			CreateSpectatorCamera();
			return;
		}

		if (m_SpectatorCamera || m_SpectatorMenu || m_SpectatorMarkerOverlay)
		{
			DestroySpectatorCamera();
			PrintFormat("GC | Spectator | presentation suspended after control transfer: player=%1", GetPlayerId());
		}
	}

	protected void CreateSpectatorCamera()
	{
		if (m_bEditorOpen || (m_EditorManager && m_EditorManager.IsOpened()))
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (m_SpectatorCamera)
		{
			if (cameraManager)
				cameraManager.SetCamera(m_SpectatorCamera);
			CreateSpectatorMenu();
			CreateSpectatorMarkerOverlay();
			return;
		}

		Resource cameraResource = Resource.Load(SPECTATOR_CAMERA_PREFAB);
		if (!cameraResource)
		{
			PrintFormat("GC | Spectator | manual camera prefab unavailable: %1", SPECTATOR_CAMERA_PREFAB, level: LogLevel.ERROR);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		IEntity controlledEntity = GetControlledEntity();
		if (controlledEntity)
			controlledEntity.GetWorldTransform(spawnParams.Transform);

		IEntity cameraEntity = GetGame().SpawnEntityPrefabLocal(cameraResource, GetWorld(), spawnParams);
		m_SpectatorCamera = GC_SpectatorManualCamera.Cast(cameraEntity);
		if (!m_SpectatorCamera)
		{
			PrintFormat("GC | Spectator | manual camera spawn failed: player=%1", GetPlayerId(), level: LogLevel.ERROR);
			if (cameraEntity)
				delete cameraEntity;
			return;
		}

		if (!cameraManager)
			cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
		{
			PrintFormat("GC | Spectator | camera manager unavailable: player=%1", GetPlayerId(), level: LogLevel.ERROR);
			DestroySpectatorCamera();
			return;
		}

		cameraManager.SetCamera(m_SpectatorCamera);
		if (cameraManager.CurrentCamera() != m_SpectatorCamera)
		{
			PrintFormat("GC | Spectator | manual camera activation failed: player=%1", GetPlayerId(), level: LogLevel.ERROR);
			DestroySpectatorCamera();
			return;
		}

		CreateSpectatorMenu();
		CreateSpectatorMarkerOverlay();
		RegisterSpectatorCameraActions();
		PrintFormat("GC | Spectator | manual camera entered: player=%1", GetPlayerId());
	}

	protected void DestroySpectatorCamera()
	{
		UnregisterSpectatorCameraActions();
		DestroySpectatorMarkerOverlay();
		DestroySpectatorMenu();
		if (!m_SpectatorCamera)
			return;

		GC_SpectatorManualCamera camera = m_SpectatorCamera;
		m_SpectatorCamera = null;
		camera.SetFreeMode();
		camera.Terminate();
		delete camera;
		PrintFormat("GC | Spectator | manual camera left: player=%1", GetPlayerId());
	}

	protected void RegisterSpectatorCameraActions()
	{
		if (!m_SpectatorCamera)
			return;

		if (!m_ShoulderCameraAction)
		{
			m_ShoulderCameraAction = new GC_SpectatorCameraContextAction(m_SpectatorCamera, GC_ESpectatorCameraMode.SHOULDER, "SHOULDER CAMERA");
			GC_ContextMenuActionRegistry.Register(m_ShoulderCameraAction);
		}

		if (!m_EyeCameraAction)
		{
			m_EyeCameraAction = new GC_SpectatorCameraContextAction(m_SpectatorCamera, GC_ESpectatorCameraMode.EYE, "EYE CAMERA");
			GC_ContextMenuActionRegistry.Register(m_EyeCameraAction);
		}

		if (!m_FollowCameraAction)
		{
			m_FollowCameraAction = new GC_SpectatorCameraContextAction(m_SpectatorCamera, GC_ESpectatorCameraMode.FOLLOW, "FOLLOW CAMERA");
			GC_ContextMenuActionRegistry.Register(m_FollowCameraAction);
		}
	}

	protected void UnregisterSpectatorCameraActions()
	{
		if (m_ShoulderCameraAction)
		{
			GC_ContextMenuActionRegistry.Unregister(m_ShoulderCameraAction);
			m_ShoulderCameraAction = null;
		}

		if (m_EyeCameraAction)
		{
			GC_ContextMenuActionRegistry.Unregister(m_EyeCameraAction);
			m_EyeCameraAction = null;
		}

		if (m_FollowCameraAction)
		{
			GC_ContextMenuActionRegistry.Unregister(m_FollowCameraAction);
			m_FollowCameraAction = null;
		}
	}

	protected void CreateSpectatorMarkerOverlay()
	{
		if (!m_SpectatorCamera)
			return;

		if (!m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay = new GC_SpectatorMarkerOverlay();

		if (!m_SpectatorMarkerOverlay.Start())
		{
			PrintFormat("GC | Spectator | marker overlay creation failed: player=%1", GetPlayerId(), level: LogLevel.WARNING);
			m_SpectatorMarkerOverlay = null;
		}
	}

	protected void CreateSpectatorMenu()
	{
		if (m_SpectatorMenu)
			return;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		m_SpectatorMenu = menuManager.OpenMenu(ChimeraMenuPreset.GC_Lobby_SpectatorMenu, 0, true);
		if (!m_SpectatorMenu)
			PrintFormat("GC | Spectator | spectator HUD creation failed: player=%1", GetPlayerId(), level: LogLevel.WARNING);
	}

	protected void DestroySpectatorMenu()
	{
		if (!m_SpectatorMenu)
			return;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager)
			menuManager.CloseMenu(m_SpectatorMenu);

		m_SpectatorMenu = null;
	}

	protected void DestroySpectatorMarkerOverlay()
	{
		if (!m_SpectatorMarkerOverlay)
			return;

		m_SpectatorMarkerOverlay.Destroy();
		m_SpectatorMarkerOverlay = null;
	}

	protected void SetLobbyPlayerState(GC_ELobbyPlayerState state)
	{
		if (m_eLobbyPlayerState == state)
			return;

		m_eLobbyPlayerState = state;
	}
}
