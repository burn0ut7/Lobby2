//! One owner-observed presentation sample. Character and camera values share a
//! sequence and sample time; the server routes it only to authorized spectators.
//! Positions are world metres, directions world-aligned, aim/look angles radians,
//! time world milliseconds and lens values degrees. This is presentation evidence,
//! never an authority update to another player's character.
class GC_SpectatorSample
{
	int m_iVersion = 2;
	int m_iSequence;
	float m_fSampleTime;
	int m_iStance;
	int m_iCommand;
	int m_iFlags;
	int m_iSightState;
	vector m_vPosition;
	vector m_vForward;
	vector m_vUp;
	vector m_vVelocity;
	vector m_vAimAngles;
	vector m_vLookAngles;
	float m_fLean;
	float m_fDynamicStance;
	float m_fMovement;
	//! ADS camera basis/offset are sight-local; otherwise world-aligned from root.
	//! Version 2 changes this frame and adds the two stock optic scales.
	vector m_vCameraOffset;
	vector m_vCameraForward;
	vector m_vCameraUp;
	vector m_vLensState;
	float m_fOpticReticleScale = 1;
	float m_fOpticEdgeScale = 1;

	static const int VERSION = 2;
	static const int FLAG_ADS = 1;
	static const int FLAG_RAISED = 2;
	static const int FLAG_RELOADING = 4;
	static const int FLAG_SPRINTING = 8;
	static const int FLAG_FREELOOK = 16;

	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		// Twelve scalar values and ten vectors, in Extract/Inject order.
		return lhs.CompareSnapshots(rhs, 12 * 4 + 10 * 12);
	}

	//------------------------------------------------------------------------------------------------
	static bool PropCompare(GC_SpectatorSample instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		if (!snapshot.CompareInt(instance.m_iVersion))
			return false;
		if (!snapshot.CompareInt(instance.m_iSequence))
			return false;
		if (!snapshot.CompareFloat(instance.m_fSampleTime))
			return false;
		if (!snapshot.CompareInt(instance.m_iStance))
			return false;
		if (!snapshot.CompareInt(instance.m_iCommand))
			return false;
		if (!snapshot.CompareInt(instance.m_iFlags))
			return false;
		if (!snapshot.CompareInt(instance.m_iSightState))
			return false;
		if (!snapshot.CompareVector(instance.m_vPosition))
			return false;
		if (!snapshot.CompareVector(instance.m_vForward))
			return false;
		if (!snapshot.CompareVector(instance.m_vUp))
			return false;
		if (!snapshot.CompareVector(instance.m_vVelocity))
			return false;
		if (!snapshot.CompareVector(instance.m_vAimAngles))
			return false;
		if (!snapshot.CompareVector(instance.m_vLookAngles))
			return false;
		if (!snapshot.CompareFloat(instance.m_fLean))
			return false;
		if (!snapshot.CompareFloat(instance.m_fDynamicStance))
			return false;
		if (!snapshot.CompareFloat(instance.m_fMovement))
			return false;
		if (!snapshot.CompareVector(instance.m_vCameraOffset))
			return false;
		if (!snapshot.CompareVector(instance.m_vCameraForward))
			return false;
		if (!snapshot.CompareVector(instance.m_vCameraUp))
			return false;
		if (!snapshot.CompareVector(instance.m_vLensState))
			return false;
		if (!snapshot.CompareFloat(instance.m_fOpticReticleScale) || !snapshot.CompareFloat(instance.m_fOpticEdgeScale))
			return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool CaptureCharacter(ChimeraCharacter character)
	{
		if (!character)
			return false;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || !controller.GetAnimationComponent() || !controller.GetInputContext())
			return false;

		vector transform[4];
		character.GetWorldTransform(transform);
		m_vPosition = transform[3];
		m_vForward = transform[2];
		m_vUp = transform[1];
		Physics physics = character.GetPhysics();
		if (physics)
			m_vVelocity = physics.GetVelocity();

		CharacterMovementState movement = new CharacterMovementState();
		controller.GetAnimationComponent().GetMovementState(movement);
		m_fSampleTime = character.GetWorld().GetWorldTime();
		m_iStance = controller.GetStance();
		m_iCommand = movement.m_CommandTypeId;
		m_fMovement = movement.m_fMovement;
		m_fLean = controller.GetInputContext().GetLean();
		m_fDynamicStance = controller.GetDynamicStance();
		m_vAimAngles = controller.GetInputContext().GetAimingAngles();
		m_vLookAngles = controller.GetInputContext().GetLookAtAngles();
		if (controller.IsWeaponRaised())
			m_iFlags |= FLAG_RAISED;
		if (controller.IsReloading())
			m_iFlags |= FLAG_RELOADING;
		if (controller.IsSprinting())
			m_iFlags |= FLAG_SPRINTING;
		if (controller.IsFreeLookEnabled())
			m_iFlags |= FLAG_FREELOOK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Reject non-finite or unbounded values before using network data in rendering.
	bool IsValid()
	{
		if (m_iVersion != VERSION || m_iSequence < 0 || m_iStance < 0 || m_iStance > 2 || m_iFlags < 0 || m_iFlags > 31)
			return false;
		if (!(m_fSampleTime >= 0 && m_fSampleTime < 1e12) || !(m_fLean >= -1 && m_fLean <= 1) || !(m_fDynamicStance >= 0 && m_fDynamicStance <= 1))
			return false;
		if (!(m_fMovement >= 0 && m_fMovement <= 3) || m_iSightState < 0)
			return false;
		if (!(m_fOpticReticleScale >= 0 && m_fOpticReticleScale < 100) || !(m_fOpticEdgeScale >= 0 && m_fOpticEdgeScale < 100))
			return false;
		if (!IsBoundedVector(m_vPosition, 1e7) || !IsBoundedVector(m_vVelocity, 1000) || !IsBoundedVector(m_vAimAngles, 100) || !IsBoundedVector(m_vLookAngles, 100))
			return false;
		if (!IsBasis(m_vForward, m_vUp) || !IsBasis(m_vCameraForward, m_vCameraUp) || !IsBoundedVector(m_vCameraOffset, 100))
			return false;
		return m_vLensState[0] > 0 && m_vLensState[0] < 180 && m_vLensState[1] >= -1 && m_vLensState[1] < 180 && m_vLensState[2] >= -1 && m_vLensState[2] < 180;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsBoundedVector(vector value, float bound)
	{
		for (int i = 0; i < 3; i++)
		{
			if (!(value[i] >= -bound && value[i] <= bound))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsBasis(vector forward, vector up)
	{
		if (!IsBoundedVector(forward, 1.01) || !IsBoundedVector(up, 1.01))
			return false;
		float forwardLength = forward.LengthSq();
		float upLength = up.LengthSq();
		return forwardLength > 0.99 && forwardLength < 1.01 && upLength > 0.99 && upLength < 1.01 && Math.AbsFloat(vector.Dot(forward, up)) < 0.01;
	}

	//------------------------------------------------------------------------------------------------
	static bool Extract(GC_SpectatorSample instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iVersion);
		snapshot.SerializeInt(instance.m_iSequence);
		snapshot.SerializeFloat(instance.m_fSampleTime);
		snapshot.SerializeInt(instance.m_iStance);
		snapshot.SerializeInt(instance.m_iCommand);
		snapshot.SerializeInt(instance.m_iFlags);
		snapshot.SerializeInt(instance.m_iSightState);
		snapshot.SerializeVector(instance.m_vPosition);
		snapshot.SerializeVector(instance.m_vForward);
		snapshot.SerializeVector(instance.m_vUp);
		snapshot.SerializeVector(instance.m_vVelocity);
		snapshot.SerializeVector(instance.m_vAimAngles);
		snapshot.SerializeVector(instance.m_vLookAngles);
		snapshot.SerializeFloat(instance.m_fLean);
		snapshot.SerializeFloat(instance.m_fDynamicStance);
		snapshot.SerializeFloat(instance.m_fMovement);
		snapshot.SerializeVector(instance.m_vCameraOffset);
		snapshot.SerializeVector(instance.m_vCameraForward);
		snapshot.SerializeVector(instance.m_vCameraUp);
		snapshot.SerializeVector(instance.m_vLensState);
		snapshot.SerializeFloat(instance.m_fOpticReticleScale);
		snapshot.SerializeFloat(instance.m_fOpticEdgeScale);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, GC_SpectatorSample instance)
	{
		snapshot.SerializeInt(instance.m_iVersion);
		snapshot.SerializeInt(instance.m_iSequence);
		snapshot.SerializeFloat(instance.m_fSampleTime);
		snapshot.SerializeInt(instance.m_iStance);
		snapshot.SerializeInt(instance.m_iCommand);
		snapshot.SerializeInt(instance.m_iFlags);
		snapshot.SerializeInt(instance.m_iSightState);
		snapshot.SerializeVector(instance.m_vPosition);
		snapshot.SerializeVector(instance.m_vForward);
		snapshot.SerializeVector(instance.m_vUp);
		snapshot.SerializeVector(instance.m_vVelocity);
		snapshot.SerializeVector(instance.m_vAimAngles);
		snapshot.SerializeVector(instance.m_vLookAngles);
		snapshot.SerializeFloat(instance.m_fLean);
		snapshot.SerializeFloat(instance.m_fDynamicStance);
		snapshot.SerializeFloat(instance.m_fMovement);
		snapshot.SerializeVector(instance.m_vCameraOffset);
		snapshot.SerializeVector(instance.m_vCameraForward);
		snapshot.SerializeVector(instance.m_vCameraUp);
		snapshot.SerializeVector(instance.m_vLensState);
		snapshot.SerializeFloat(instance.m_fOpticReticleScale);
		snapshot.SerializeFloat(instance.m_fOpticEdgeScale);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet); // m_iVersion
		snapshot.EncodeInt(packet); // m_iSequence
		snapshot.EncodeFloat(packet); // m_fSampleTime
		snapshot.EncodeInt(packet); // m_iStance
		snapshot.EncodeInt(packet); // m_iCommand
		snapshot.EncodeInt(packet); // m_iFlags
		snapshot.EncodeInt(packet); // m_iSightState
		snapshot.EncodeVector(packet); // m_vPosition
		snapshot.EncodeVector(packet); // m_vForward
		snapshot.EncodeVector(packet); // m_vUp
		snapshot.EncodeVector(packet); // m_vVelocity
		snapshot.EncodeVector(packet); // m_vAimAngles
		snapshot.EncodeVector(packet); // m_vLookAngles
		snapshot.EncodeFloat(packet); // m_fLean
		snapshot.EncodeFloat(packet); // m_fDynamicStance
		snapshot.EncodeFloat(packet); // m_fMovement
		snapshot.EncodeVector(packet); // m_vCameraOffset
		snapshot.EncodeVector(packet); // m_vCameraForward
		snapshot.EncodeVector(packet); // m_vCameraUp
		snapshot.EncodeVector(packet); // m_vLensState
		snapshot.EncodeFloat(packet); // m_fOpticReticleScale
		snapshot.EncodeFloat(packet); // m_fOpticEdgeScale
	}

	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet); // m_iVersion
		snapshot.DecodeInt(packet); // m_iSequence
		snapshot.DecodeFloat(packet); // m_fSampleTime
		snapshot.DecodeInt(packet); // m_iStance
		snapshot.DecodeInt(packet); // m_iCommand
		snapshot.DecodeInt(packet); // m_iFlags
		snapshot.DecodeInt(packet); // m_iSightState
		snapshot.DecodeVector(packet); // m_vPosition
		snapshot.DecodeVector(packet); // m_vForward
		snapshot.DecodeVector(packet); // m_vUp
		snapshot.DecodeVector(packet); // m_vVelocity
		snapshot.DecodeVector(packet); // m_vAimAngles
		snapshot.DecodeVector(packet); // m_vLookAngles
		snapshot.DecodeFloat(packet); // m_fLean
		snapshot.DecodeFloat(packet); // m_fDynamicStance
		snapshot.DecodeFloat(packet); // m_fMovement
		snapshot.DecodeVector(packet); // m_vCameraOffset
		snapshot.DecodeVector(packet); // m_vCameraForward
		snapshot.DecodeVector(packet); // m_vCameraUp
		snapshot.DecodeVector(packet); // m_vLensState
		snapshot.DecodeFloat(packet); // m_fOpticReticleScale
		snapshot.DecodeFloat(packet); // m_fOpticEdgeScale
		return true;
	}
}
