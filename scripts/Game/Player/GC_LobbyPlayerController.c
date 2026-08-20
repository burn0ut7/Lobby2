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
	protected static const ResourceName SPECTATOR_CAMERA_PREFAB = "{D6DE32D1C0FCC1C7}Prefabs/Editor/Camera/ManualCameraBase.et";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;

	protected GC_ELobbyPlayerState m_eLobbyPlayerState;
	protected SCR_ManualCamera m_SpectatorCamera;
	protected MenuBase m_SpectatorMenu;
	protected ref GC_SpectatorMarkerOverlay m_SpectatorMarkerOverlay;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected int m_iEditorManagerAttachAttempts;
	protected bool m_bEditorOpen;
	//! Remains true while gameplay expects the local spectator presentation.
	//! Temporary possession suspends the presentation without losing this intent.
	protected bool m_bSpectatorPresentationRequested;

	void ~GC_LobbyPlayerController()
	{
		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);

		if (m_EditorManager)
		{
			m_EditorManager.GetOnOpened().Remove(OnEditorOpened);
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);
		}
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

	void RequestLobbyPhaseAdvance(GC_ELobbyPhase requestedPhase)
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
	protected void RpcAsk_RequestLobbyPhaseAdvance(GC_ELobbyPhase requestedPhase)
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
		m_SpectatorCamera = SCR_ManualCamera.Cast(cameraEntity);
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
		PrintFormat("GC | Spectator | manual camera entered: player=%1", GetPlayerId());
	}

	protected void DestroySpectatorCamera()
	{
		DestroySpectatorMarkerOverlay();
		DestroySpectatorMenu();

		if (!m_SpectatorCamera)
			return;

		SCR_ManualCamera camera = m_SpectatorCamera;
		m_SpectatorCamera = null;
		camera.Terminate();
		delete camera;
		PrintFormat("GC | Spectator | manual camera left: player=%1", GetPlayerId());
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
