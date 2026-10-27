//! Opt-in network spectator probes. Commands use the owning player's normal
//! character controller and spectator entry point; no roles or state are granted.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSpectatorSampleContract()
	{
		GC_SpectatorSample sample = new GC_SpectatorSample();
		sample.m_vForward = vector.Forward;
		sample.m_vUp = vector.Up;
		sample.m_vCameraForward = vector.Forward;
		sample.m_vCameraUp = vector.Up;
		sample.m_vLensState = "60 -1 -1";
		if (!sample.IsValid())
			return "FAIL valid sample";
		sample.m_iVersion = 2;
		if (sample.IsValid())
			return "FAIL version accepted";
		sample.m_iVersion = 1;
		sample.m_fLean = 2;
		if (sample.IsValid())
			return "FAIL lean accepted";
		sample.m_fLean = -1;
		sample.m_iStance = 3;
		if (sample.IsValid())
			return "FAIL stance accepted";
		sample.m_iStance = 2;
		sample.m_vCameraUp = vector.Forward;
		if (sample.IsValid())
			return "FAIL degenerate basis accepted";
		sample.m_vCameraUp = vector.Up;
		sample.m_vLensState = "180 -1 -1";
		if (sample.IsValid())
			return "FAIL FOV accepted";
		sample.m_vLensState = "60 -1 -1";
		sample.m_vPosition = "10000001 0 0";
		if (sample.IsValid())
			return "FAIL position accepted";
		sample.m_vPosition = vector.Zero;
		if (!sample.IsValid())
			return "FAIL boundary sample";
		return "PASS valid and boundary samples; rejected version, lean, stance, basis, FOV, position";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestNetworkSpectator(string action, string value)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE local Lobby2 controller";

		return controller.GC_TestNetworkSpectator(action, value);
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestNetworkSpectator(string action, string value)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return "UNAVAILABLE owning Lobby2 player";

		if (action == "request")
		{
			RequestNetworkCameraStream(value.ToInt());
			return "DISPATCHED production subscription request";
		}
		if (action == "unsubscribe")
		{
			StopNetworkCameraStream();
			return "DISPATCHED production unsubscribe; camera should expire";
		}

		if (action == "observe")
		{
			if (!m_SpectatorCamera || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
				return "UNAVAILABLE spectator camera";
			IEntity target = GetGame().GetPlayerManager().GetPlayerControlledEntity(value.ToInt());
			return string.Format("selected=%1", m_SpectatorCamera.TrySetMode(GC_ESpectatorCameraMode.NETWORK, target));
		}

		if (action == "free")
		{
			if (!m_SpectatorCamera)
				return "UNAVAILABLE spectator camera";
			m_SpectatorCamera.SetFreeMode();
			return "free requested";
		}

		if (action == "snapshot")
		{
			string result = string.Format("player=%1 state=%2 publishing=%3 sequence=%4 subscribers=%5", GetPlayerId(), GetLobbyPlayerState(), m_bPublishNetworkCamera, m_iNetworkCameraSequence, m_aNetworkCameraSpectators.Count());
			if (m_SpectatorCamera)
				result += " " + m_SpectatorCamera.GC_TestNetworkSnapshot();
			CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
			if (camera)
				result += string.Format(" camera=%1 cameraPosition=%2", camera.Type(), camera.GetOrigin());
			return result + " " + GC_TestCharacterPose(GetControlledEntity());
		}
		if (action == "target-pose")
			return GC_TestCharacterPose(GetGame().GetPlayerManager().GetPlayerControlledEntity(value.ToInt()));

		ChimeraCharacter character = ChimeraCharacter.Cast(GetControlledEntity());
		if (!character || GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
			return "UNAVAILABLE playing character";
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return "UNAVAILABLE character controller";

		if (action == "stance")
			controller.SetStanceChange(value.ToInt());
		else if (action == "lean")
			SCR_CharacterControllerComponent.Cast(controller).GC_TestHoldSpectatorPose(action, value.ToFloat());
		else if (action == "ads")
			SCR_CharacterControllerComponent.Cast(controller).GC_TestHoldSpectatorPose(action, value.ToFloat());
		else if (action == "raise")
			controller.SetWeaponRaised(value == "1");
		else if (action == "dynamic")
			controller.SetDynamicStance(value.ToFloat());
		else if (action == "move" || action == "pitch" || action == "turn")
			SCR_CharacterControllerComponent.Cast(controller).GC_TestHoldSpectatorPose(action, value.ToFloat());
		else
			return "REJECTED unknown action";

		return "DISPATCHED character input; observe on a later frame";
	}

	//------------------------------------------------------------------------------------------------
	static string GC_TestCharacterPose(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return "character=none";
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || !controller.GetInputContext() || !controller.GetAnimationComponent() || !character.GetAnimation())
			return "character=initializing";
		CharacterAnimationComponent animation = controller.GetAnimationComponent();
		CharacterMovementState movement = new CharacterMovementState();
		animation.GetMovementState(movement);
		vector head[4];
		character.GetAnimation().GetBoneMatrix(character.GetAnimation().GetBoneIndex("Head"), head);
		string result = string.Format("position=%1 stance=%2 lean=%3 inputLean=%4 ads=%5 animationADS=%6 movement=%7 aim=%8", character.GetOrigin(), controller.GetStance(), controller.GetCurrentLeanAmount(), controller.GetInputContext().GetLean(), controller.IsWeaponADS(), animation.IsWeaponADSTag(), movement.m_fMovement, controller.GetInputContext().GetAimingAngles());
		return result + string.Format(" headUp=%1 animationLean=%2 dynamic=%3", head[1], movement.m_fLeaning, controller.GetDynamicStance());
	}
}

