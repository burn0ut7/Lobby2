enum GC_ESpectatorCameraMode
{
	FREE,
	SHOULDER,
	EYE,
	FOLLOW,
	MIRROR
}

class GC_SpectatorManualCameraClass : SCR_ManualCameraClass
{
}

//! Lobby2-owned manual camera and canonical owner of client-local spectator modes.
//! The inherited ManualCameraBase component stack continues to own movement,
//! rotation, zoom and Follow attachment. Fixed-view modes apply their animated
//! world transform after that stack.
[EntityEditorProps(category: "GameScripted/Camera", description: "Lobby2 spectator manual camera", color: "0 255 255 255")]
class GC_SpectatorManualCamera : SCR_ManualCamera
{
	[Attribute("0.3", desc: "Helmet-camera lateral offset along the Camera bone's negative X axis.")]
	protected float m_fShoulderOffsetLateral;

	[Attribute("0.2", desc: "Helmet-camera distance behind the corrected Camera bone direction.")]
	protected float m_fShoulderOffsetBack;

	[Attribute("0.2", desc: "Helmet-camera vertical offset along the head's up axis.")]
	protected float m_fShoulderOffsetUp;

	protected GC_ESpectatorCameraMode m_eMode = GC_ESpectatorCameraMode.FREE;
	protected IEntity m_Target;
	protected int m_iTargetPlayerId = -1;
	protected int m_iManualInputReleaseFrames;
	protected SCR_MovementInertiaManualCameraComponent m_MovementInertia;
	protected SCR_RotationInertiaManualCameraComponent m_RotationInertia;
	protected SCR_CharacterCameraHandlerComponent m_EyeCameraHandler;
	protected bool m_bMovementInertiaWasEnabled;
	protected bool m_bRotationInertiaWasEnabled;
	protected bool m_bMirrorWasADS;
	protected BaseWeaponComponent m_MirrorADSWeapon;
	protected bool m_bMirrorActivatedSight;
	protected SCR_2DPIPSightsComponent m_MirrorPIPSights;
	protected CameraBase m_MirrorOverlayCamera;
	protected bool m_bMirrorDiagnosticsLogged;
	protected float m_fMirrorPreviousFOV;
	protected float m_fMirrorPreviousNearPlane;
	protected bool m_bMirrorFOVOverrideActive;
	protected bool m_bMirrorPositionStabilized;
	protected bool m_bMirrorRotationStabilized;
	protected vector m_vMirrorSmoothedLocalPosition;
	protected vector m_vMirrorSmoothedLocalForward;
	protected static AnimationTagID s_MirrorFPCameraTag = -1;
	protected static const float MIRROR_POSITION_STABILIZATION_SPEED = 6.0;
	protected static const float MIRROR_ROTATION_STABILIZATION_SPEED = 10.0;

