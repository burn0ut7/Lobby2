modded class SCR_2DPIPSightsComponent
{
	//! The observed proxy supplies only the selected optic's presentation. The
	//! replicated spectator camera remains authoritative for view and parallax.
	void GC_UpdateSpectatorPIP(float timeSlice, float authoritativePIPFOV, float authoritativeSightFOV)
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return;

		if (!IsPIPEnabled() || !m_PIPCamera || !m_pMaterial || authoritativePIPFOV <= 0 || authoritativeSightFOV <= 0)
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;

		CameraBase mainCamera = cameraManager.CurrentCamera();
		if (!mainCamera)
			return;

		vector mainCameraTransform[4];
		mainCamera.GetWorldTransform(mainCameraTransform);
		m_PIPCamera.SetWorldTransform(mainCameraTransform);
		m_PIPCamera.SetVerticalFOV(authoritativePIPFOV);
		m_PIPCamera.ApplyTransform(timeSlice);

		UpdateHDR();

		SightsFOVInfo fovInfo = GetFOVInfo();
		if (fovInfo)
		{
			SCR_BaseVariableSightsFOVInfo variableSights = SCR_BaseVariableSightsFOVInfo.Cast(fovInfo);
			if (variableSights)
				m_fEdgeScale = authoritativeSightFOV / variableSights.GetBaseFOV();

			float reticleFOV;
			if (m_fReticleBaseZoom > 0)
				m_fReticleBaseZoom = -CalculateZoomFOV(m_fReticleBaseZoom);

			if (m_fReticleBaseZoom == 0)
				reticleFOV = authoritativeSightFOV;
			else if (m_fReticleBaseZoom < 0)
				reticleFOV = -m_fReticleBaseZoom;

			if (reticleFOV > 0 && m_fReticleAngularSize != 0)
				m_fReticleScale = reticleFOV * (authoritativePIPFOV / authoritativeSightFOV) * m_fReticlePortion / m_fReticleAngularSize;
		}

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
