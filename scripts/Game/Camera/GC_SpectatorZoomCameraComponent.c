//! Only the Lobby2 camera uses this component; stock cameras retain their input handling.
[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
class GC_SpectatorZoomCameraComponent : SCR_ZoomManualCameraComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		if (!param.isCursorEnabled)
		{
			// Discard zoom inertia too, so leaving a panel cannot replay an old wheel step.
			m_fTargetFOV = param.fov;
			m_bIsAnimating = false;
			m_bIsInstant = false;
			return;
		}

		super.EOnCameraFrame(param);
	}
}