//! Bounded owner-only input fixture; inert in other modes and remote proxies.
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_SpectatorPoseSeconds;
	protected float m_fGC_SpectatorLean;
	protected bool m_bGC_SpectatorADS;
	protected float m_fGC_SpectatorMove;
	protected float m_fGC_SpectatorPitch;
	protected float m_fGC_SpectatorTurn;
	protected float m_fGC_SpectatorMoveSeconds;

	//------------------------------------------------------------------------------------------------
	void GC_TestHoldSpectatorPose(string action, float value)
	{
		m_fGC_SpectatorPoseSeconds = 20;
		if (action == "lean")
			m_fGC_SpectatorLean = Math.Clamp(value, -1, 1);
		if (action == "ads")
			m_bGC_SpectatorADS = value > 0;
		if (action == "move")
		{
			m_fGC_SpectatorMove = Math.Clamp(value, 0, 2);
			m_fGC_SpectatorMoveSeconds = 3;
		}
		if (action == "pitch")
			m_fGC_SpectatorPitch = Math.Clamp(value, -45, 45);
		if (action == "turn")
			m_fGC_SpectatorTurn = Math.Clamp(value, -90, 90);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			m_fGC_SpectatorPoseSeconds = 0;
		if (m_fGC_SpectatorPoseSeconds > 0 && am && player && GetGame().GetPlayerController() && GetGame().GetPlayerController().GetControlledEntity() == owner)
		{
			m_fGC_SpectatorPoseSeconds -= dt;
			float holding = 0;
			if (m_fGC_SpectatorPoseSeconds > 0)
				holding = 1;
			am.SetActionValue("CharacterLeanLeft", holding * Math.Max(0, -m_fGC_SpectatorLean));
			am.SetActionValue("CharacterLeanRight", holding * Math.Max(0, m_fGC_SpectatorLean));
			float ads = 0;
			if (m_bGC_SpectatorADS)
				ads = holding;
			am.SetActionValue("CharacterWeaponADSHold", ads);
			m_fGC_SpectatorMoveSeconds -= dt;
			if (m_fGC_SpectatorMoveSeconds <= 0)
				m_fGC_SpectatorMove = 0;
			am.SetActionValue("CharacterForward", holding * Math.Clamp(m_fGC_SpectatorMove, 0, 1));
			am.SetActionValue("CharacterTurnRight", holding * m_fGC_SpectatorTurn * dt);
			am.SetActionValue("CharacterAimRight", holding * Math.Max(0, m_fGC_SpectatorTurn) * dt);
			am.SetActionValue("CharacterAimLeft", holding * Math.Max(0, -m_fGC_SpectatorTurn) * dt);
			vector aim = GetInputContext().GetAimingAngles();
			float pitchError = m_fGC_SpectatorPitch - aim[1] * Math.RAD2DEG;
			am.SetActionValue("CharacterTurnUp", holding * Math.Clamp(pitchError, -1, 1));
			am.SetActionValue("CharacterAimUp", holding * Math.Clamp(pitchError, 0, 1));
			am.SetActionValue("CharacterAimDown", holding * Math.Clamp(-pitchError, 0, 1));
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_SpectatorManualCamera
{
	//------------------------------------------------------------------------------------------------
	string GC_TestNetworkSnapshot()
	{
		string result = string.Format("mode=%1 target=%2 received=%3 sequence=%4 ads=%5 age=%6 proxy=[%7]", m_eMode, m_iTargetPlayerId, m_bNetworkSnapshotReceived, m_iNetworkLastSequence, m_bNetworkIsADS, m_fNetworkInterpolationElapsed, GC_LobbyPlayerController.GC_TestCharacterPose(m_Target));
		if (m_NetworkSample)
			result += string.Format(" sample=[version=%1 time=%2 position=%3 stance=%4 lean=%5 movement=%6 flags=%7 aim=%8 offset=%9]", m_NetworkSample.m_iVersion, m_NetworkSample.m_fSampleTime, m_NetworkSample.m_vPosition, m_NetworkSample.m_iStance, m_NetworkSample.m_fLean, m_NetworkSample.m_fMovement, m_NetworkSample.m_iFlags, m_NetworkSample.m_vAimAngles, m_NetworkSample.m_vCameraOffset);
		return result;
	}
}
