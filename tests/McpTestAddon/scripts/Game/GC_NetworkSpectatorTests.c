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
		sample.m_iVersion = 1;
		if (sample.IsValid())
			return "FAIL version accepted";
		sample.m_iVersion = 2;
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
		sample.m_fOpticReticleScale = -1;
		if (sample.IsValid())
			return "FAIL negative reticle scale accepted";
		sample.m_fOpticReticleScale = 0;
		sample.m_fOpticEdgeScale = 100;
		if (sample.IsValid())
			return "FAIL unbounded edge scale accepted";
		sample.m_fOpticEdgeScale = 0;
		if (!sample.IsValid())
			return "FAIL boundary sample";
		return "PASS valid and boundary samples; rejected version, lean, stance, basis, FOV, position, optic scales";
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
	protected int m_iGC_CameraProbeRemaining;
	protected string m_sGC_CameraProbe;

	//------------------------------------------------------------------------------------------------
	string GC_TestNetworkSpectator(string action, string value)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return "UNAVAILABLE owning Lobby2 player";

		if (action == "measure")
		{
			if (m_iGC_CameraProbeRemaining > 0)
				return "UNAVAILABLE probe active";
			m_iGC_CameraProbeRemaining = 120;
			m_sGC_CameraProbe = "frame,fps,frameMs,simMs,renderMs,gpuMs,renderErrorMm,sightX,sightY,sightZ,mainFOV,pipFOV\n";
			GetGame().GetCallqueue().CallLater(GC_TestCameraProbeSample, 16);
			return "STARTED 120 observations at minimum 16ms intervals";
		}
		if (action == "measurement")
		{
			int offset = Math.Clamp(value.ToInt(), 0, m_sGC_CameraProbe.Length());
			int count = Math.Min(1500, m_sGC_CameraProbe.Length() - offset);
			return string.Format("remaining=%1 length=%2 offset=%3\n%4", m_iGC_CameraProbeRemaining, m_sGC_CameraProbe.Length(), offset, m_sGC_CameraProbe.Substring(offset, count));
		}
		if (action == "optic")
		{
			IEntity target = GetControlledEntity();
			if (m_SpectatorCamera && m_SpectatorCamera.GetMode() == GC_ESpectatorCameraMode.NETWORK)
				target = m_SpectatorCamera.GetTarget();
			if (!target)
				return "UNAVAILABLE target";
			BaseWeaponManagerComponent weapons = BaseWeaponManagerComponent.Cast(target.FindComponent(BaseWeaponManagerComponent));
			if (!weapons || !weapons.GetCurrentWeapon())
				return "UNAVAILABLE weapon";
			SCR_2DPIPSightsComponent optic = SCR_2DPIPSightsComponent.Cast(weapons.GetCurrentWeapon().GetSightsAt(weapons.GetCurrentSightsIndex()));
			if (!optic)
				return "UNAVAILABLE optic";
			return optic.GC_TestPIPUpdates();
		}

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
		if (action == "sight" || action == "weapon")
		{
			BaseWeaponManagerComponent weapons = controller.GetWeaponManagerComponent();
			if (!weapons || !weapons.GetCurrentWeapon())
				return "UNAVAILABLE weapon";
			int index = value.ToInt();
			if (action == "sight")
			{
				if (index < 0 || index > 3 || !weapons.GetCurrentWeapon().GetSightsAt(index))
					return "UNAVAILABLE sight index";
				return string.Format("selected=%1", weapons.GetCurrentWeapon().SetSights(index));
			}
			array<IEntity> inventoryWeapons = {};
			weapons.GetWeaponsList(inventoryWeapons);
			if (index < 0 || index >= inventoryWeapons.Count())
				return "UNAVAILABLE weapon index";
			BaseWeaponComponent selectedWeapon = BaseWeaponComponent.Cast(inventoryWeapons[index].FindComponent(BaseWeaponComponent));
			if (!selectedWeapon)
				return "UNAVAILABLE weapon component";
			return string.Format("selected=%1 count=%2", controller.SelectWeapon(selectedWeapon), inventoryWeapons.Count());
		}

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
		else if (action == "zeroing")
			controller.SetSightsRange(Math.Clamp(value.ToInt(), 0, 3));
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

	//------------------------------------------------------------------------------------------------
	//! Bounded observations only; native ProfileData values are rolling averages.
	protected void GC_TestCameraProbeSample()
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
		{
			m_iGC_CameraProbeRemaining = 0;
			return;
		}
		CameraManager manager = GetGame().GetCameraManager();
		CameraBase camera = manager.CurrentCamera();
		IEntity target = GetControlledEntity();
		if (m_SpectatorCamera && m_SpectatorCamera.GetMode() == GC_ESpectatorCameraMode.NETWORK)
			target = m_SpectatorCamera.GetTarget();
		if (camera && target)
		{
			vector actual[4], rendered[4], targetWorld[4], sights[4], sightWorld[4], relative[4];
			camera.GetWorldTransform(actual);
			camera.GetWorldCameraTransform(rendered);
			target.GetWorldTransform(targetWorld);
			BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(target.FindComponent(BaseWeaponManagerComponent));
			float sightFOV;
			if (weaponManager && weaponManager.GetCurrentSightsCameraTransform(sights, sightFOV))
			{
				Math3D.MatrixMultiply4(targetWorld, sights, sightWorld);
				Math3D.MatrixInvMultiply4(actual, sightWorld, relative);
			}
			float pipFOV = -1;
			if (manager.GetOverlayCamera())
				pipFOV = manager.GetOverlayCamera().GetVerticalFOV();
			ProfileData profile = System.GetProfileData();
			m_sGC_CameraProbe += string.Format("%1,%2,%3,%4,%5,%6,", GetGame().GetWorld().GetFrameNumber(), profile.FPS, profile.FrameTimeMs, profile.SimulationTimeMs, profile.RenderTimeMs, profile.GPUTimeMs);
			m_sGC_CameraProbe += string.Format("%1,%2,%3,%4,%5,%6\n", vector.Distance(actual[3], rendered[3]) * 1000, relative[3][0], relative[3][1], relative[3][2], camera.GetVerticalFOV(), pipFOV);
		}
		m_iGC_CameraProbeRemaining--;
		if (m_iGC_CameraProbeRemaining > 0)
			GetGame().GetCallqueue().CallLater(GC_TestCameraProbeSample, 16);
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
		{
			result += string.Format(" sample=[version=%1 time=%2 position=%3 stance=%4 lean=%5 movement=%6 flags=%7 aim=%8 offset=%9]", m_NetworkSample.m_iVersion, m_NetworkSample.m_fSampleTime, m_NetworkSample.m_vPosition, m_NetworkSample.m_iStance, m_NetworkSample.m_fLean, m_NetworkSample.m_fMovement, m_NetworkSample.m_iFlags, m_NetworkSample.m_vAimAngles, m_NetworkSample.m_vCameraOffset);
			result += string.Format(" lens=%1 pip=%2", m_NetworkSample.m_vLensState, m_fNetworkPIPFOV);
		}
		if (m_MirrorPIPSights)
			result += " " + m_MirrorPIPSights.GC_TestPIPUpdates();
		return result;
	}
}

