modded class SCR_2DPIPSightsComponent
{
	//! The stock PiP post-frame path resolves material geometry through the local
	//! controlled character. A spectator observes a remote proxy without taking
	//! possession, so update the same presentation from that proxy explicitly.
	void GC_UpdateSpectatorPIP(IEntity observedCharacter, float timeSlice, float authoritativePIPFOV)
	{
		if (!IsPIPEnabled() || !m_PIPCamera || !m_pMaterial || authoritativePIPFOV <= 0)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(observedCharacter);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;

		CameraBase mainCamera = cameraManager.CurrentCamera();
		if (!mainCamera)
			return;

		// Refresh proxy-local parallax and material state first, then restore the
		// owner's authoritative render FOV before deriving FOV-dependent values.
		UpdateCamera(timeSlice);
		UpdateHDR();
		m_PIPCamera.SetVerticalFOV(authoritativePIPFOV);

		SightsFOVInfo fovInfo = GetFOVInfo();
		if (fovInfo)
		{
			float sightFOV = fovInfo.GetFOV();
			if (sightFOV > 0)
			{
				SCR_BaseVariableSightsFOVInfo variableSights = SCR_BaseVariableSightsFOVInfo.Cast(fovInfo);
				if (variableSights)
					m_fEdgeScale = sightFOV / variableSights.GetBaseFOV();

				float reticleFOV;
				if (m_fReticleBaseZoom > 0)
					m_fReticleBaseZoom = -CalculateZoomFOV(m_fReticleBaseZoom);

				if (m_fReticleBaseZoom == 0)
					reticleFOV = sightFOV;
				else if (m_fReticleBaseZoom < 0)
					reticleFOV = -m_fReticleBaseZoom;

				if (reticleFOV > 0 && m_fReticleAngularSize != 0)
					m_fReticleScale = reticleFOV * (authoritativePIPFOV / sightFOV) * m_fReticlePortion / m_fReticleAngularSize;
			}
		}

		vector sightsModelTransform[4];
		float sightsFOV;
		if (!weaponManager.GetCurrentSightsCameraTransform(sightsModelTransform, sightsFOV))
			return;

		vector characterWorldTransform[4];
		character.GetWorldTransform(characterWorldTransform);
		vector sightsWorldTransform[4];
		Math3D.MatrixMultiply4(characterWorldTransform, sightsModelTransform, sightsWorldTransform);

		vector mainCameraTransform[4];
		mainCamera.GetWorldTransform(mainCameraTransform);
		vector scopePosition = sightsWorldTransform[3] - mainCameraTransform[3];
		float sightProjection = vector.Dot(mainCameraTransform[2], sightsWorldTransform[2]);
		if (Math.AbsFloat(sightProjection) < 0.0001)
			return;

		float eyeDistance = vector.Dot(scopePosition, sightsWorldTransform[2]) / sightProjection;
		SCR_2DPIPSightsComponentClass data = SCR_2DPIPSightsComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return;

		float distance = Math.AbsFloat(eyeDistance) - data.GetCenterDistance();
		float distancePower;
		if (distance > 0)
			distancePower = data.GetDistanceMoveFar();
		else
			distancePower = data.GetDistanceMoveNear();

		float distanceFactor = 1 - 1 / Math.Log2(distancePower * Math.AbsFloat(distance) + 2);
		float vignettePower = Math.Clamp(distanceFactor * 100, data.GetBasicParallax(), data.GetMaxParallax());
		float pipFOV = authoritativePIPFOV;
		if (pipFOV <= 0)
			return;

		float vignetteParallaxScale;
		if (m_fObjectiveFov > 0)
			vignetteParallaxScale = data.GetVignetteParallaxScale() / (m_fObjectiveFov * m_fObjectiveScale);

		m_pMaterial.SetParamByIndex(m_iVignetteCenterXIndex, -m_fParallaxX * vignetteParallaxScale / pipFOV);
		m_pMaterial.SetParamByIndex(m_iVignetteCenterYIndex, -m_fParallaxY * vignetteParallaxScale / pipFOV);
		m_pMaterial.SetParamByIndex(m_iVignettePowerIndex, vignettePower);

		float reticlePIPScale = data.GetReticlePIPScale();
		m_pMaterial.SetParamByIndex(m_iEdgeCenterXIndex, m_fParallaxX * reticlePIPScale / pipFOV);
		m_pMaterial.SetParamByIndex(m_iEdgeCenterYIndex, m_fParallaxY * reticlePIPScale / pipFOV);
		float edgeSize = m_fObjectiveFov * m_fObjectiveScale * m_fEdgeScale / pipFOV;
		m_pMaterial.SetParamByIndex(m_iEdgeMinIndex, 0.5 * reticlePIPScale * edgeSize * edgeSize * data.GetObjectivePIPEdgeMin());
		m_pMaterial.SetParamByIndex(m_iEdgeMaxIndex, 0.5 * reticlePIPScale * edgeSize * edgeSize * data.GetObjectivePIPEdgeMax());

		m_pMaterial.SetParamByIndex(m_iReticleScaleIndex, m_fReticleScale / reticlePIPScale);
		m_pMaterial.SetParamByIndex(m_iReticleOffsetXIndex, m_fReticleScale * (m_fParallaxX + m_fReticleOffsetX) / pipFOV);
		m_pMaterial.SetParamByIndex(m_iReticleOffsetYIndex, m_fReticleScale * (m_fParallaxY + m_fCurrentReticleOffsetY) / pipFOV);
		float reticleRGBA[4] = {m_cReticleColor.R(), m_cReticleColor.G(), m_cReticleColor.B(), m_cReticleColor.A()};
		m_pMaterial.SetParamByIndex(m_iReticleColorIndex, reticleRGBA);
	}
}
