modded class SCR_2DPIPSightsComponent
{
	//------------------------------------------------------------------------------------------------
	//! Suppress only competing lens calculations. The inherited ADS callback
	//! still advances zeroing, observer state, settings and HDR normally.
	override void UpdateCamera(float timeSlice)
	{
		if (!GC_IsNetworkPIP())
			super.UpdateCamera(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	override void UpdateScopeMaterial()
	{
		if (!GC_IsNetworkPIP())
			super.UpdateScopeMaterial();
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_IsNetworkPIP()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return false;
		CameraManager manager = GetGame().GetCameraManager();
		if (!manager)
			return false;
		GC_SpectatorManualCamera camera = GC_SpectatorManualCamera.Cast(manager.CurrentCamera());
		return camera && camera.OwnsNetworkPIPSight(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Capture the stock result instead of recalculating with a manual camera's
	//! different focus-FOV reference. No additional owner-side optics work runs.
	void GC_GetSpectatorOpticScales(out float reticleScale, out float edgeScale)
	{
		reticleScale = m_fReticleScale;
		edgeScale = m_fEdgeScale;
	}

	//------------------------------------------------------------------------------------------------
	void GC_UpdateSpectatorPIP(float timeSlice, float authoritativePIPFOV, float reticleScale, float edgeScale)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;

		if (!IsPIPEnabled() || !m_PIPCamera || !m_pMaterial || authoritativePIPFOV <= 0)
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;

		CameraBase mainCamera = cameraManager.CurrentCamera();
		if (!mainCamera)
			return;

		// Use the optic's camera mount, in front of the lens. Placing the PiP
		// camera at the player's eye renders the scope itself inside its texture.
		vector opticCameraTransform[4];
		GetCameraLocalTransform(opticCameraTransform);
		m_PIPCamera.SetLocalTransform(opticCameraTransform);
		m_PIPCamera.SetVerticalFOV(authoritativePIPFOV);
		m_PIPCamera.ApplyTransform(timeSlice);

		m_fReticleScale = reticleScale;
		m_fEdgeScale = edgeScale;

		SCR_2DPIPSightsComponentClass data = SCR_2DPIPSightsComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return;

		float pipFOV = authoritativePIPFOV;
		m_fParallaxX = 0;
		m_fParallaxY = 0;
		m_pMaterial.SetParamByIndex(m_iVignetteCenterXIndex, 0);
		m_pMaterial.SetParamByIndex(m_iVignetteCenterYIndex, 0);
		m_pMaterial.SetParamByIndex(m_iVignettePowerIndex, data.GetBasicParallax());

		float reticlePIPScale = data.GetReticlePIPScale();
		m_pMaterial.SetParamByIndex(m_iEdgeCenterXIndex, 0);
		m_pMaterial.SetParamByIndex(m_iEdgeCenterYIndex, 0);
		float edgeSize = m_fObjectiveFov * m_fObjectiveScale * m_fEdgeScale / pipFOV;
		m_pMaterial.SetParamByIndex(m_iEdgeMinIndex, 0.5 * reticlePIPScale * edgeSize * edgeSize * data.GetObjectivePIPEdgeMin());
		m_pMaterial.SetParamByIndex(m_iEdgeMaxIndex, 0.5 * reticlePIPScale * edgeSize * edgeSize * data.GetObjectivePIPEdgeMax());

		m_pMaterial.SetParamByIndex(m_iReticleScaleIndex, m_fReticleScale / reticlePIPScale);
		m_pMaterial.SetParamByIndex(m_iReticleOffsetXIndex, m_fReticleScale * m_fReticleOffsetX / pipFOV);
		m_pMaterial.SetParamByIndex(m_iReticleOffsetYIndex, m_fReticleScale * m_fCurrentReticleOffsetY / pipFOV);
		float reticleRGBA[4] = {m_cReticleColor.R(), m_cReticleColor.G(), m_cReticleColor.B(), m_cReticleColor.A()};
		m_pMaterial.SetParamByIndex(m_iReticleColorIndex, reticleRGBA);
	}
}
