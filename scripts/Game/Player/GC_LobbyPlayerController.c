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
	//! The eight-argument RPC ceiling leaves one integer for both sight indexes.
	//! Each index is offset by one so zero represents the no-sight/no-zoom state.
	static const int NETWORK_CAMERA_SIGHT_STATE_SCALE = 65536;
	protected static const ResourceName SPECTATOR_CAMERA_PREFAB = "{0C428AB427BB44F5}Prefabs/Editor/Camera/GC_SpectatorCamera.et";
	protected static const ResourceName PAUSE_ACCESS_BUTTON_LAYOUT = "{6A4D00000000001A}UI/layouts/Briefing/Lobby_Briefing_Parameter_Option.layout";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;
	[Attribute("30", desc: "Networked first-person camera snapshots per second (1-60).")]
	protected int m_iNetworkCameraUpdateRate;
	[Attribute("0.2", desc: "Seconds of per-frame projectile path retained by the spectator tracer.")]
	protected float m_fProjectileTraceDuration;

	protected GC_ELobbyPlayerState m_eLobbyPlayerState;
	protected GC_SpectatorManualCamera m_SpectatorCamera;
	protected ref GC_SpectatorCameraContextAction m_ShoulderCameraAction;
	protected ref GC_SpectatorCameraContextAction m_EyeCameraAction;
	protected ref GC_SpectatorCameraContextAction m_FollowCameraAction;
	protected ref GC_SpectatorCameraContextAction m_MirrorCameraAction;
	protected ref GC_SpectatorCameraContextAction m_NetworkCameraAction;
	protected int m_iServerNetworkCameraTargetPlayerId = -1;
	protected int m_iServerNetworkCameraSpectatorPlayerId = -1;
	protected ref array<int> m_aNetworkCameraSpectators = new array<int>();
	protected int m_iNetworkCameraSequence;
	protected bool m_bNetworkCameraDiagnosticsInitialized;
	protected bool m_bNetworkCameraDiagnosticsADS;
	protected int m_iNetworkCameraDiagnosticsSightState;
	protected MenuBase m_SpectatorMenu;
	protected ref GC_SpectatorMarkerOverlay m_SpectatorMarkerOverlay;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected int m_iEditorManagerAttachAttempts;
	protected bool m_bEditorOpen;
	//! Remains true while gameplay expects the local spectator presentation.
	//! Temporary possession suspends the presentation without losing this intent.
	protected bool m_bSpectatorPresentationRequested;
	protected bool m_bSpectatorInterfaceVisible = true;
	//! Client-local spectator night-mode preference. Uses the stock Game Master EV behavior.
	protected bool m_bSpectatorNightModeEnabled = false;
	//! Client-local spectator presentation preference. Defaults to disabled.
	protected bool m_bSpectatorProjectileTracersEnabled = false;
	//! Client-local killed-from presentation preference. Defaults to disabled.
	protected bool m_bSpectatorKilledFromEnabled = false;
	protected ref GC_KilledFromSnapshot m_KilledFromSnapshot;
	protected ref GC_KilledFromDamageTracker m_LocalKilledFromTracker;
	protected PauseMenuUI m_PauseMenu;
	protected Widget m_wPauseSlottingButton;
	protected Widget m_wPauseBriefingButton;
	protected SCR_ButtonTextComponent m_PauseSlottingButton;
	protected SCR_ButtonTextComponent m_PauseBriefingButton;
	protected InputManager m_LobbyInputManager;

	void ~GC_LobbyPlayerController()
	{
		StopLocalKilledFromTracking(false);
		StopServerNetworkCameraStream();
		GetGame().GetCallqueue().Remove(PublishNetworkCameraTransform);
		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);
		GetGame().GetCallqueue().Remove(OpenSlottingFromPause);
		GetGame().GetCallqueue().Remove(OpenBriefingFromPause);
		PauseMenuUI.m_OnPauseMenuOpened.Remove(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Remove(OnPauseMenuClosed);
		if (m_LobbyInputManager)
			m_LobbyInputManager.RemoveActionListener("GC_OpenLobby", EActionTrigger.DOWN, OpenSlottingFromPause);
		m_LobbyInputManager = null;
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
		m_LobbyInputManager = GetGame().GetInputManager();
		if (m_LobbyInputManager)
			m_LobbyInputManager.AddActionListener("GC_OpenLobby", EActionTrigger.DOWN, OpenSlottingFromPause);
		gameMode.UpdateLocalPhasePresentation(this, "controller-init");
		StartLocalKilledFromTracking(GetControlledEntity());
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

	int GetNetworkCameraUpdateRate()
	{
		if (m_iNetworkCameraUpdateRate < 1)
			return 1;
		if (m_iNetworkCameraUpdateRate > 60)
			return 60;

		return m_iNetworkCameraUpdateRate;
	}

	void RequestNetworkCameraStream(int targetPlayerId)
	{
		if (targetPlayerId <= 0)
			return;

		Rpc(RpcAsk_SetNetworkCameraTarget, targetPlayerId);
	}

	void StopNetworkCameraStream()
	{
		Rpc(RpcAsk_SetNetworkCameraTarget, -1);
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

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetNetworkCameraTarget(int targetPlayerId)
	{
		if (targetPlayerId == m_iServerNetworkCameraTargetPlayerId)
			return;

		StopServerNetworkCameraStream();
		if (targetPlayerId <= 0 || m_eLobbyPlayerState != GC_ELobbyPlayerState.SPECTATOR)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		IEntity targetEntity = playerManager.GetPlayerControlledEntity(targetPlayerId);
		GC_LobbyPlayerController targetController = GC_LobbyPlayerController.Cast(playerManager.GetPlayerController(targetPlayerId));
		if (!SCR_ChimeraCharacter.Cast(targetEntity) || targetEntity.FindComponent(GC_LobbySpectatorBodyComponent) || !targetController || targetController.GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
			return;

		m_iServerNetworkCameraTargetPlayerId = targetPlayerId;
		m_iServerNetworkCameraSpectatorPlayerId = GetPlayerId();
		targetController.ServerSetNetworkCameraSpectator(m_iServerNetworkCameraSpectatorPlayerId, true);
		PrintFormat("GC | Spectator Camera RPC | stream subscribed: spectator=%1 target=%2 rate=%3", m_iServerNetworkCameraSpectatorPlayerId, targetPlayerId, targetController.GetNetworkCameraUpdateRate());
	}

	protected void StopServerNetworkCameraStream()
	{
		if (m_iServerNetworkCameraTargetPlayerId <= 0)
			return;

		int targetPlayerId = m_iServerNetworkCameraTargetPlayerId;
		int spectatorPlayerId = m_iServerNetworkCameraSpectatorPlayerId;
		m_iServerNetworkCameraTargetPlayerId = -1;
		m_iServerNetworkCameraSpectatorPlayerId = -1;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		GC_LobbyPlayerController targetController;
		if (playerManager)
			targetController = GC_LobbyPlayerController.Cast(playerManager.GetPlayerController(targetPlayerId));
		if (targetController)
			targetController.ServerSetNetworkCameraSpectator(spectatorPlayerId, false);

		PrintFormat("GC | Spectator Camera RPC | stream unsubscribed: spectator=%1 target=%2", spectatorPlayerId, targetPlayerId);
	}

	protected void ServerSetNetworkCameraSpectator(int spectatorPlayerId, bool enabled)
	{
		Rpc(RpcDo_SetNetworkCameraSpectator, spectatorPlayerId, enabled);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetNetworkCameraSpectator(int spectatorPlayerId, bool enabled)
	{
		if (GetGame().GetPlayerController() != this)
			return;

		int spectatorIndex = m_aNetworkCameraSpectators.Find(spectatorPlayerId);
		if (enabled)
		{
			if (spectatorIndex == -1)
				m_aNetworkCameraSpectators.Insert(spectatorPlayerId);

			m_bNetworkCameraDiagnosticsInitialized = false;
		}
		else if (spectatorIndex != -1)
		{
			m_aNetworkCameraSpectators.Remove(spectatorIndex);
		}

		GetGame().GetCallqueue().Remove(PublishNetworkCameraTransform);
		if (m_aNetworkCameraSpectators.IsEmpty())
			return;

		PublishNetworkCameraTransform();
		GetGame().GetCallqueue().CallLater(PublishNetworkCameraTransform, 1000 / GetNetworkCameraUpdateRate(), true);
	}

	protected void PublishNetworkCameraTransform()
	{
		if (GetGame().GetPlayerController() != this || m_aNetworkCameraSpectators.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(PublishNetworkCameraTransform);
			return;
		}

		CameraManager cameraManager = GetGame().GetCameraManager();
		CameraBase currentCamera;
		if (cameraManager)
			currentCamera = cameraManager.CurrentCamera();
		if (!currentCamera)
			return;

		IEntity controlledEntity = GetControlledEntity();
		if (!controlledEntity)
			return;

		vector cameraTransform[4];
		currentCamera.GetWorldTransform(cameraTransform);
		vector snapshotPosition = cameraTransform[3] - controlledEntity.GetWorldTransformAxis(3);
		vector snapshotForward = cameraTransform[2];
		vector snapshotUp = cameraTransform[1];
		// X is main-camera FOV, Y is optional PiP FOV and Z is the selected
		// sight's actual FOV. Every camera transform uses world-aligned axes
		// relative to the character origin.
		vector lensState = Vector(currentCamera.GetVerticalFOV(), -1, -1);
		bool isADS;
		int sightState;
		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(controlledEntity.FindComponent(BaseWeaponManagerComponent));
		BaseWeaponComponent weapon;
		if (weaponManager)
			weapon = weaponManager.GetCurrentWeapon();

		if (weapon && weapon.IsSightADSActive())
		{
			isADS = true;
			int sightsIndex = weaponManager.GetCurrentSightsIndex();
			int zoomIndex = -1;
			BaseSightsComponent sights = weapon.GetSightsAt(sightsIndex);
			if (sights)
			{
				SightsFOVInfo fovInfo = sights.GetFOVInfo();
				if (fovInfo)
					lensState[2] = fovInfo.GetFOV();

				SCR_BaseVariableSightsFOVInfo variableFOV = SCR_BaseVariableSightsFOVInfo.Cast(fovInfo);
				if (variableFOV)
					zoomIndex = variableFOV.GetCurrentIndex();

				SCR_2DPIPSightsComponent pipSights = SCR_2DPIPSightsComponent.Cast(sights);
				CameraBase overlayCamera = cameraManager.GetOverlayCamera();
				if (pipSights && overlayCamera && overlayCamera == pipSights.GetPIPCamera())
					lensState[1] = overlayCamera.GetVerticalFOV();
			}

			sightState = (sightsIndex + 1) * NETWORK_CAMERA_SIGHT_STATE_SCALE + zoomIndex + 1;
		}
		m_iNetworkCameraSequence++;
		bool diagnosticStateChanged = !m_bNetworkCameraDiagnosticsInitialized || m_bNetworkCameraDiagnosticsADS != isADS || m_iNetworkCameraDiagnosticsSightState != sightState;
		if (diagnosticStateChanged || isADS && m_iNetworkCameraSequence % GC_SpectatorCameraDiagnostics.SAMPLE_INTERVAL == 0)
			GC_SpectatorCameraDiagnostics.LogSnapshot("owner", m_iNetworkCameraSequence, controlledEntity, cameraTransform, snapshotPosition, lensState[0], lensState[1], lensState[2], isADS, sightState);

		m_bNetworkCameraDiagnosticsInitialized = true;
		m_bNetworkCameraDiagnosticsADS = isADS;
		m_iNetworkCameraDiagnosticsSightState = sightState;
		foreach (int spectatorPlayerId : m_aNetworkCameraSpectators)
			Rpc(RpcAsk_PublishNetworkCameraTransform, spectatorPlayerId, m_iNetworkCameraSequence, snapshotPosition, snapshotForward, snapshotUp, lensState, isADS, sightState);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RpcAsk_PublishNetworkCameraTransform(int spectatorPlayerId, int sequence, vector position, vector forward, vector up, vector lensState, bool isADS, int sightState)
	{
		if (m_eLobbyPlayerState != GC_ELobbyPlayerState.PLAYING)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		GC_LobbyPlayerController spectatorController;
		if (playerManager)
			spectatorController = GC_LobbyPlayerController.Cast(playerManager.GetPlayerController(spectatorPlayerId));
		if (!spectatorController || spectatorController.m_iServerNetworkCameraTargetPlayerId != GetPlayerId() || spectatorController.GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return;

		spectatorController.ServerSendNetworkCameraTransform(GetPlayerId(), sequence, position, forward, up, lensState, isADS, sightState);
	}

	protected void ServerSendNetworkCameraTransform(int targetPlayerId, int sequence, vector position, vector forward, vector up, vector lensState, bool isADS, int sightState)
	{
		Rpc(RpcDo_ReceiveNetworkCameraTransform, targetPlayerId, sequence, position, forward, up, lensState, isADS, sightState);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveNetworkCameraTransform(int targetPlayerId, int sequence, vector position, vector forward, vector up, vector lensState, bool isADS, int sightState)
	{
		if (GetGame().GetPlayerController() != this || !m_SpectatorCamera)
			return;

		m_SpectatorCamera.ReceiveNetworkCameraTransform(targetPlayerId, sequence, position, forward, up, lensState, isADS, sightState);
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

	//! Authority-to-owner publication of the latest fatal damage snapshot.
	void SendKilledFromSnapshot(GC_KilledFromSnapshot snapshot)
	{
		if (!Replication.IsServer() || !snapshot)
		{
			PrintFormat("GC | Killed From | event=snapshot-send result=rejected server=%1 player=%2 snapshot=%3", Replication.IsServer(), GetPlayerId(), snapshot, level: LogLevel.WARNING);
			return;
		}

		ResourceName killerVehiclePrefab;
		vector killerVehiclePosition;
		vector killerVehicleAngles;
		ECompartmentType killerCompartmentType;
		int killerCompartmentIndex = -1;
		GC_KilledFromVehicleState killerVehicle = snapshot.GetKillerVehicle();
		if (killerVehicle && killerVehicle.IsValid())
		{
			killerVehiclePrefab = killerVehicle.GetPrefab();
			killerVehiclePosition = killerVehicle.GetPosition();
			killerVehicleAngles = killerVehicle.GetAngles();
			killerCompartmentType = killerVehicle.GetCompartmentType();
			killerCompartmentIndex = killerVehicle.GetCompartmentIndex();
		}
		PrintFormat("GC | Killed From | event=snapshot-send result=rpc-owner player=%1 snapshot={%2}", GetPlayerId(), snapshot.GetDebugReport());
		Rpc(RpcDo_SetKilledFromSnapshot, snapshot.GetKillerPrefab(), snapshot.GetKillerPosition(), snapshot.GetKillerAngles(), snapshot.GetLineOrigin(), snapshot.GetHitPosition(), snapshot.GetStance(), snapshot.GetDynamicStance(), snapshot.GetFlags());
		Rpc(RpcDo_SetKilledFromIdentity, snapshot.GetKillerDisplayName(), snapshot.IsKillerDisplayNameLocalized());
		if (killerVehiclePrefab != ResourceName.Empty)
			Rpc(RpcDo_SetKilledFromVehicle, killerVehiclePrefab, killerVehiclePosition, killerVehicleAngles, killerCompartmentType, killerCompartmentIndex);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetKilledFromSnapshot(ResourceName killerPrefab, vector killerPosition, vector killerAngles, vector lineOrigin, vector hitPosition, int stance, float dynamicStance, int flags)
	{
		GC_KilledFromSnapshot serverFallback = new GC_KilledFromSnapshot();
		serverFallback.SetNetworkData(killerPrefab, killerPosition, killerAngles, lineOrigin, hitPosition, stance, dynamicStance, flags);
		PrintFormat("GC | Killed From | event=snapshot-receive player=%1 serverFallback={%2} localTracker=%3 retainedLocal=%4", GetPlayerId(), serverFallback.GetDebugReport(), m_LocalKilledFromTracker != null, m_KilledFromSnapshot != null);
		if (m_LocalKilledFromTracker)
			serverFallback.MergeLocalDamage(m_LocalKilledFromTracker.GetSnapshot());
		else
			serverFallback.MergeLocalDamage(m_KilledFromSnapshot);
		m_KilledFromSnapshot = serverFallback;
		PrintFormat("GC | Killed From | event=snapshot-merge player=%1 final={%2} overlayReady=%3 enabled=%4", GetPlayerId(), m_KilledFromSnapshot.GetDebugReport(), m_SpectatorMarkerOverlay != null, m_bSpectatorKilledFromEnabled);
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetKilledFromSnapshot(m_KilledFromSnapshot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetKilledFromIdentity(string killerDisplayName, bool killerDisplayNameLocalized)
	{
		if (!m_KilledFromSnapshot)
		{
			PrintFormat("GC | Killed From | event=identity-fallback-receive result=rejected reason=no-snapshot player=%1", GetPlayerId(), level: LogLevel.WARNING);
			return;
		}

		bool hadLocalIdentity = !m_KilledFromSnapshot.GetKillerDisplayName().IsEmpty();
		m_KilledFromSnapshot.SetNetworkKillerDisplayName(killerDisplayName, killerDisplayNameLocalized);
		string result = "applied";
		if (hadLocalIdentity)
			result = "ignored-local-retained";
		PrintFormat("GC | Killed From | event=identity-fallback-receive result=%1 player=%2 name=%3 localized=%4", result, GetPlayerId(), m_KilledFromSnapshot.GetKillerDisplayName(), m_KilledFromSnapshot.IsKillerDisplayNameLocalized());
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetKilledFromSnapshot(m_KilledFromSnapshot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetKilledFromVehicle(ResourceName prefab, vector position, vector angles, ECompartmentType compartmentType, int compartmentIndex)
	{
		if (!m_KilledFromSnapshot)
		{
			PrintFormat("GC | Killed From | event=vehicle-fallback-receive result=rejected reason=no-snapshot player=%1 prefab=%2", GetPlayerId(), prefab, level: LogLevel.WARNING);
			return;
		}
		if (m_KilledFromSnapshot.HasKillerVehicle())
		{
			PrintFormat("GC | Killed From | event=vehicle-fallback-receive result=ignored reason=local-vehicle-retained player=%1 prefab=%2", GetPlayerId(), prefab);
			return;
		}
		m_KilledFromSnapshot.SetNetworkKillerVehicleData(prefab, position, angles, compartmentType, compartmentIndex);
		PrintFormat("GC | Killed From | event=vehicle-fallback-receive result=applied player=%1 snapshot={%2}", GetPlayerId(), m_KilledFromSnapshot.GetDebugReport());
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetKilledFromSnapshot(m_KilledFromSnapshot);
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
		{
			SetLobbyPlayerState(GC_ELobbyPlayerState.PLAYING);
			m_KilledFromSnapshot = null;
		}

		DestroySpectatorCamera();
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		StopLocalKilledFromTracking(true);
		super.OnControlledEntityChanged(from, to);

		// Camera and menu presentation exists only on the owning client. The same
		// controller class also exists on dedicated servers and remote proxies.
		if (GetGame().GetPlayerController() != this)
			return;

		StartLocalKilledFromTracking(to);

		AttachEditorManager();
		ReconcileSpectatorPresentation();
	}

	protected void StartLocalKilledFromTracking(IEntity entity)
	{
		if (m_LocalKilledFromTracker)
		{
			PrintFormat("GC | Killed From | event=tracker-request result=ignored reason=already-tracking player=%1 entity=%2", GetPlayerId(), entity);
			return;
		}
		if (GetGame().GetPlayerController() != this)
			return;
		if (!entity)
		{
			PrintFormat("GC | Killed From | event=tracker-request result=ignored reason=no-controlled-entity player=%1", GetPlayerId());
			return;
		}
		if (entity.FindComponent(GC_LobbySpectatorBodyComponent))
		{
			PrintFormat("GC | Killed From | event=tracker-request result=ignored reason=spectator-body player=%1 entity=%2 retainedSnapshot=%3", GetPlayerId(), entity, m_KilledFromSnapshot != null);
			return;
		}

		m_KilledFromSnapshot = null;
		m_LocalKilledFromTracker = new GC_KilledFromDamageTracker();
		if (!m_LocalKilledFromTracker.Start(entity))
		{
			m_LocalKilledFromTracker = null;
			PrintFormat("GC | Killed From | event=tracker-request result=failed player=%1 entity=%2", GetPlayerId(), entity, level: LogLevel.WARNING);
		}
	}

	protected void StopLocalKilledFromTracking(bool retainSnapshot)
	{
		if (!m_LocalKilledFromTracker)
			return;

		PrintFormat("GC | Killed From | event=tracker-retain player=%1 retain=%2 existingSnapshot=%3", GetPlayerId(), retainSnapshot, m_KilledFromSnapshot != null);

		if (retainSnapshot)
		{
			m_LocalKilledFromTracker.CaptureTerminalVictim();
			GC_KilledFromSnapshot localDamage = m_LocalKilledFromTracker.GetSnapshot();
			if (localDamage)
			{
				if (m_KilledFromSnapshot)
					m_KilledFromSnapshot.MergeLocalDamage(localDamage);
				else
					m_KilledFromSnapshot = localDamage;
			}
		}
		if (m_KilledFromSnapshot)
			PrintFormat("GC | Killed From | event=tracker-retain result=complete player=%1 snapshot={%2}", GetPlayerId(), m_KilledFromSnapshot.GetDebugReport());
		else
			PrintFormat("GC | Killed From | event=tracker-retain result=complete player=%1 snapshot=none", GetPlayerId());

		m_LocalKilledFromTracker.Destroy();
		m_LocalKilledFromTracker = null;
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
			SetBaseHUDVisible(m_bSpectatorInterfaceVisible);
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

		if (m_bSpectatorNightModeEnabled && !m_SpectatorCamera.SetSpectatorNightModeEnabled(true))
			m_bSpectatorNightModeEnabled = false;

		SetBaseHUDVisible(m_bSpectatorInterfaceVisible);
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
		SetBaseHUDVisible(true);
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

		if (!m_MirrorCameraAction)
		{
			m_MirrorCameraAction = new GC_SpectatorCameraContextAction(m_SpectatorCamera, GC_ESpectatorCameraMode.MIRROR, "MIRROR FIRST PERSON");
			GC_ContextMenuActionRegistry.Register(m_MirrorCameraAction);
		}

		if (!m_NetworkCameraAction)
		{
			m_NetworkCameraAction = new GC_SpectatorCameraContextAction(m_SpectatorCamera, GC_ESpectatorCameraMode.NETWORK, "NETWORK FIRST PERSON");
			GC_ContextMenuActionRegistry.Register(m_NetworkCameraAction);
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

		if (m_MirrorCameraAction)
		{
			GC_ContextMenuActionRegistry.Unregister(m_MirrorCameraAction);
			m_MirrorCameraAction = null;
		}

		if (m_NetworkCameraAction)
		{
			GC_ContextMenuActionRegistry.Unregister(m_NetworkCameraAction);
			m_NetworkCameraAction = null;
		}
	}

	protected void CreateSpectatorMarkerOverlay()
	{
		if (!m_SpectatorCamera)
			return;

		if (!m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay = new GC_SpectatorMarkerOverlay();

		if (!m_SpectatorMarkerOverlay.Start(m_fProjectileTraceDuration, m_bSpectatorProjectileTracersEnabled, m_KilledFromSnapshot, m_bSpectatorKilledFromEnabled))
		{
			PrintFormat("GC | Spectator | marker overlay creation failed: player=%1", GetPlayerId(), level: LogLevel.WARNING);
			m_SpectatorMarkerOverlay = null;
		}
		else
		{
			m_SpectatorMarkerOverlay.SetInterfaceVisible(m_bSpectatorInterfaceVisible);
			if (m_KilledFromSnapshot)
				PrintFormat("GC | Killed From | event=overlay-create result=ready player=%1 enabled=%2 snapshot={%3}", GetPlayerId(), m_bSpectatorKilledFromEnabled, m_KilledFromSnapshot.GetDebugReport());
			else
				PrintFormat("GC | Killed From | event=overlay-create result=ready player=%1 enabled=%2 snapshot=none", GetPlayerId(), m_bSpectatorKilledFromEnabled);
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
		else
		{
			GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
			if (spectatorMenu)
			{
				spectatorMenu.SetInterfaceVisible(m_bSpectatorInterfaceVisible);
				spectatorMenu.SetNightModeEnabled(m_bSpectatorNightModeEnabled);
				spectatorMenu.SetProjectileTracersEnabled(m_bSpectatorProjectileTracersEnabled);
				spectatorMenu.SetKilledFromEnabled(m_bSpectatorKilledFromEnabled);
			}
		}
	}

	bool IsSpectatorInterfaceVisible()
	{
		return m_bSpectatorInterfaceVisible;
	}

	void ToggleSpectatorInterface()
	{
		SetSpectatorInterfaceVisible(!m_bSpectatorInterfaceVisible);
	}

	void SetSpectatorInterfaceVisible(bool visible)
	{
		m_bSpectatorInterfaceVisible = visible;
		SetBaseHUDVisible(visible);
		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
			spectatorMenu.SetInterfaceVisible(visible);
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetInterfaceVisible(visible);
		if (!visible)
			GC_ContextMenuComponent.Close();
	}

	bool IsSpectatorNightModeEnabled()
	{
		return m_bSpectatorNightModeEnabled;
	}

	void ToggleSpectatorNightMode()
	{
		SetSpectatorNightModeEnabled(!m_bSpectatorNightModeEnabled);
	}

	void SetSpectatorNightModeEnabled(bool enabled)
	{
		bool changed = m_bSpectatorNightModeEnabled != enabled;
		if (m_SpectatorCamera && !m_SpectatorCamera.SetSpectatorNightModeEnabled(enabled))
			enabled = false;

		changed = changed && m_bSpectatorNightModeEnabled != enabled;
		m_bSpectatorNightModeEnabled = enabled;
		if (changed)
			SCR_UISoundEntity.SoundEvent("SOUND_E_TOGGLE_FLASHLIGHT");

		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
			spectatorMenu.SetNightModeEnabled(enabled);
	}

	bool AreSpectatorProjectileTracersEnabled()
	{
		return m_bSpectatorProjectileTracersEnabled;
	}

	void ToggleSpectatorProjectileTracers()
	{
		SetSpectatorProjectileTracersEnabled(!m_bSpectatorProjectileTracersEnabled);
	}

	void SetSpectatorProjectileTracersEnabled(bool enabled)
	{
		m_bSpectatorProjectileTracersEnabled = enabled;
		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
			spectatorMenu.SetProjectileTracersEnabled(enabled);
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetProjectileTracersEnabled(enabled);
	}

	bool IsSpectatorKilledFromEnabled()
	{
		return m_bSpectatorKilledFromEnabled;
	}

	void ToggleSpectatorKilledFrom()
	{
		SetSpectatorKilledFromEnabled(!m_bSpectatorKilledFromEnabled);
	}

	void SetSpectatorKilledFromEnabled(bool enabled)
	{
		m_bSpectatorKilledFromEnabled = enabled;
		PrintFormat("GC | Killed From | event=toggle player=%1 enabled=%2 snapshot=%3 menuReady=%4 overlayReady=%5", GetPlayerId(), enabled, m_KilledFromSnapshot != null, m_SpectatorMenu != null, m_SpectatorMarkerOverlay != null);
		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
			spectatorMenu.SetKilledFromEnabled(enabled);
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetKilledFromEnabled(enabled);
	}

	protected void SetBaseHUDVisible(bool visible)
	{
		SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.GetHUDManager();
		if (hudManager)
			hudManager.SetVisible(visible);
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

		PrintFormat("GC | Killed From | event=overlay-destroy player=%1 enabled=%2 snapshotRetained=%3", GetPlayerId(), m_bSpectatorKilledFromEnabled, m_KilledFromSnapshot != null);
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