	bool TrySetMode(GC_ESpectatorCameraMode mode, IEntity target = null)
	{
		if (mode == GC_ESpectatorCameraMode.FREE)
		{
			SetFreeMode();
			return true;
		}

		if ((mode != GC_ESpectatorCameraMode.SHOULDER && mode != GC_ESpectatorCameraMode.EYE && mode != GC_ESpectatorCameraMode.FOLLOW && mode != GC_ESpectatorCameraMode.MIRROR) || !IsEligibleTarget(target))
			return false;

		SCR_AttachManualCameraComponent attachComponent = SCR_AttachManualCameraComponent.Cast(FindCameraComponent(SCR_AttachManualCameraComponent));
		if (attachComponent)
			attachComponent.Detach();
		else if (mode == GC_ESpectatorCameraMode.FOLLOW)
			return false;

		RestoreAttachedPresentation();

		// Follow uses the inherited manual-camera attachment pipeline. It keeps the
		// camera's current world orientation and offset, then applies movement in the
		// target-relative space without treating manual input as a detach request.
		if (mode == GC_ESpectatorCameraMode.FOLLOW)
		{
			if (m_MovementInertia)
				m_MovementInertia.SetEnabled(m_bMovementInertiaWasEnabled);
			if (m_RotationInertia)
				m_RotationInertia.SetEnabled(m_bRotationInertiaWasEnabled);
			m_MovementInertia = null;
			m_RotationInertia = null;

			m_Target = target;
			m_eMode = mode;
			m_iTargetPlayerId = -1;
			m_iManualInputReleaseFrames = 0;
			PlayerManager followPlayerManager = GetGame().GetPlayerManager();
			if (followPlayerManager)
				m_iTargetPlayerId = followPlayerManager.GetPlayerIdFromControlledEntity(target);

			if (!attachComponent.AttachTo(target))
			{
				SetFreeMode();
				return false;
			}

			PrintFormat("GC | Spectator Camera | follow mode entered: target=%1 player=%2", target, m_iTargetPlayerId);
			return true;
		}

		if (m_eMode == GC_ESpectatorCameraMode.FREE || m_eMode == GC_ESpectatorCameraMode.FOLLOW)
		{
			m_MovementInertia = SCR_MovementInertiaManualCameraComponent.Cast(FindCameraComponent(SCR_MovementInertiaManualCameraComponent));
			if (m_MovementInertia)
			{
				m_bMovementInertiaWasEnabled = m_MovementInertia.IsEnabled() != 0;
				m_MovementInertia.SetEnabled(false);
			}

			m_RotationInertia = SCR_RotationInertiaManualCameraComponent.Cast(FindCameraComponent(SCR_RotationInertiaManualCameraComponent));
			if (m_RotationInertia)
			{
				m_bRotationInertiaWasEnabled = m_RotationInertia.IsEnabled() != 0;
				m_RotationInertia.SetEnabled(false);
			}
		}

		m_Target = target;
		m_eMode = mode;
		m_iTargetPlayerId = -1;
		m_iManualInputReleaseFrames = 0;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
			m_iTargetPlayerId = playerManager.GetPlayerIdFromControlledEntity(target);

		if (mode == GC_ESpectatorCameraMode.EYE || mode == GC_ESpectatorCameraMode.MIRROR)
		{
			m_EyeCameraHandler = SCR_CharacterCameraHandlerComponent.Cast(target.FindComponent(SCR_CharacterCameraHandlerComponent));
			if (m_EyeCameraHandler)
				m_EyeCameraHandler.OnAlphatestChange(255);
		}

		if (mode == GC_ESpectatorCameraMode.MIRROR)
		{
			m_fMirrorPreviousFOV = GetVerticalFOV();
			m_fMirrorPreviousNearPlane = GetNearPlane();
			m_bMirrorFOVOverrideActive = true;
			SetNearPlane(0.0125);
			m_bMirrorPositionStabilized = false;
			m_bMirrorRotationStabilized = false;
			m_bMirrorDiagnosticsLogged = false;
			CharacterAnimationComponent mirrorAnimation = CharacterAnimationComponent.Cast(target.FindComponent(CharacterAnimationComponent));
			m_bMirrorWasADS = mirrorAnimation && mirrorAnimation.IsWeaponADSTag();
			PrintFormat("GC | Spectator Camera | mirror state: ads=%1 target=%2 player=%3", m_bMirrorWasADS, target, m_iTargetPlayerId);
		}

		UpdateAttachedTransform(0);
		if (m_eMode != mode || m_Target != target)
			return false;

		PrintFormat("GC | Spectator Camera | attached mode entered: mode=%1 target=%2 player=%3", mode, target, m_iTargetPlayerId);
		return true;
	}

