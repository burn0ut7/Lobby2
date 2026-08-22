enum GC_ESpectatorCameraMode
{
	FREE,
	SHOULDER,
	EYE,
	FOLLOW
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

	bool TrySetMode(GC_ESpectatorCameraMode mode, IEntity target = null)
	{
		if (mode == GC_ESpectatorCameraMode.FREE)
		{
			SetFreeMode();
			return true;
		}

		if ((mode != GC_ESpectatorCameraMode.SHOULDER && mode != GC_ESpectatorCameraMode.EYE && mode != GC_ESpectatorCameraMode.FOLLOW) || !IsEligibleTarget(target))
			return false;

		SCR_AttachManualCameraComponent attachComponent = SCR_AttachManualCameraComponent.Cast(FindCameraComponent(SCR_AttachManualCameraComponent));
		if (attachComponent)
			attachComponent.Detach();
		else if (mode == GC_ESpectatorCameraMode.FOLLOW)
			return false;

		RestoreEyeTargetVisibility();

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

		if (mode == GC_ESpectatorCameraMode.EYE)
		{
			m_EyeCameraHandler = SCR_CharacterCameraHandlerComponent.Cast(target.FindComponent(SCR_CharacterCameraHandlerComponent));
			if (m_EyeCameraHandler)
				m_EyeCameraHandler.OnAlphatestChange(255);
		}

		UpdateAttachedTransform();
		PrintFormat("GC | Spectator Camera | attached mode entered: mode=%1 target=%2 player=%3", mode, target, m_iTargetPlayerId);
		return true;
	}

	void SetFreeMode()
	{
		if (m_eMode == GC_ESpectatorCameraMode.FREE && !m_Target && !m_EyeCameraHandler)
			return;

		RestoreEyeTargetVisibility();
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

	protected void RestoreEyeTargetVisibility()
	{
		if (m_EyeCameraHandler)
			m_EyeCameraHandler.OnAlphatestChange(0);

		m_EyeCameraHandler = null;
	}

	void ~GC_SpectatorManualCamera()
	{
		RestoreEyeTargetVisibility();
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

		UpdateAttachedTransform();
	}

	protected void UpdateAttachedTransform()
	{
		Animation animation = m_Target.GetAnimation();
		if (!animation)
		{
			SetFreeMode();
			return;
		}

		string positionBoneName = "Camera";
		if (m_eMode == GC_ESpectatorCameraMode.EYE)
			positionBoneName = "leftEye";

		vector positionBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex(positionBoneName), positionBoneTransform))
		{
			SetFreeMode();
			return;
		}

		vector targetTransform[4];
		m_Target.GetWorldTransform(targetTransform);

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

		// The eye bone supplies the optical position, but its animated basis is not
		// camera-aligned. Match the proven first-person spectator calculation by
		// taking rotation from Head and position from leftEye.
		if (m_eMode == GC_ESpectatorCameraMode.EYE)
		{
			cameraTransform[3] = positionBoneWorldTransform[3];
			SetTransform(cameraTransform);
			return;
		}

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
