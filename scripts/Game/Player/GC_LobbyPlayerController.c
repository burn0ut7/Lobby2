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
	protected bool m_bFactionReadinessPending;
	protected string m_sFactionReadinessError;
	ref ScriptInvoker<string> m_OnFactionReadinessResult = new ScriptInvoker<string>();

	ref ScriptInvoker<int, string, string, string> m_OnSlotReservationResult = new ScriptInvoker<int, string, string, string>();
	ref ScriptInvoker<int, string> m_OnAdminGroupLockResult = new ScriptInvoker<int, string>();
	ref ScriptInvoker<int, string> m_OnAdminFactionRulesResult = new ScriptInvoker<int, string>();

	//! The eight-argument RPC ceiling leaves one integer for both sight indexes.
	//! Each index is offset by one so zero represents the no-sight/no-zoom state.
	static const int NETWORK_CAMERA_SIGHT_STATE_SCALE = 65536;
	protected static const ResourceName SPECTATOR_CAMERA_PREFAB = "{0C428AB427BB44F5}Prefabs/Editor/Camera/GC_SpectatorCamera.et";
	protected static const ResourceName PAUSE_ACCESS_BUTTON_LAYOUT = "{6A4D00000000001A}UI/layouts/Briefing/Lobby_Briefing_Parameter_Option.layout";
	protected static const int EDITOR_MANAGER_ATTACH_RETRIES = 20;
	[Attribute("30", desc: "Networked first-person camera snapshots per second (1-60).")]
	protected int m_iNetworkCameraUpdateRate;
	[Attribute("0.2", desc: "Seconds of travel represented by each live projectile segment and final-segment retention.")]
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
	protected bool m_bPublishNetworkCamera;
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
	protected string m_sPendingKilledFromLoadout;
	protected ref GC_KilledFromDamageTracker m_LocalKilledFromTracker;
	protected PauseMenuUI m_PauseMenu;
	protected Widget m_wPauseAdminButton;
	protected SCR_ButtonTextComponent m_PauseAdminButton;
	protected ref GC_AdminSignInDialog m_AdminSignInDialog;
	protected Widget m_wPauseSlottingButton;
	protected Widget m_wPauseBriefingButton;
	protected SCR_ButtonTextComponent m_PauseSlottingButton;
	protected SCR_ButtonTextComponent m_PauseBriefingButton;
	protected InputManager m_LobbyInputManager;
	[RplProp(onRplName: "ApplyLobbyRadio")]
	protected int m_iLobbyChatFrequency = GC_LobbyRadio.INACTIVE_FREQUENCY;
	[RplProp(onRplName: "ApplyLobbyRadio")]
	protected int m_iLobbyFactionFrequency = GC_LobbyRadio.INACTIVE_FREQUENCY;
	[RplProp(onRplName: "ApplyLobbyRadio")]
	protected bool m_bLobbyVoiceDeafened;
	protected int m_iLobbyVoiceRequest;
	protected int m_iLobbyVoiceSlot = -1;
	protected bool m_bLobbyVoiceHeld;
	protected SCR_VoNComponent m_LobbyObservedVoice;
	ref ScriptInvokerInt m_OnVoiceActivity = new ScriptInvokerInt();
	ref ScriptInvoker<bool, int> m_OnScenarioParameterResult = new ScriptInvoker<bool, int>();
	protected int m_iScenarioParameterRequest;
	protected SCR_VoNComponent m_LobbyCapture;
	protected int m_iServerLobbyVoiceSlot = -1;
	protected ref array<ref GC_AdminMessage> m_aAdminMessages = {};
	ref ScriptInvoker m_OnAdminMessagesChanged = new ScriptInvoker();
	ref ScriptInvoker m_OnAdminMessageResult = new ScriptInvoker();
	protected int m_iAdminMessageRequestId;
	//! Matches feedback to the latest action; never used as authorization.
	protected int m_iAdminActionRequestId;
	protected int m_iAdminFollowPendingRequest;
	protected int m_iAdminFollowPendingTarget;
	protected UUID m_sAdminFollowPendingIdentity;
	protected int m_iAdminFollowIssuedAt;
	protected bool m_bAdminFollowPending;
	ref ScriptInvoker m_OnAdminReport = new ScriptInvoker();
	ref ScriptInvoker m_OnAdminBanPage = new ScriptInvoker();
	ref ScriptInvoker m_OnAdminActionResult = new ScriptInvoker();
	ref ScriptInvoker m_OnAdminModerationResult = new ScriptInvoker();
	ref ScriptInvoker m_OnModerationRows = new ScriptInvoker();
	ref ScriptInvoker m_OnModerationChanged = new ScriptInvoker();
	ref ScriptInvoker m_OnPlayerDetailsCopied = new ScriptInvoker();
	protected bool m_bLobbyDisabledNativeVoice;
	//! Server-only, session-local permission to receive new Admin-channel messages.
	protected float m_fAdminChatReceiveUntil;

	//------------------------------------------------------------------------------------------------
	void RenewAdminChatReceiveWindow()
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager())
			m_fAdminChatReceiveUntil = GetGame().GetWorld().GetWorldTime() + 180000;
	}

	//------------------------------------------------------------------------------------------------
	bool CanReceiveAdminChat()
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager())
			return false;

		return SCR_Global.IsAdmin(GetPlayerId()) || GetGame().GetWorld().GetWorldTime() < m_fAdminChatReceiveUntil;
	}

	void ~GC_LobbyPlayerController()
	{
		ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	//! Release owned local state before the world changes, not only on destruction.
	void ShutdownLobbyPresentation()
	{
		m_aAdminMessages.Clear();
		if (GetGame().GetPlayerController() == this)
		{
			GC_AdminMessagePopupComponent.CloseAll();
			GC_AdminToolsComponent.Close();
		}
		StopLobbyVoice(true);
		BindLobbyVoiceActivity(null);
		if (m_LobbyInputManager)
		{
			m_LobbyInputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, LobbyChatDown);
			m_LobbyInputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, LobbyChatUp);
			m_LobbyInputManager.RemoveActionListener("VONChannel", EActionTrigger.DOWN, LobbyFactionDown);
			m_LobbyInputManager.RemoveActionListener("VONChannel", EActionTrigger.UP, LobbyFactionUp);
			m_LobbyInputManager.RemoveActionListener("GC_AdminVON", EActionTrigger.DOWN, LobbyAdminDown);
			m_LobbyInputManager.RemoveActionListener("GC_AdminVON", EActionTrigger.UP, LobbyAdminUp);
		}
		StopLocalKilledFromTracking(false);
		StopServerNetworkCameraStream();
		GetGame().GetCallqueue().Remove(PublishNetworkCameraTransform);
		GetGame().GetCallqueue().Remove(AttachEditorManager);
		GetGame().GetCallqueue().Remove(ReconcileSpectatorPresentation);
		GetGame().GetCallqueue().Remove(OpenSlottingFromPause);
		GetGame().GetCallqueue().Remove(OpenBriefingFromPause);
		GetGame().GetCallqueue().Remove(OpenAdminToolsFromPause);
		CloseAdminSignIn();
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
		m_EditorManager = null;
		m_bSpectatorPresentationRequested = false;
		m_bPublishNetworkCamera = false;
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
		{
			m_LobbyInputManager.AddActionListener("GC_OpenLobby", EActionTrigger.DOWN, OpenSlottingFromPause);
			m_LobbyInputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, LobbyChatDown);
			m_LobbyInputManager.AddActionListener("VONDirect", EActionTrigger.UP, LobbyChatUp);
			m_LobbyInputManager.AddActionListener("VONChannel", EActionTrigger.DOWN, LobbyFactionDown);
			m_LobbyInputManager.AddActionListener("VONChannel", EActionTrigger.UP, LobbyFactionUp);
			m_LobbyInputManager.AddActionListener("GC_AdminVON", EActionTrigger.DOWN, LobbyAdminDown);
			m_LobbyInputManager.AddActionListener("GC_AdminVON", EActionTrigger.UP, LobbyAdminUp);
		}
		ApplyLobbyRadio();
		gameMode.UpdateLocalPhasePresentation(this, "controller-init");
		StartLocalKilledFromTracking(GetControlledEntity());
	}

	//! Apply on channel replication, possession, or native radio registration.
	void SetLobbyRadioChannels(int chatFrequency, int factionFrequency, bool deafened)
	{
		if (!Replication.IsServer())
			return;
		if (m_iLobbyChatFrequency == chatFrequency && m_iLobbyFactionFrequency == factionFrequency && m_bLobbyVoiceDeafened == deafened)
			return;
		m_iLobbyChatFrequency = chatFrequency;
		m_iLobbyFactionFrequency = factionFrequency;
		m_bLobbyVoiceDeafened = deafened;
		Replication.BumpMe();
		ApplyLobbyRadio();
	}

	void ApplyLobbyRadio()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;
		IEntity body = GetControlledEntity();
		bool spectator = body && body.FindComponent(GC_LobbySpectatorBodyComponent);
		if (GetGame().GetPlayerController() == this)
		{
			SCR_VONController nativeVoice = SCR_VONController.Cast(FindComponent(SCR_VONController));
			if (nativeVoice)
			{
				if (spectator && !m_bLobbyDisabledNativeVoice)
					m_bLobbyDisabledNativeVoice = nativeVoice.SetVONDisabled(true);
				else if (!spectator && m_bLobbyDisabledNativeVoice)
				{
					nativeVoice.SetVONDisabled(false);
					m_bLobbyDisabledNativeVoice = false;
				}
			}
		}
		BaseRadioComponent radio = GC_LobbyRadio.FindRadio(body);
		if (!radio || radio.TransceiversCount() < 3)
			return;
		if (GetGame().GetPlayerController() == this)
			StopLobbyVoice();
		radio.SetPower(true);
		radio.GetTransceiver(GC_LobbyRadio.ADMIN).SetFrequency(0);
		radio.GetTransceiver(GC_LobbyRadio.ADMIN).SetMuteState(false);
		radio.GetTransceiver(GC_LobbyRadio.CHAT).SetFrequency(m_iLobbyChatFrequency);
		radio.GetTransceiver(GC_LobbyRadio.CHAT).SetMuteState(!spectator || m_bLobbyVoiceDeafened || m_iLobbyChatFrequency == GC_LobbyRadio.INACTIVE_FREQUENCY);
		radio.GetTransceiver(GC_LobbyRadio.FACTION).SetFrequency(m_iLobbyFactionFrequency);
		//! Avoid a duplicate receiver when selected chat is already faction HQ.
		radio.GetTransceiver(GC_LobbyRadio.FACTION).SetMuteState(!spectator || m_bLobbyVoiceDeafened || m_iLobbyFactionFrequency == GC_LobbyRadio.INACTIVE_FREQUENCY || m_iLobbyFactionFrequency == m_iLobbyChatFrequency);
		PrintFormat("GC | Voice | tuned: player=%1 admin=0 chat=%2 faction=%3 spectator=%4 deafened=%5", GetPlayerId(), m_iLobbyChatFrequency, m_iLobbyFactionFrequency, spectator, m_bLobbyVoiceDeafened);
	}

	//! Native capture/receive callbacks describe locally observed speech only.
	protected void BindLobbyVoiceActivity(IEntity body)
	{
		if (m_LobbyObservedVoice)
		{
			m_LobbyObservedVoice.m_OnCaptureVON.Remove(OnLobbyVoiceCaptured);
			m_LobbyObservedVoice.m_OnReceivedVON.Remove(OnLobbyVoiceReceived);
		}
		m_LobbyObservedVoice = null;
		if (body)
			m_LobbyObservedVoice = SCR_VoNComponent.Cast(body.FindComponent(SCR_VoNComponent));
		if (m_LobbyObservedVoice)
		{
			m_LobbyObservedVoice.m_OnCaptureVON.Insert(OnLobbyVoiceCaptured);
			m_LobbyObservedVoice.m_OnReceivedVON.Insert(OnLobbyVoiceReceived);
		}
	}

	protected void OnLobbyVoiceCaptured(BaseTransceiver transmitter)
	{
		m_OnVoiceActivity.Invoke(GetPlayerId());
	}

	protected void OnLobbyVoiceReceived(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (receiver && receiver.IsMuted())
			return;
		m_OnVoiceActivity.Invoke(playerId);
	}

	protected void LobbyChatDown() { BeginLobbyVoice(GC_LobbyRadio.CHAT); }
	protected void LobbyFactionDown() { BeginLobbyVoice(GC_LobbyRadio.FACTION); }
	protected void LobbyAdminDown() { BeginLobbyVoice(GC_LobbyRadio.ADMIN); }
	protected void LobbyChatUp() { ReleaseLobbyVoice(GC_LobbyRadio.CHAT); }
	protected void LobbyFactionUp() { ReleaseLobbyVoice(GC_LobbyRadio.FACTION); }
	protected void LobbyAdminUp() { ReleaseLobbyVoice(GC_LobbyRadio.ADMIN); }

	protected void BeginLobbyVoice(int slot)
	{
		IEntity body = GetControlledEntity();
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this || !body)
			return;
		if (slot != GC_LobbyRadio.ADMIN && !body.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		StopLobbyVoice();
		m_iLobbyVoiceSlot = slot;
		m_bLobbyVoiceHeld = true;
		PrintFormat("GC | Voice transmit | request player=%1 slot=%2 sequence=%3", GetPlayerId(), slot, m_iLobbyVoiceRequest);
		Rpc(RpcAsk_LobbyVoice, slot, m_iLobbyVoiceRequest);
	}

	protected void ReleaseLobbyVoice(int slot)
	{
		if (m_iLobbyVoiceSlot == slot)
			StopLobbyVoice();
	}

	protected void StopLobbyVoice(bool releaseNativeVoice = false)
	{
		if (m_bLobbyVoiceHeld && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_ReleaseLobbyVoice);
		m_iLobbyVoiceRequest++;
		m_bLobbyVoiceHeld = false;
		m_iLobbyVoiceSlot = -1;
		GetGame().GetCallqueue().Remove(CheckLobbyVoiceHold);
		if (m_LobbyCapture)
		{
			m_LobbyCapture.GC_EndLobbyCapture();
			m_LobbyCapture.SetTransmitRadio(null);
			m_LobbyCapture.SetCommMethod(ECommMethod.DIRECT);
			m_LobbyCapture = null;
		}
		IEntity body = GetControlledEntity();
		if (m_bLobbyDisabledNativeVoice && (releaseNativeVoice || !GC_LobbyGameMode.GetGameMode() || !body || !body.FindComponent(GC_LobbySpectatorBodyComponent)))
		{
			SCR_VONController nativeVoice = SCR_VONController.Cast(FindComponent(SCR_VONController));
			if (nativeVoice)
				nativeVoice.SetVONDisabled(false);
			m_bLobbyDisabledNativeVoice = false;
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LobbyVoice(int slot, int request)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity body = GetControlledEntity();
		bool allowed = manager && body && GetGame().GetPlayerManager().IsPlayerConnected(GetPlayerId());
		if (slot == GC_LobbyRadio.ADMIN)
		{
			allowed = allowed && SCR_Global.IsAdmin(GetPlayerId());
			string outcome = "rejected-not-logged-in";
			if (allowed)
				outcome = "authorized";
			GC_AdminActionLogger.Log(GetPlayerId(), "voice-broadcast", "all-players", outcome);
		}
		else
		{
			allowed = allowed && body.FindComponent(GC_LobbySpectatorBodyComponent) && !m_bLobbyVoiceDeafened;
			if (slot == GC_LobbyRadio.CHAT)
				allowed = allowed && m_iLobbyChatFrequency != GC_LobbyRadio.INACTIVE_FREQUENCY;
			else if (slot == GC_LobbyRadio.FACTION)
				allowed = allowed && m_iLobbyFactionFrequency != GC_LobbyRadio.INACTIVE_FREQUENCY;
			else
				allowed = false;
		}
		PrintFormat("GC | Voice transmit | authority player=%1 slot=%2 sequence=%3 allowed=%4 chat=%5 faction=%6", GetPlayerId(), slot, request, allowed, m_iLobbyChatFrequency, m_iLobbyFactionFrequency);
		m_iServerLobbyVoiceSlot = -1;
		if (allowed)
			m_iServerLobbyVoiceSlot = slot;
		Rpc(RpcDo_LobbyVoice, slot, request, allowed);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_LobbyVoice(int slot, int request, bool allowed)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return;

		PrintFormat("GC | Voice transmit | grant player=%1 slot=%2 sequence=%3 allowed=%4 held=%5 currentSequence=%6 currentSlot=%7", GetPlayerId(), slot, request, allowed, m_bLobbyVoiceHeld, m_iLobbyVoiceRequest, m_iLobbyVoiceSlot);
		if (!allowed || !m_bLobbyVoiceHeld || request != m_iLobbyVoiceRequest || slot != m_iLobbyVoiceSlot)
			return;
		IEntity body = GetControlledEntity();
		BaseRadioComponent radio = GC_LobbyRadio.FindRadio(body);
		if (!body || !radio || radio.TransceiversCount() < 3 || !radio.IsPowered())
		{
			PrintFormat("GC | Voice transmit | capture unavailable player=%1 body=%2 radio=%3", GetPlayerId(), body, radio, level: LogLevel.WARNING);
			return;
		}
		if (slot == GC_LobbyRadio.ADMIN && !SCR_Global.IsAdmin(GetPlayerId()))
			return;
		SCR_VoNComponent voice = SCR_VoNComponent.Cast(body.FindComponent(SCR_VoNComponent));
		if (!voice)
			return;
		SCR_VONController nativeVoice = SCR_VONController.Cast(FindComponent(SCR_VONController));
		if (nativeVoice && !m_bLobbyDisabledNativeVoice)
			m_bLobbyDisabledNativeVoice = nativeVoice.SetVONDisabled(true);
		m_LobbyCapture = voice;
		voice.SetTransmitRadio(radio.GetTransceiver(slot));
		voice.SetCommMethod(ECommMethod.SQUAD_RADIO);
		bool captured = voice.GC_SetLobbyCapture(true, GetPlayerId());
		PrintFormat("GC | Voice transmit | capture player=%1 slot=%2 frequency=%3 accepted=%4", GetPlayerId(), slot, radio.GetTransceiver(slot).GetFrequency(), captured);
		GetGame().GetCallqueue().CallLater(CheckLobbyVoiceHold, 50, true);
	}

	protected void CheckLobbyVoiceHold()
	{
		string action = "GC_AdminVON";
		if (m_iLobbyVoiceSlot == GC_LobbyRadio.CHAT)
			action = "VONDirect";
		else if (m_iLobbyVoiceSlot == GC_LobbyRadio.FACTION)
			action = "VONChannel";
		if (!GC_LobbyGameMode.GetGameMode() || !m_LobbyInputManager || m_LobbyInputManager.GetActionValue(action) <= 0 || (m_iLobbyVoiceSlot == GC_LobbyRadio.ADMIN && !SCR_Global.IsAdmin(GetPlayerId())))
			StopLobbyVoice();
		else if (m_LobbyCapture)
			m_LobbyCapture.GC_SetLobbyCapture(true, GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	bool HasAuthorizedLobbyCapture()
	{
		IEntity body = GetControlledEntity();
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || m_iServerLobbyVoiceSlot < 0 || !body)
			return false;
		// Normal character/gameplay VON never participates in lobby moderation.
		if (!body.FindComponent(GC_LobbySpectatorBodyComponent))
			return false;
		if (m_iServerLobbyVoiceSlot == GC_LobbyRadio.ADMIN)
			return SCR_Global.IsAdmin(GetPlayerId());
		return !m_bLobbyVoiceDeafened;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReleaseLobbyVoice()
	{
		m_iServerLobbyVoiceSlot = -1;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.ReleaseLobbyVoiceTransmission(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	ref ScriptInvoker m_OnAdminPlayerToolResult = new ScriptInvoker();
	protected int m_iAdminPlayerToolRequest;

	//------------------------------------------------------------------------------------------------
	int ReserveAdminPlayerToolRequest()
	{
		m_iAdminPlayerToolRequest++;
		return m_iAdminPlayerToolRequest;
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminHeal(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters = GC_LobbyManager.GC_DEFAULT_HEAL_RADIUS, string factionKey = "")
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_AdminHeal, scope, players, identities, requestId, radiusMeters, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminHeal(GC_EAdminHealScope scope, array<int> players, array<string> identities, int requestId, int radiusMeters, string factionKey)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string outcome = "Admin tools are unavailable in this game mode.";
		if (manager)
			outcome = manager.GC_ExecuteAdminHeal(GetPlayerId(), scope, players, identities, requestId, radiusMeters, factionKey);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "heal", "", "rejected-inactive-mode");
		Rpc(RpcDo_AdminPlayerToolResult, requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminPlayerToolResult(int requestId, string outcome)
	{
		m_OnAdminPlayerToolResult.Invoke(requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	int NextAdminMessageRequestId()
	{
		m_iAdminMessageRequestId++;
		return m_iAdminMessageRequestId;
	}

	//------------------------------------------------------------------------------------------------
	void RequestStartSlotLottery(string factionKey, RplId groupId, int slotId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_StartSlotLottery, factionKey, groupId, slotId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartSlotLottery(string factionKey, RplId groupId, int slotId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_SlotLotteryResult, manager.StartSlotLottery(GetPlayerId(), factionKey, groupId, slotId));
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "lottery-start", "", "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestEnterSlotLottery(int lotteryId = 0, bool enter = true)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_EnterSlotLottery, lotteryId, enter);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_EnterSlotLottery(int lotteryId, bool enter)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_SlotLotteryResult, manager.EnterSlotLottery(GetPlayerId(), lotteryId, enter));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SlotLotteryResult(string result)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && result != "started")
			SCR_HintManagerComponent.ShowCustomHint(result, "Slot lottery", 8);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminMessage(GC_EAdminMessageAudience audience, string message, array<string> selectedIdentities, int requestId = 0, string factionKey = "")
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_AdminMessage, audience, message, selectedIdentities, requestId, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminMessage(GC_EAdminMessageAudience audience, string message, array<string> selectedIdentities, int requestId, string factionKey)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_AdminMessageResult, requestId, manager.SendAdminMessage(GetPlayerId(), audience, message, selectedIdentities, factionKey));
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "message-send", "", "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminMessageResult(int requestId, string outcome)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			m_OnAdminMessageResult.Invoke(requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	void DeliverAdminMessage(int messageId, int senderId, string senderName, GC_EAdminMessageAudience audience, string message, bool preserveLineBreaks = false)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager())
			Rpc(RpcDo_AdminMessage, messageId, senderId, senderName, audience, message, preserveLineBreaks);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminMessage(int messageId, int senderId, string senderName, GC_EAdminMessageAudience audience, string message, bool preserveLineBreaks)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return;
		if (audience == GC_EAdminMessageAudience.ADMINS && !SCR_Global.IsAdmin(GetPlayerId()))
			return;
		GC_AdminMessage entry = new GC_AdminMessage();
		entry.m_iMessageId = messageId;
		entry.m_iSenderId = senderId;
		entry.m_sSenderName = senderName;
		entry.m_sText = message;
		entry.m_bPreserveLineBreaks = preserveLineBreaks;
		entry.m_eAudience = audience;
		if (m_aAdminMessages.Count() >= 100)
			m_aAdminMessages.RemoveOrdered(0);
		m_aAdminMessages.Insert(entry);
		m_OnAdminMessagesChanged.Invoke();
		if (audience != GC_EAdminMessageAudience.ADMINS)
			GC_AdminMessagePopupComponent.Show(entry);
	}

	//------------------------------------------------------------------------------------------------
	array<ref GC_AdminMessage> GetAdminMessages()
	{
		if (!SCR_Global.IsAdmin(GetPlayerId()))
		{
			for (int i = m_aAdminMessages.Count() - 1; i >= 0; i--)
			{
				if (m_aAdminMessages[i].m_eAudience == GC_EAdminMessageAudience.ADMINS)
					m_aAdminMessages.RemoveOrdered(i);
			}
		}
		return m_aAdminMessages;
	}

	//------------------------------------------------------------------------------------------------
	void RequestSlotReservation(string action, int slotId, UUID expectedClaim, string identity, string playerName, string input, int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_SlotReservation, action, slotId, expectedClaim, identity, playerName, input, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SlotReservation(string action, int slotId, string expectedClaim, string identity, string playerName, string input, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string output;
		string outcome = "Lobby2 is no longer active.";
		if (manager)
			outcome = manager.ExecuteSlotReservation(GetPlayerId(), action, slotId, expectedClaim, identity, playerName, input, output);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-reservation", action, "rejected-inactive-mode");
		string currentClaim;
		if (manager)
		{
			GC_Slot slot = manager.GetSlot(slotId);
			if (slot)
				currentClaim = slot.GetUUID();
		}
		Rpc(RpcDo_SlotReservation, requestId, outcome, output, currentClaim);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SlotReservation(int requestId, string outcome, string output, string currentClaim)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			m_OnSlotReservationResult.Invoke(requestId, outcome, output, currentClaim);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminAssignSlot(int playerId, UUID expectedIdentity, int slotId, UUID expectedClaim)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminAssignSlot, playerId, expectedIdentity, slotId, expectedClaim, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminAssignSlot(int playerId, string expectedIdentity, int slotId, string expectedClaim, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_AdminActionResult, manager.ExecuteAdminAssignSlot(GetPlayerId(), playerId, expectedIdentity, slotId, expectedClaim), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-assign", slotId.ToString(), "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminRemoveFactionLock(int playerId, UUID expectedIdentity)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminRemoveFactionLock, playerId, expectedIdentity, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminRemoveFactionLock(int playerId, string expectedIdentity, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_AdminActionResult, manager.ExecuteAdminRemoveFactionLock(GetPlayerId(), playerId, expectedIdentity), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "faction-unlock", playerId.ToString(), "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminFactionRules(int expectedRevision, int lockOverride, bool enforceBalance, int requestId = -1)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			if (requestId < 0)
				requestId = ReserveAdminPlayerToolRequest();
			Rpc(RpcAsk_AdminFactionRules, expectedRevision, lockOverride, enforceBalance, requestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminFreeze(int expectedRevision, string action, int seconds, array<int> selectedPlayers = null, array<string> selectedIdentities = null)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminFreeze, expectedRevision, action, seconds, m_iAdminActionRequestId, selectedPlayers, selectedIdentities, selectedPlayers != null);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Authority chooses the destination; native teleport must run on the owner
	//! of the controlled character. Client requests cannot choose corrections.
	void ApplyFreezeBoundary(vector position)
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		IEntity character = GetControlledEntity();
		if (!Replication.IsServer() || !freeze || !freeze.AffectsPlayer(GetPlayerId()) || !character)
			return;
		RplId characterId = Replication.FindItemId(character);
		if (characterId == RplId.Invalid())
			return;
		Rpc(RpcDo_FreezeBoundary, characterId, position, freeze.GetRevision());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_FreezeBoundary(RplId characterId, vector position, int freezeRevision)
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		IEntity character = GetControlledEntity();
		if (GetGame().GetPlayerController() != this || !freeze || !freeze.AffectsPlayer(GetPlayerId()) || !character || Replication.FindItemId(character) != characterId)
			return;
		if (freeze.GetRevision() != freezeRevision)
			return;
		SCR_Global.TeleportPlayer(GetPlayerId(), position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminFreeze(int expectedRevision, string action, int seconds, int requestId, array<int> selectedPlayers, array<string> selectedIdentities, bool selectedOnly)
	{
		if (!selectedOnly)
		{
			selectedPlayers = null;
			selectedIdentities = null;
		}
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (freeze)
			SendAdminActionResult(freeze.ExecuteAdminAction(GetPlayerId(), expectedRevision, action, seconds, selectedPlayers, selectedIdentities), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "mission-freeze", action, "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminFactionRules(int expectedRevision, int lockOverride, bool enforceBalance, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string outcome = "rejected-inactive-mode";
		if (manager)
			outcome = manager.ExecuteAdminFactionRules(GetPlayerId(), expectedRevision, lockOverride, enforceBalance);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "faction-rules", "", "rejected-inactive-mode");
		// A permission rejection must also resolve the requesting UI's pending state.
		Rpc(RpcDo_AdminFactionRulesResult, requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminFactionRulesResult(int requestId, string outcome)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return;
		m_OnAdminFactionRulesResult.Invoke(requestId, outcome);
		m_OnAdminActionResult.Invoke(outcome);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminSessionReport(string search)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_AdminSessionReport, search);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminSessionReport(string search)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return;
		string report = manager.GetAdminSessionReport(GetPlayerId(), search);
		if (SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_AdminSessionReport, report);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminSessionReport(string report)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && SCR_Global.IsAdmin(GetPlayerId()))
			m_OnAdminReport.Invoke(report);
	}

	//------------------------------------------------------------------------------------------------
	int ReserveAdminActionRequest()
	{
		return ++m_iAdminActionRequestId;
	}

	//------------------------------------------------------------------------------------------------
	void RequestModerationSession(int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_ModerationSession, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestCopyPlayerDetails(UUID identity)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(RpcAsk_CopyPlayerDetails, identity);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CopyPlayerDetails(UUID identity)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.CopyAdminPlayerDetails(GetPlayerId(), identity);
	}

	//------------------------------------------------------------------------------------------------
	void SendPlayerDetailsToClipboard(string details)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_CopyPlayerDetails, details);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CopyPlayerDetails(string details)
	{
		GetGame().GetCallqueue().CallLater(CopyPlayerDetailsToClipboard, 0, false, details);
	}

	//------------------------------------------------------------------------------------------------
	protected void CopyPlayerDetailsToClipboard(string details)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this || !SCR_Global.IsAdmin(GetPlayerId()))
			return;
		System.ExportToClipboard(details);
		string clipboard = System.ImportFromClipboard();
		clipboard.Replace("\r", "");
		m_OnPlayerDetailsCopied.Invoke(clipboard == details);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ModerationSession(int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.SendModerationSession(GetPlayerId(), requestId);
	}

	//------------------------------------------------------------------------------------------------
	void SendModerationRows(array<string> rows, int requestId)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_ModerationRows, rows, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void SendModerationChanged()
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_ModerationChanged);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ModerationChanged()
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && SCR_Global.IsAdmin(GetPlayerId()))
			m_OnModerationChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ModerationRows(array<string> rows, int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && SCR_Global.IsAdmin(GetPlayerId()))
			m_OnModerationRows.Invoke(rows, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminPlayerAction(string action, int targetId, UUID identity, string reason, int duration = 0, int page = 0, int requestId = -1, int expiresAt = 0)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			if (requestId < 0)
				requestId = ReserveAdminActionRequest();
			Rpc(RpcAsk_AdminPlayerAction, action, targetId, identity, reason, duration, page, requestId, expiresAt);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminPlayerAction(string action, int targetId, string identity, string reason, int duration, int page, int requestId, int expiresAt)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			SendAdminActionResult(manager.ExecuteAdminPlayerAction(GetPlayerId(), action, targetId, identity, reason, duration, page, requestId, expiresAt), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), action, identity, "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminActionResult(string outcome, int requestId)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_AdminActionResult, outcome, requestId);
	}

	//------------------------------------------------------------------------------------------------
	//! Attribute edits have no lobby-menu request ID. Report to their owner directly.
	void SendEditorPlayableResult(string outcome)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager())
			Rpc(RpcDo_EditorPlayableResult, outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_EditorPlayableResult(string outcome)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return;
		SCR_HintManagerComponent.ShowCustomHint(GC_AdminActionResultText.Get(outcome), "Playable slots", 8);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminBanPage(array<string> identities, array<string> labels, int page, int total, int requestId)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(GetPlayerId()))
			Rpc(RpcDo_AdminBanPage, identities, labels, page, total, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminBanPage(array<string> identities, array<string> labels, int page, int total, int requestId)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this || !SCR_Global.IsAdmin(GetPlayerId()) || requestId != m_iAdminActionRequestId)
			return;
		m_OnAdminBanPage.Invoke(identities, labels, page, total);
	}

	//------------------------------------------------------------------------------------------------
	bool CanUseAdminFollowCamera()
	{
		return GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this && m_SpectatorCamera && GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR;
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminFollowPlayer(int targetId, UUID expectedIdentity)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminFollowPlayer, targetId, expectedIdentity, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminFollowPlayer(int targetId, UUID expectedIdentity, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "spectator-follow", targetId.ToString(), "rejected-inactive-mode");
			return;
		}
		RplId entityId;
		string outcome = manager.ValidateAdminFollowPlayer(GetPlayerId(), targetId, expectedIdentity, entityId);
		if (outcome != "authorized")
		{
			SendAdminActionResult(outcome, requestId);
			return;
		}
		m_iAdminFollowPendingRequest = requestId;
		m_iAdminFollowPendingTarget = targetId;
		m_sAdminFollowPendingIdentity = expectedIdentity;
		m_iAdminFollowIssuedAt = System.GetTickCount();
		m_bAdminFollowPending = true;
		Rpc(RpcDo_AdminFollowPlayer, targetId, expectedIdentity, entityId, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminFollowPlayer(int targetId, UUID expectedIdentity, RplId entityId, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || GetGame().GetPlayerController() != this || !SCR_Global.IsAdmin(GetPlayerId()) || requestId != m_iAdminActionRequestId)
			return;
		RplComponent replication = RplComponent.Cast(Replication.FindItem(entityId));
		IEntity entity;
		if (replication)
			entity = replication.GetEntity();
		string outcome = "unavailable-follow-camera";
		if (CanUseAdminFollowCamera() && entity && manager.GetPlayerVoiceIdentity(targetId) == expectedIdentity && GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId) == entity)
		{
			if (m_SpectatorCamera.TryFollowPlayer(entity))
				outcome = "applied";
		}
		m_OnAdminActionResult.Invoke(outcome);
		// Camera state is client-local; this evidence must not be represented as server observation.
		GC_AdminActionLogger.Log(GetPlayerId(), "spectator-follow-local", string.Format("player=%1 identity=%2", targetId, expectedIdentity), outcome);
		Rpc(RpcAsk_AdminFollowReceipt, requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminFollowReceipt(int requestId, string outcome)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager)
			return;
		if (!m_bAdminFollowPending || requestId != m_iAdminFollowPendingRequest)
			return;
		m_bAdminFollowPending = false;
		string target = string.Format("request=%1 player=%2 identity=%3", requestId, m_iAdminFollowPendingTarget, m_sAdminFollowPendingIdentity);
		if (!manager.AuthorizeAdminAction(GetPlayerId(), "spectator-follow-receipt", target))
			return;
		if (System.GetTickCount() - m_iAdminFollowIssuedAt > 10000)
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "spectator-follow-receipt", target, "rejected-expired-receipt");
			return;
		}
		if (outcome != "applied" && outcome != "unavailable-follow-camera")
		{
			GC_AdminActionLogger.Log(GetPlayerId(), "spectator-follow-receipt", target, "rejected-invalid-outcome");
			return;
		}
		// The server records provenance without claiming to observe local camera state.
		GC_AdminActionLogger.Log(GetPlayerId(), "spectator-follow-receipt", target, "client-reported-" + outcome);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminMakePlayable(RplId entityId, RplId expectedGroup)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminMakePlayable, entityId, expectedGroup, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminMakePlayable(RplId entityId, RplId expectedGroup, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			SendAdminActionResult(manager.ExecuteAdminMakePlayable(GetPlayerId(), entityId, expectedGroup), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-make-playable", entityId.ToString(), "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminCreateSlot(int templateSlotId, RplId expectedGroup, vector position)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminCreateSlot, templateSlotId, expectedGroup, position, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminCreateSlot(int templateSlotId, RplId expectedGroup, vector position, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_AdminActionResult, manager.ExecuteAdminCreateSlot(GetPlayerId(), templateSlotId, expectedGroup, position), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-create", templateSlotId.ToString(), "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminSlot(int slotId, UUID expectedClaim, GC_EAdminSlotAction action)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminSlot, slotId, expectedClaim, action, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminSlot(int slotId, string expectedClaim, GC_EAdminSlotAction action, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			Rpc(RpcDo_AdminActionResult, manager.ExecuteAdminSlotAction(GetPlayerId(), slotId, expectedClaim, action), requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-action", slotId.ToString(), "rejected-inactive-mode");
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminGroupLock(RplId groupId, bool locked, int requestId = -1)
	{
		GC_ESlotLockScope scope = GC_ESlotLockScope.GROUP;
		if (groupId == RplId.Invalid())
			scope = GC_ESlotLockScope.ALL;
		RequestAdminScopedSlotLock(scope, groupId, string.Empty, locked, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminScopedSlotLock(GC_ESlotLockScope scope, RplId groupId, FactionKey factionKey, bool locked, int requestId = -1)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			if (requestId < 0)
				requestId = ReserveAdminPlayerToolRequest();
			Rpc(RpcAsk_AdminScopedSlotLock, scope, groupId, factionKey, locked, requestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminScopedSlotLock(GC_ESlotLockScope scope, RplId groupId, string factionKey, bool locked, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string outcome = "rejected-inactive-mode";
		if (manager)
			outcome = manager.ExecuteAdminSlotLocks(GetPlayerId(), scope, groupId, factionKey, locked);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "slot-scope-lock", string.Format("scope=%1 group=%2 faction=%3", scope, groupId, factionKey), outcome);
		Rpc(RpcDo_AdminGroupLockResult, requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminGroupLockResult(int requestId, string outcome)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return;
		m_OnAdminGroupLockResult.Invoke(requestId, outcome);
		m_OnAdminActionResult.Invoke(outcome);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminActionResult(string outcome, int requestId)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this || requestId != m_iAdminActionRequestId)
			return;
		m_OnAdminModerationResult.Invoke(requestId, outcome);
		m_OnAdminActionResult.Invoke(outcome);
	}

	//------------------------------------------------------------------------------------------------
	ref ScriptInvoker m_OnAdminVoiceChannelResult = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	int ReserveAdminVoiceChannelRequestId()
	{
		m_iAdminActionRequestId++;
		return m_iAdminActionRequestId;
	}

	//------------------------------------------------------------------------------------------------
	int RequestAdminVoiceChannel(int targetId, UUID expectedIdentity, GC_VoiceChannelType channelType,
		string factionKey, RplId groupRplId, UUID personalOwnerIdentity, int requestId = -1)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return -1;
		if (requestId < 0)
			requestId = ReserveAdminVoiceChannelRequestId();
		Rpc(RpcAsk_AdminVoiceChannel, targetId, expectedIdentity, channelType, factionKey,
			groupRplId, personalOwnerIdentity, requestId);
		return requestId;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminVoiceChannel(int targetId, string expectedIdentity, GC_VoiceChannelType channelType,
		string factionKey, RplId groupRplId, string personalOwnerIdentity, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string outcome = "rejected-inactive-mode";
		if (manager)
			outcome = manager.ExecuteAdminVoiceChannel(GetPlayerId(), targetId, expectedIdentity,
				channelType, factionKey, groupRplId, personalOwnerIdentity);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "voice-channel", targetId.ToString(), outcome);
		Rpc(RpcDo_AdminVoiceChannelResult, outcome, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_AdminVoiceChannelResult(string outcome, int requestId)
	{
		m_OnAdminVoiceChannelResult.Invoke(requestId, outcome);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdminVoice(int targetId, UUID expectedIdentity, bool priorityControl, bool enabled)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
		{
			m_iAdminActionRequestId++;
			Rpc(RpcAsk_AdminVoice, targetId, expectedIdentity, priorityControl, enabled, m_iAdminActionRequestId);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AdminVoice(int targetId, string expectedIdentity, bool priorityControl, bool enabled, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			{
			string outcome = "rejected-voice";
			if (manager.TrySetAdminVoice(GetPlayerId(), targetId, expectedIdentity, priorityControl, enabled))
				outcome = "applied";
			Rpc(RpcDo_AdminActionResult, outcome, requestId);
		}
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "voice-moderation", targetId.ToString(), "rejected-inactive-mode");
	}

	void RevokeLobbyAdminVoice()
	{
		Rpc(RpcDo_RevokeLobbyAdminVoice);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RevokeLobbyAdminVoice()
	{
		for (int i = m_aAdminMessages.Count() - 1; i >= 0; i--)
		{
			if (m_aAdminMessages[i].m_eAudience == GC_EAdminMessageAudience.ADMINS)
				m_aAdminMessages.RemoveOrdered(i);
		}
		m_OnAdminMessagesChanged.Invoke();
		if (m_iLobbyVoiceSlot == GC_LobbyRadio.ADMIN)
			StopLobbyVoice();
	}

	//! Adds Lobby2-owned access points to the active pause menu without
	//! modifying the shared pause-menu class or layout for unrelated modes.
	protected void OnPauseMenuOpened()
	{
		StopLobbyVoice();
		RemovePauseAccessButtons();

		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		m_PauseMenu = PauseMenuUI.Cast(menuManager.GetTopMenu());
		if (!m_PauseMenu)
			return;

		m_wPauseAdminButton = GetGame().GetWorkspace().CreateWidgets("{A934F7BC18E24510}UI/layouts/Admin/Lobby_PauseAdminButton.layout", m_PauseMenu.GetRootWidget());
		if (m_wPauseAdminButton)
		{
			m_PauseAdminButton = SCR_ButtonTextComponent.Cast(m_wPauseAdminButton.FindHandler(SCR_ButtonTextComponent));
			if (m_PauseAdminButton)
			{
				m_PauseAdminButton.SetText("ADMIN TOOLS");
				m_PauseAdminButton.m_OnClicked.Insert(OnPauseAdminToolsClicked);
			}
		}

		if (!GC_LobbyPhase.IsAtOrAfter(lobbyManager.GetLobbyPhase(), SCR_EGameModeState.SLOTTING))
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
		CloseAdminSignIn();
		RemovePauseAccessButtons();
		m_PauseMenu = null;
	}

	protected void RemovePauseAccessButtons()
	{
		if (m_PauseAdminButton)
			m_PauseAdminButton.m_OnClicked.Remove(OnPauseAdminToolsClicked);
		if (m_wPauseAdminButton)
			m_wPauseAdminButton.RemoveFromHierarchy();
		m_PauseAdminButton = null;
		m_wPauseAdminButton = null;
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

	//------------------------------------------------------------------------------------------------
	protected void OnPauseAdminToolsClicked()
	{
		if (!m_PauseMenu || !GC_LobbyManager.GetManager())
			return;

		if (SCR_Global.IsAdmin(GetPlayerId()))
		{
			RequestOpenAdminToolsFromPause();
			return;
		}

		CloseAdminSignIn();
		m_AdminSignInDialog = new GC_AdminSignInDialog();
		m_AdminSignInDialog.m_OnAuthenticated.Insert(RequestOpenAdminToolsFromPause);
		SCR_ConfigurableDialogUi.CreateFromPreset("{471EFCF445C3E9C6}Configs/ServerBrowser/JoiningDialogs.conf", "PASSWORD_REQUIRED", m_AdminSignInDialog);
		m_AdminSignInDialog.m_OnClose.Insert(OnAdminSignInClosed);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAdminSignInClosed()
	{
		if (!m_AdminSignInDialog)
			return;
		m_AdminSignInDialog.m_OnAuthenticated.Remove(RequestOpenAdminToolsFromPause);
		m_AdminSignInDialog.m_OnClose.Remove(OnAdminSignInClosed);
		m_AdminSignInDialog = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseAdminSignIn()
	{
		if (!m_AdminSignInDialog)
			return;

		m_AdminSignInDialog.m_OnAuthenticated.Remove(RequestOpenAdminToolsFromPause);
		m_AdminSignInDialog.m_OnClose.Remove(OnAdminSignInClosed);
		m_AdminSignInDialog.Close();
		m_AdminSignInDialog = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestOpenAdminToolsFromPause()
	{
		if (m_PauseMenu)
			m_PauseMenu.Close();
		GetGame().GetCallqueue().CallLater(OpenAdminToolsFromPause, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenAdminToolsFromPause()
	{
		GC_AdminToolsComponent.Open();
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
		if (lobbyManager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_DebriefMenu, 0, true);
			return;
		}

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

	//------------------------------------------------------------------------------------------------
	bool IsFactionReadinessPending()
	{
		return m_bFactionReadinessPending;
	}

	//------------------------------------------------------------------------------------------------
	string GetFactionReadinessError()
	{
		return m_sFactionReadinessError;
	}

	//------------------------------------------------------------------------------------------------
	void RequestFactionReadiness(string factionKey, int slotId, int revision, bool ready)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this || m_bFactionReadinessPending)
			return;
		m_bFactionReadinessPending = true;
		m_sFactionReadinessError = string.Empty;
		m_OnFactionReadinessResult.Invoke(string.Empty);
		Rpc(RpcAsk_FactionReadiness, factionKey, slotId, revision, ready);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_FactionReadiness(string factionKey, int slotId, int revision, bool ready)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string error = "Readiness is unavailable.";
		if (manager)
			error = manager.SetFactionReady(GetPlayerId(), factionKey, slotId, revision, ready);
		Rpc(RpcDo_FactionReadinessResult, error);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_FactionReadinessResult(string error)
	{
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this)
			return;
		m_bFactionReadinessPending = false;
		m_sFactionReadinessError = error;
		m_OnFactionReadinessResult.Invoke(error);
	}

	void RequestLobbySlot(int slotId)
	{
		if (slotId < 0)
			return;

		Rpc(RpcAsk_RequestLobbySlot, slotId);
	}

	void RequestVoiceChannel(GC_VoiceChannelType channelType, string factionKey = "", RplId groupRplId = RplId.Invalid(), UUID personalOwnerIdentity = "")
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || GetGame().GetPlayerController() != this)
			return;

		if (SCR_Global.IsAdmin(GetPlayerId()))
		{
			// The normal header supplies presentation metadata for personal rooms;
			// the admin request requires only the destination's canonical identity.
			if (channelType == GC_VoiceChannelType.PERSONAL || channelType == GC_VoiceChannelType.GLOBAL)
				factionKey = string.Empty;
			else if (channelType == GC_VoiceChannelType.GROUP)
			{
				SCR_AIGroup group = manager.GetRegisteredGroupByRplId(groupRplId);
				if (group && group.GetFaction())
					factionKey = group.GetFaction().GetFactionKey();
			}

			// Use the authenticated, audited request even if login changes in transit.
			RequestAdminVoiceChannel(GetPlayerId(), manager.GetPlayerVoiceIdentity(GetPlayerId()),
				channelType, factionKey, groupRplId, personalOwnerIdentity);
			return;
		}

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
		m_iScenarioParameterRequest++;
		Rpc(RpcAsk_RequestScenarioParameters, m_iScenarioParameterRequest, baseRevision, values);
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
	protected void RpcAsk_RequestScenarioParameters(int requestId, int baseRevision, array<ref GC_LobbyParameterValue> values)
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		bool accepted;
		int revision = -1;
		if (lobbyManager)
		{
			accepted = lobbyManager.TryApplyScenarioParameters(GetPlayerId(), baseRevision, values);
			revision = lobbyManager.GetScenarioParameterRevision();
		}
		Rpc(RpcDo_ScenarioParameterResult, requestId, accepted, revision);
	}

	//------------------------------------------------------------------------------------------------
	//! Acknowledgement is separate from the authoritative snapshot broadcast.
	//! Ignore responses superseded by a newer request from this controller.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ScenarioParameterResult(int requestId, bool accepted, int revision)
	{
		if (GetGame().GetPlayerController() != this || requestId != m_iScenarioParameterRequest)
			return;

		m_OnScenarioParameterResult.Invoke(accepted, revision);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetNetworkCameraTarget(int targetPlayerId)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;

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
		bool wasPublishing = !m_aNetworkCameraSpectators.IsEmpty();
		int spectatorIndex = m_aNetworkCameraSpectators.Find(spectatorPlayerId);
		if (enabled && spectatorIndex == -1)
			m_aNetworkCameraSpectators.Insert(spectatorPlayerId);
		else if (!enabled && spectatorIndex != -1)
			m_aNetworkCameraSpectators.Remove(spectatorIndex);

		bool publishing = !m_aNetworkCameraSpectators.IsEmpty();
		if (publishing != wasPublishing)
			Rpc(RpcDo_SetNetworkCameraPublishing, publishing);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetNetworkCameraPublishing(bool enabled)
	{
		if (GetGame().GetPlayerController() != this)
			return;
		if (!GC_LobbyGameMode.GetGameMode())
			enabled = false;

		m_bPublishNetworkCamera = enabled;
		m_bNetworkCameraDiagnosticsInitialized = false;
		GetGame().GetCallqueue().Remove(PublishNetworkCameraTransform);
		if (!enabled)
			return;

		PublishNetworkCameraTransform();
		GetGame().GetCallqueue().CallLater(PublishNetworkCameraTransform, 1000 / GetNetworkCameraUpdateRate(), true);
	}

	protected void PublishNetworkCameraTransform()
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this || !m_bPublishNetworkCamera)
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

		GC_SpectatorSample sample = new GC_SpectatorSample();
		if (!sample.CaptureCharacter(ChimeraCharacter.Cast(controlledEntity)))
			return;
		RplComponent characterReplication = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
		if (!characterReplication)
			return;

		vector cameraTransform[4];
		currentCamera.GetWorldTransform(cameraTransform);
		vector snapshotPosition = cameraTransform[3] - controlledEntity.GetWorldTransformAxis(3);
		vector snapshotForward = cameraTransform[2];
		vector snapshotUp = cameraTransform[1];
		// X is main-camera FOV, Y is optional PiP FOV and Z is the selected
		// sight's actual FOV. ADS uses the camera relative to the animated sight;
		// other views use world-aligned axes relative to the character origin.
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
				{
					lensState[1] = overlayCamera.GetVerticalFOV();
					pipSights.GC_GetSpectatorOpticScales(sample.m_fOpticReticleScale, sample.m_fOpticEdgeScale);
				}
			}

			sightState = (sightsIndex + 1) * NETWORK_CAMERA_SIGHT_STATE_SCALE + zoomIndex + 1;

			// Preserve the owner's eye-to-sight relationship. Sampling eye motion
			// relative to the root mixes it with a separately animated remote weapon.
			vector sightTransform[4], characterTransform[4], localCamera[4], relativeCamera[4];
			float sightFOV;
			if (!weaponManager.GetCurrentSightsCameraTransform(sightTransform, sightFOV))
				return;
			controlledEntity.GetWorldTransform(characterTransform);
			Math3D.MatrixInvMultiply4(characterTransform, cameraTransform, localCamera);
			Math3D.MatrixInvMultiply4(sightTransform, localCamera, relativeCamera);
			snapshotPosition = relativeCamera[3];
			snapshotForward = relativeCamera[2];
			snapshotUp = relativeCamera[1];
		}
		m_iNetworkCameraSequence++;
		bool diagnosticStateChanged = !m_bNetworkCameraDiagnosticsInitialized || m_bNetworkCameraDiagnosticsADS != isADS || m_iNetworkCameraDiagnosticsSightState != sightState;
		if (diagnosticStateChanged || isADS && m_iNetworkCameraSequence % GC_SpectatorCameraDiagnostics.SAMPLE_INTERVAL == 0)
			GC_SpectatorCameraDiagnostics.LogSnapshot("owner", m_iNetworkCameraSequence, controlledEntity, cameraTransform, cameraTransform[3] - controlledEntity.GetOrigin(), lensState[0], lensState[1], lensState[2], isADS, sightState);

		m_bNetworkCameraDiagnosticsInitialized = true;
		m_bNetworkCameraDiagnosticsADS = isADS;
		m_iNetworkCameraDiagnosticsSightState = sightState;
		sample.m_iSequence = m_iNetworkCameraSequence;
		sample.m_vCameraOffset = snapshotPosition;
		sample.m_vCameraForward = snapshotForward;
		sample.m_vCameraUp = snapshotUp;
		sample.m_vLensState = lensState;
		sample.m_iSightState = sightState;
		if (isADS)
			sample.m_iFlags |= GC_SpectatorSample.FLAG_ADS;
		Rpc(RpcAsk_PublishNetworkCameraTransform, characterReplication.Id(), sample);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RpcAsk_PublishNetworkCameraTransform(RplId characterId, GC_SpectatorSample sample)
	{
		if (!GC_LobbyGameMode.GetGameMode() || m_eLobbyPlayerState != GC_ELobbyPlayerState.PLAYING || !sample || !sample.IsValid())
			return;
		RplComponent characterReplication = RplComponent.Cast(Replication.FindItem(characterId));
		if (!characterReplication || characterReplication.GetEntity() != GetControlledEntity())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// The authority owns subscriptions. Upload one owner sample, then publish
		// only to controllers that still observe this player as spectators.
		foreach (int spectatorPlayerId : m_aNetworkCameraSpectators)
		{
			GC_LobbyPlayerController spectatorController = GC_LobbyPlayerController.Cast(playerManager.GetPlayerController(spectatorPlayerId));
			if (!spectatorController || spectatorController.m_iServerNetworkCameraTargetPlayerId != GetPlayerId() || spectatorController.GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
				continue;

			spectatorController.ServerSendNetworkCameraTransform(GetPlayerId(), characterId, sample);
		}
	}

	protected void ServerSendNetworkCameraTransform(int targetPlayerId, RplId characterId, GC_SpectatorSample sample)
	{
		Rpc(RpcDo_ReceiveNetworkCameraTransform, targetPlayerId, characterId, sample);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveNetworkCameraTransform(int targetPlayerId, RplId characterId, GC_SpectatorSample sample)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this || !m_SpectatorCamera)
			return;

		m_SpectatorCamera.ReceiveNetworkCameraTransform(targetPlayerId, characterId, sample);
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
		Rpc(RpcDo_SetKilledFromLoadout, snapshot.GetKillerLoadout());
		Rpc(RpcDo_SetKilledFromSnapshot, snapshot.GetKillerPrefab(), snapshot.GetKillerPosition(), snapshot.GetKillerAngles(), snapshot.GetLineOrigin(), snapshot.GetHitPosition(), snapshot.GetStance(), snapshot.GetDynamicStance(), snapshot.GetFlags());
		Rpc(RpcDo_SetKilledFromIdentity, snapshot.GetKillerDisplayName(), snapshot.IsKillerDisplayNameLocalized());
		if (killerVehiclePrefab != ResourceName.Empty)
			Rpc(RpcDo_SetKilledFromVehicle, killerVehiclePrefab, killerVehiclePosition, killerVehicleAngles, killerCompartmentType, killerCompartmentIndex);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetKilledFromLoadout(string killerLoadout)
	{
		m_sPendingKilledFromLoadout = killerLoadout;
		PrintFormat("GC | Killed From | event=loadout-receive player=%1 bytes=%2", GetPlayerId(), killerLoadout.Length());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetKilledFromSnapshot(ResourceName killerPrefab, vector killerPosition, vector killerAngles, vector lineOrigin, vector hitPosition, int stance, float dynamicStance, int flags)
	{
		GC_KilledFromSnapshot authoritativeSnapshot = new GC_KilledFromSnapshot();
		authoritativeSnapshot.SetNetworkData(killerPrefab, m_sPendingKilledFromLoadout, killerPosition, killerAngles, lineOrigin, hitPosition, stance, dynamicStance, flags);
		m_sPendingKilledFromLoadout = string.Empty;
		PrintFormat("GC | Killed From | event=snapshot-receive player=%1 authoritative={%2} localTracker=%3 retainedLocal=%4", GetPlayerId(), authoritativeSnapshot.GetDebugReport(), m_LocalKilledFromTracker != null, m_KilledFromSnapshot != null);
		if (m_LocalKilledFromTracker)
			authoritativeSnapshot.MergeLocalDamage(m_LocalKilledFromTracker.GetSnapshot());
		else
			authoritativeSnapshot.MergeLocalDamage(m_KilledFromSnapshot);
		m_KilledFromSnapshot = authoritativeSnapshot;
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
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return;

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
			m_sPendingKilledFromLoadout = string.Empty;
		}

		DestroySpectatorCamera();
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		StopLobbyVoice();
		BaseRadioComponent oldLobbyRadio = GC_LobbyRadio.FindRadio(from);
		if (oldLobbyRadio)
			oldLobbyRadio.SetPower(false);
		StopLocalKilledFromTracking(true);
		super.OnControlledEntityChanged(from, to);
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (stats && Replication.IsServer())
			stats.OnPlayerCharacterChanged(GetPlayerId(), to);
		GC_LobbyManager radioManager = GC_LobbyManager.GetManager();
		if (Replication.IsServer() && radioManager)
			radioManager.RefreshPlayerRadio(GetPlayerId());
		ApplyLobbyRadio();

		// Camera and menu presentation exists only on the owning client. The same
		// controller class also exists on dedicated servers and remote proxies.
		if (GetGame().GetPlayerController() != this)
			return;

		if (!GC_LobbyGameMode.GetGameMode())
		{
			ShutdownLobbyPresentation();
			return;
		}

		BindLobbyVoiceActivity(to);
		StartLocalKilledFromTracking(to);

		AttachEditorManager();
		ReconcileSpectatorPresentation();
	}

	protected void StartLocalKilledFromTracking(IEntity entity)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;

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
		m_sPendingKilledFromLoadout = string.Empty;
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
		if (!GC_LobbyGameMode.GetGameMode() || m_EditorManager || GetGame().GetPlayerController() != this)
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
		if (!GC_LobbyGameMode.GetGameMode() || !m_bSpectatorPresentationRequested)
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
			ConnectSpectatorOverlay();
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
				ConnectSpectatorOverlay();
			}
		}
	}

	protected void ConnectSpectatorOverlay()
	{
		if (!m_SpectatorMarkerOverlay)
			return;

		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
		{
			spectatorMenu.SetMarkerOverlay(m_SpectatorMarkerOverlay);
			spectatorMenu.SetMapProjectileTracer(m_SpectatorMarkerOverlay.GetProjectileTracer());
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

	//! The marker, tracer, and killed-from overlay is authored outside the menu
	//! hierarchy, so menu focus must suspend it explicitly.
	void SetSpectatorMenuFocused(bool focused)
	{
		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.SetInterfaceVisible(focused && m_bSpectatorInterfaceVisible);
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

		GC_Lobby_SpectatorMenu spectatorMenu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (spectatorMenu)
		{
			spectatorMenu.SetMarkerOverlay(null);
			spectatorMenu.SetMapProjectileTracer(null);
		}
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