	void SetFreeMode()
	{
		if (m_eMode == GC_ESpectatorCameraMode.FREE && !m_Target && !m_EyeCameraHandler)
			return;

		RestoreAttachedPresentation();
		SCR_AttachManualCameraComponent attachComponent = SCR_AttachManualCameraComponent.Cast(FindCameraComponent(SCR_AttachManualCameraComponent));
		if (attachComponent)
			attachComponent.Detach();

		m_eMode = GC_ESpectatorCameraMode.FREE;
		m_Target = null;
		m_iTargetPlayerId = -1;
		m_iManualInputReleaseFrames = 0;

		if (m_MovementInertia)
			m_MovementInertia.SetEnabled(m_bMovementInertiaWasEnabled);
		if (m_RotationInertia)
			m_RotationInertia.SetEnabled(m_bRotationInertiaWasEnabled);
		m_MovementInertia = null;
		m_RotationInertia = null;
		Print("GC | Spectator Camera | free camera entered");
	}

	protected void RestoreAttachedPresentation()
	{
		RestoreMirrorSightPresentation();
		if (m_bMirrorFOVOverrideActive)
		{
			SetVerticalFOV(m_fMirrorPreviousFOV);
			SetNearPlane(m_fMirrorPreviousNearPlane);
			m_bMirrorFOVOverrideActive = false;
		}

		if (m_EyeCameraHandler)
			m_EyeCameraHandler.OnAlphatestChange(0);

		m_EyeCameraHandler = null;

		m_bMirrorWasADS = false;
		m_bMirrorDiagnosticsLogged = false;
		m_bMirrorPositionStabilized = false;
		m_bMirrorRotationStabilized = false;
	}

	protected void RestoreMirrorSightPresentation()
	{
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (m_MirrorOverlayCamera && cameraManager && cameraManager.GetOverlayCamera() == m_MirrorOverlayCamera)
			cameraManager.SetOverlayCamera(null);

		m_MirrorOverlayCamera = null;

		if (m_MirrorPIPSights && m_MirrorPIPSights.IsPIPEnabled())
			m_MirrorPIPSights.SetPIPEnabled(false);

		m_MirrorPIPSights = null;

		if (m_MirrorADSWeapon && m_bMirrorActivatedSight && m_MirrorADSWeapon.IsSightADSActive())
			m_MirrorADSWeapon.SightADSDeactivated();

		m_MirrorADSWeapon = null;
		m_bMirrorActivatedSight = false;
	}

	protected void SetMirrorOverlayCamera(CameraBase overlayCamera)
	{
		if (m_MirrorOverlayCamera == overlayCamera)
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
		{
			m_MirrorOverlayCamera = null;
			return;
		}

		if (m_MirrorOverlayCamera && cameraManager.GetOverlayCamera() == m_MirrorOverlayCamera)
			cameraManager.SetOverlayCamera(null);

		m_MirrorOverlayCamera = overlayCamera;
		if (m_MirrorOverlayCamera)
			cameraManager.SetOverlayCamera(m_MirrorOverlayCamera);
	}

	void ~GC_SpectatorManualCamera()
	{
		RestoreAttachedPresentation();
	}

	GC_ESpectatorCameraMode GetMode()
	{
		return m_eMode;
	}