modded class SCR_2DPIPSightsComponent
{
	protected int m_iGC_NativeUpdates;
	protected int m_iGC_NetworkUpdates;
	protected string m_sGC_LastPIPUpdate;

	//------------------------------------------------------------------------------------------------
	override void UpdateCamera(float timeSlice)
	{
		bool networkOwned = GC_IsNetworkPIP();
		super.UpdateCamera(timeSlice);
		if (networkOwned)
			return;
		m_iGC_NativeUpdates++;
		m_sGC_LastPIPUpdate = "native";
	}

	//------------------------------------------------------------------------------------------------
	override void GC_UpdateSpectatorPIP(float timeSlice, float authoritativePIPFOV, float reticleScale, float edgeScale)
	{
		super.GC_UpdateSpectatorPIP(timeSlice, authoritativePIPFOV, reticleScale, edgeScale);
		m_iGC_NetworkUpdates++;
		m_sGC_LastPIPUpdate = "network";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPIPUpdates()
	{
		string result = string.Format("updatesNative=%1 network=%2 last=%3 mainFOV=%4", m_iGC_NativeUpdates, m_iGC_NetworkUpdates, m_sGC_LastPIPUpdate, m_fMainCameraFOV);
		result += string.Format(" radius=%1 objectiveFOV=%2 objectiveScale=%3 edgeScale=%4 reticleScale=%5 reticlePortion=%6 reticleAngular=%7 parallax=%8,%9", m_fScopeRadius, m_fObjectiveFov, m_fObjectiveScale, m_fEdgeScale, m_fReticleScale, m_fReticlePortion, m_fReticleAngularSize, m_fParallaxX, m_fParallaxY);
		result += string.Format(" zeroReticle=%1 zeroPitch=%2", m_fCurrentReticleOffsetY, m_fCurrentCameraPitch);
		if (m_PIPCamera)
			result += string.Format(" pipPosition=%1 pipFOV=%2 pipIndex=%3", m_PIPCamera.GetOrigin(), m_PIPCamera.GetVerticalFOV(), m_PIPCamera.GetCameraIndex());
		return result;
	}
}