	IEntity GetTarget()
	{
		return m_Target;
	}

	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);

		if (m_eMode == GC_ESpectatorCameraMode.FREE)
			return;

		if (!IsEligibleTarget(m_Target))
		{
			SetFreeMode();
			return;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (m_iTargetPlayerId > 0 && (!playerManager || playerManager.GetPlayerControlledEntity(m_iTargetPlayerId) != m_Target))
		{
			SetFreeMode();
			return;
		}

		// The native attachment component has already updated the camera before this
		// entity post-frame. Follow deliberately accepts movement and look input.
		if (m_eMode == GC_ESpectatorCameraMode.FOLLOW)
			return;

		// Context-menu mouse input can remain in the native manual-camera pipeline
		// for a few frames after activation. Arm cancellation only after two clean
		// frames, then use the base camera's canonical manual-input signal.
		bool manualInput = IsManualInput();
		if (m_iManualInputReleaseFrames < 2)
		{
			if (manualInput)
				m_iManualInputReleaseFrames = 0;
			else
				m_iManualInputReleaseFrames++;
		}
		else if (manualInput)
		{
			SetFreeMode();
			return;
		}

		UpdateAttachedTransform(timeSlice);
	}

	protected void UpdateAttachedTransform(float timeSlice)
	{
		if (m_eMode == GC_ESpectatorCameraMode.MIRROR)
		{
			UpdateMirrorTransform(timeSlice);
			return;
		}

		Animation animation = m_Target.GetAnimation();
		if (!animation)
		{
			SetFreeMode();
			return;
		}

		vector targetTransform[4];
		m_Target.GetWorldTransform(targetTransform);

		if (m_eMode == GC_ESpectatorCameraMode.EYE)
		{
			vector eyeCameraTransform[4];
			if (!GetEyeCameraTransform(animation, targetTransform, eyeCameraTransform))
			{
				SetFreeMode();
				return;
			}

			SetTransform(eyeCameraTransform);
			return;
		}

		vector positionBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex("Camera"), positionBoneTransform))
		{
			SetFreeMode();
			return;
		}

		vector positionBoneWorldTransform[4];
		Math3D.MatrixMultiply4(targetTransform, positionBoneTransform, positionBoneWorldTransform);

		// Character bones and cameras face along opposite Z axes. This conversion
		// changes only the basis; attached modes add no aim angles or look-at logic.
		vector headBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex("Head"), headBoneTransform))
		{
			SetFreeMode();
			return;
		}

		vector headWorldTransform[4];
		Math3D.MatrixMultiply4(targetTransform, headBoneTransform, headWorldTransform);
		vector cameraAxisTransform[3] = { "1 0 0", "0 1 0", "0 0 -1" };
		vector cameraTransform[4];
		Math3D.MatrixMultiply3(headWorldTransform, cameraAxisTransform, cameraTransform);

		cameraTransform[3] = positionBoneWorldTransform[3]
			+ cameraTransform[1] * m_fShoulderOffsetUp
			- cameraTransform[2] * m_fShoulderOffsetBack
			- cameraTransform[0] * m_fShoulderOffsetLateral;

		// SCR_ManualCamera processes and stores a local transform. This camera is a
		// root entity, so its local transform is also the authored world transform.
		// Apply immediately every frame; Lobby2 intentionally keeps no target-switch
		// easing because spectator camera interpolation is explicitly disabled.
		SetTransform(cameraTransform);
	}

	//! Remote character controllers do not expose their owner-local look input.
	//! The animated head and eye bones are the authoritative presentation that is
	//! actually available on a spectator proxy.
	protected bool GetEyeCameraTransform(Animation animation, vector targetTransform[4], out vector cameraTransform[4])
	{
		vector eyeBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex("leftEye"), eyeBoneTransform))
			return false;

		vector eyeBoneWorldTransform[4];
		Math3D.MatrixMultiply4(targetTransform, eyeBoneTransform, eyeBoneWorldTransform);

		vector headBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex("Head"), headBoneTransform))
			return false;

		vector headWorldTransform[4];
		Math3D.MatrixMultiply4(targetTransform, headBoneTransform, headWorldTransform);
		vector cameraAxisTransform[3] = { "1 0 0", "0 1 0", "0 0 -1" };
		Math3D.MatrixMultiply3(headWorldTransform, cameraAxisTransform, cameraTransform);
		cameraTransform[3] = eyeBoneWorldTransform[3];
		return true;
	}

	//! Reconstructs the remote first-person transform from state available on a
	//! non-owning proxy. The dynamic Camera/Head bone supplies only attachment
	//! position using the same offset and direct-bone mode as the stock first-person
	//! camera. The animated Head bone supplies the complete camera rotation so remote
	//! freelook follows the presented head instead of unavailable owner-local input.
	protected bool GetMirrorFirstPersonTransform(Animation animation, CharacterAnimationComponent characterAnimation, vector targetTransform[4], float timeSlice, out vector cameraTransform[4])
	{
		if (s_MirrorFPCameraTag == -1)
			s_MirrorFPCameraTag = GameAnimationUtils.RegisterAnimationTag("TagFPCamera");

		string anchorBoneName = "Head";
		if (characterAnimation.IsPrimaryTag(s_MirrorFPCameraTag))
			anchorBoneName = "Camera";

		TNodeId anchorBoneIndex = animation.GetBoneIndex(anchorBoneName);
		if (anchorBoneIndex == -1)
			return false;

		CharacterHeadAimingComponent headAiming = CharacterHeadAimingComponent.Cast(m_Target.FindComponent(CharacterHeadAimingComponent));
		if (!headAiming)
			return false;

		vector stockFirstPersonTransform[4];
		vector stockOffset = "0 0.03 -0.07";
		vector zeroRotation = vector.Zero;
		headAiming.GetLookTransformationLS(anchorBoneIndex, EDirectBoneMode.RelativePosition, stockOffset, zeroRotation, stockFirstPersonTransform);

		// Match CharacterCamera1stPerson's character-pitch composition before the
		// native camera pipeline attaches the result to the selected direct bone.
		vector characterPitch = vector.Zero;
		characterPitch[1] = m_Target.GetLocalYawPitchRoll()[1];
		vector characterPitchTransform[4];
		Math3D.AnglesToMatrix(characterPitch, characterPitchTransform);
		Math3D.MatrixMultiply4(characterPitchTransform, stockFirstPersonTransform, stockFirstPersonTransform);

		// CharacterCamera1stPerson returns this transform relative to its selected
		// direct bone. The native player-camera pipeline applies that bone later;
		// mirror mode must perform the same composition explicitly.
		vector anchorBoneTransform[4];
		if (!animation.GetBoneMatrix(anchorBoneIndex, anchorBoneTransform))
			return false;

		vector stockFirstPersonModelTransform[4];
		Math3D.MatrixMultiply4(anchorBoneTransform, stockFirstPersonTransform, stockFirstPersonModelTransform);

		vector stockLocalPosition = stockFirstPersonModelTransform[3];
		StabilizeMirrorLocalPosition(stockLocalPosition, timeSlice);
		cameraTransform[3] = targetTransform[3]
			+ targetTransform[0] * stockLocalPosition[0]
			+ targetTransform[1] * stockLocalPosition[1]
			+ targetTransform[2] * stockLocalPosition[2];
		return ApplyMirrorHeadRotation(animation, targetTransform, timeSlice, cameraTransform);
	}

	//! Filters only model-space animation displacement. Character world movement is
	//! composed afterward, so stabilization reduces proxy head bob without making
	//! the spectator camera trail behind the observed player.
	protected void StabilizeMirrorLocalPosition(inout vector localPosition, float timeSlice)
	{
		if (!m_bMirrorPositionStabilized || timeSlice <= 0)
		{
			m_vMirrorSmoothedLocalPosition = localPosition;
			m_bMirrorPositionStabilized = true;
		}
		else
		{
			float blend = Math.Clamp(timeSlice * MIRROR_POSITION_STABILIZATION_SPEED, 0.0, 1.0);
			m_vMirrorSmoothedLocalPosition = vector.Lerp(m_vMirrorSmoothedLocalPosition, localPosition, blend);
		}

		localPosition = m_vMirrorSmoothedLocalPosition;
	}

	//! Viewing direction intentionally comes from the animated Head bone in every
	//! mirror state. Remote owner-local look input is unavailable, while the filtered
	//! direction preserves visible freelook without inheriting the bone's animated roll.
	protected bool ApplyMirrorHeadRotation(Animation animation, vector targetTransform[4], float timeSlice, inout vector cameraTransform[4])
	{
		TNodeId headBoneIndex = animation.GetBoneIndex("Head");
		if (headBoneIndex == -1)
			return false;

		vector headBoneTransform[4];
		if (!animation.GetBoneMatrix(headBoneIndex, headBoneTransform))
			return false;

		vector cameraAxisTransform[3] = { "1 0 0", "0 1 0", "0 0 -1" };
		vector headCameraModelTransform[3];
		Math3D.MatrixMultiply3(headBoneTransform, cameraAxisTransform, headCameraModelTransform);

		vector localForward = headCameraModelTransform[2];
		if (!m_bMirrorRotationStabilized || timeSlice <= 0)
		{
			m_vMirrorSmoothedLocalForward = localForward;
			m_bMirrorRotationStabilized = true;
		}
		else
		{
			float blend = Math.Clamp(timeSlice * MIRROR_ROTATION_STABILIZATION_SPEED, 0.0, 1.0);
			m_vMirrorSmoothedLocalForward = vector.Lerp(m_vMirrorSmoothedLocalForward, localForward, blend);
			m_vMirrorSmoothedLocalForward.Normalize();
		}

		vector worldForward = targetTransform[0] * m_vMirrorSmoothedLocalForward[0]
			+ targetTransform[1] * m_vMirrorSmoothedLocalForward[1]
			+ targetTransform[2] * m_vMirrorSmoothedLocalForward[2];
		vector stableUp = targetTransform[1];
		if (Math.AbsFloat(vector.Dot(worldForward, stableUp)) > 0.98)
		{
			vector headWorldTransform[4];
			Math3D.MatrixMultiply4(targetTransform, headBoneTransform, headWorldTransform);
			stableUp = headWorldTransform[1];
		}

		vector stableRotation[4];
		Math3D.DirectionAndUpMatrix(worldForward, stableUp, stableRotation);
		cameraTransform[0] = stableRotation[0];
		cameraTransform[1] = stableRotation[1];
		cameraTransform[2] = stableRotation[2];
		return true;
	}

	//! Reconstructs the observed character's first-person presentation entirely on
	//! the spectator client. It reads the remote proxy's character and weapon state;
	//! it never sends input, changes possession, or publishes replicated state.
	protected void UpdateMirrorTransform(float timeSlice)
	{
		Animation animation = m_Target.GetAnimation();
		CharacterAnimationComponent characterAnimation = CharacterAnimationComponent.Cast(m_Target.FindComponent(CharacterAnimationComponent));
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!animation || !characterAnimation || !cameraManager)
		{
			SetFreeMode();
			return;
		}

		float mirrorFOV = cameraManager.GetFirstPersonFOV();
		bool isADS = characterAnimation.IsWeaponADSTag();
		if (isADS != m_bMirrorWasADS)
		{
			m_bMirrorWasADS = isADS;
			m_bMirrorDiagnosticsLogged = false;
			m_bMirrorPositionStabilized = false;
			m_bMirrorRotationStabilized = false;
			PrintFormat("GC | Spectator Camera | mirror state changed: ads=%1 target=%2 player=%3", isADS, m_Target, m_iTargetPlayerId);
		}

		vector cameraModelTransform[4];
		if (isADS)
		{
			BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(m_Target.FindComponent(BaseWeaponManagerComponent));
			BaseWeaponComponent weapon;
			if (weaponManager)
				weapon = weaponManager.GetCurrentWeapon();

			float sightsFOV;
			if (!weaponManager || !weapon || !weaponManager.GetCurrentSightsCameraTransform(cameraModelTransform, sightsFOV))
			{
				PrintFormat("GC | Spectator Camera | mirror ADS unavailable: target=%1 player=%2", m_Target, m_iTargetPlayerId, level: LogLevel.WARNING);
				SetFreeMode();
				return;
			}

			if (weapon != m_MirrorADSWeapon)
			{
				RestoreMirrorSightPresentation();
				m_MirrorADSWeapon = weapon;
			}

			if (!weapon.IsSightADSActive())
			{
				weapon.SightADSActivated();
				m_bMirrorActivatedSight = true;
				PrintFormat("GC | Spectator Camera | mirror sight activated: weapon=%1 target=%2 player=%3", weapon.GetOwner(), m_Target, m_iTargetPlayerId);
			}

			vector zeroingTransform[4];
			if (weapon.GetCurrentSightsZeroingTransform(zeroingTransform))
			{
				zeroingTransform[3] = -zeroingTransform[3];
				Math3D.MatrixMultiply4(cameraModelTransform, zeroingTransform, cameraModelTransform);
			}

			SightsComponent sights = weaponManager.GetCurrentSights();
			SCR_2DPIPSightsComponent pipSights = SCR_2DPIPSightsComponent.Cast(sights);
			if (pipSights)
			{
				// Stock CharacterCameraADS replaces the main-camera FOV for PiP
				// sights instead of applying the ordinary sight-FOV clamp.
				mirrorFOV = pipSights.GetMainCameraFOV();
				if (m_MirrorPIPSights != pipSights)
				{
					if (m_MirrorPIPSights && m_MirrorPIPSights.IsPIPEnabled())
						m_MirrorPIPSights.SetPIPEnabled(false);

					m_MirrorPIPSights = pipSights;
				}

				if (!pipSights.IsPIPEnabled())
					pipSights.SetPIPEnabled(true);

				SetMirrorOverlayCamera(pipSights.GetPIPCamera());
				cameraModelTransform[3] = cameraModelTransform[3] - pipSights.GetMainCameraOffset();
			}
			else
			{
				mirrorFOV = Math.Min(mirrorFOV, sightsFOV);
				if (m_MirrorPIPSights && m_MirrorPIPSights.IsPIPEnabled())
					m_MirrorPIPSights.SetPIPEnabled(false);

				m_MirrorPIPSights = null;
				SetMirrorOverlayCamera(null);
			}
		}
		else
		{
			RestoreMirrorSightPresentation();
			vector targetWorldTransform[4];
			m_Target.GetWorldTransform(targetWorldTransform);
			if (!GetMirrorFirstPersonTransform(animation, characterAnimation, targetWorldTransform, timeSlice, cameraModelTransform))
			{
				PrintFormat("GC | Spectator Camera | mirror first-person animation unavailable: target=%1 player=%2", m_Target, m_iTargetPlayerId, level: LogLevel.WARNING);
				SetFreeMode();
				return;
			}

			ApplyMirrorFocusFOV(mirrorFOV, timeSlice);
			SetVerticalFOV(mirrorFOV);
			SetTransform(cameraModelTransform);
			ApplyTransform(timeSlice);
			LogMirrorDiagnostics(cameraModelTransform, isADS);
			return;
		}

		vector targetWorldTransform[4];
		m_Target.GetWorldTransform(targetWorldTransform);
		vector stabilizedADSPosition = cameraModelTransform[3];
		StabilizeMirrorLocalPosition(stabilizedADSPosition, timeSlice);
		cameraModelTransform[3] = stabilizedADSPosition;
		vector cameraWorldTransform[4];
		Math3D.MatrixMultiply4(targetWorldTransform, cameraModelTransform, cameraWorldTransform);
		if (!ApplyMirrorHeadRotation(animation, targetWorldTransform, timeSlice, cameraWorldTransform))
		{
			SetFreeMode();
			return;
		}

		ApplyMirrorFocusFOV(mirrorFOV, timeSlice);

		SetVerticalFOV(mirrorFOV);
		SetTransform(cameraWorldTransform);
		ApplyTransform(timeSlice);
		LogMirrorDiagnostics(cameraWorldTransform, isADS);
	}

	protected void ApplyMirrorFocusFOV(inout float mirrorFOV, float timeSlice)
	{
		SCR_PlayerController localPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		PlayerCamera localPlayerCamera;
		if (localPlayerController)
			localPlayerCamera = localPlayerController.GetPlayerCamera();
		if (localPlayerCamera)
			mirrorFOV = localPlayerCamera.GetFocusFOVScale(mirrorFOV, localPlayerController.GetFocusValue(1, timeSlice));
	}

	protected void LogMirrorDiagnostics(vector cameraTransform[4], bool isADS)
	{
		if (m_bMirrorDiagnosticsLogged)
			return;

		m_bMirrorDiagnosticsLogged = true;
		CameraManager cameraManager = GetGame().GetCameraManager();
		CameraBase currentCamera;
		CameraBase overlayCamera;
		if (cameraManager)
		{
			currentCamera = cameraManager.CurrentCamera();
			overlayCamera = cameraManager.GetOverlayCamera();
		}

		PrintFormat("GC | Spectator Camera | mirror diagnostics: ads=%1 current=%2 self=%3 overlay=%4 headHidden=%5 fov=%6", isADS, currentCamera, this, overlayCamera, m_EyeCameraHandler != null, GetVerticalFOV());
		PrintFormat("GC | Spectator Camera | mirror transform: x=%1 y=%2 z=%3 position=%4 target=%5 player=%6", cameraTransform[0], cameraTransform[1], cameraTransform[2], cameraTransform[3], m_Target, m_iTargetPlayerId);
	}

	protected bool IsEligibleTarget(IEntity target)
	{
		if (!SCR_ChimeraCharacter.Cast(target) || target.FindComponent(GC_LobbySpectatorBodyComponent))
			return false;

		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(target.FindComponent(SCR_CharacterControllerComponent));
		if (!characterController || characterController.IsDead())
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		return !playerManager || playerManager.GetPlayerIdFromControlledEntity(target) <= 0 || !characterController.IsUnconscious();
	}
}

//! Camera-mode context action registered only for the local spectator lifetime.
class GC_SpectatorCameraContextAction : GC_ContextMenuAction
{
	protected GC_SpectatorManualCamera m_Camera;
	protected GC_ESpectatorCameraMode m_eMode;
	protected string m_sLabel;

	void GC_SpectatorCameraContextAction(GC_SpectatorManualCamera camera, GC_ESpectatorCameraMode mode, string label)
	{
		m_Camera = camera;
		m_eMode = mode;
		m_sLabel = label;
	}

	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "CAMERA";
	}

	override string GetLabel(GC_ContextMenuContext context)
	{
		return m_sLabel;
	}

	override bool IsVisible(GC_ContextMenuContext context)
	{
		return ResolveEligibleTarget(context) != null;
	}

	override bool CanExecute(GC_ContextMenuContext context)
	{
		return m_Camera && ResolveEligibleTarget(context) != null;
	}

	override void Execute(GC_ContextMenuContext context)
	{
		if (!m_Camera)
			return;

		IEntity target = ResolveEligibleTarget(context);
		if (target)
			m_Camera.TrySetMode(m_eMode, target);
	}

	protected IEntity ResolveEligibleTarget(GC_ContextMenuContext context)
	{
		if (!context)
			return null;

		IEntity target;
		if (context.GetTargetType() == GC_EContextTargetType.PLAYER)
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
				target = playerManager.GetPlayerControlledEntity(context.GetPlayerId());
		}
		else if (context.GetTargetType() == GC_EContextTargetType.ENTITY)
		{
			target = context.GetEntity();
		}

		if (!SCR_ChimeraCharacter.Cast(target) || target.FindComponent(GC_LobbySpectatorBodyComponent))
			return null;

		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(target.FindComponent(SCR_CharacterControllerComponent));
		if (!characterController || characterController.IsDead())
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager && playerManager.GetPlayerIdFromControlledEntity(target) > 0 && characterController.IsUnconscious())
			return null;

		return target;
	}
}
