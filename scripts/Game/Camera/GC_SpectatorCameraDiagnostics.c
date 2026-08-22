class GC_SpectatorCameraDiagnostics
{
	static const int SAMPLE_INTERVAL = 30;

	static void LogSnapshot(string role, int sequence, IEntity character, vector cameraWorldTransform[4], vector cameraOffset, float mainFOV, float pipFOV, float networkSightFOV, bool isADS, int sightState)
	{
		if (!character)
			return;

		int sightsIndex = -1;
		int zoomIndex = -1;
		if (isADS && sightState > 0)
		{
			sightsIndex = sightState / GC_LobbyPlayerController.NETWORK_CAMERA_SIGHT_STATE_SCALE - 1;
			zoomIndex = sightState % GC_LobbyPlayerController.NETWORK_CAMERA_SIGHT_STATE_SCALE - 1;
		}

		vector characterWorldTransform[4];
		character.GetWorldTransform(characterWorldTransform);
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 ads=%3 sight=%4 zoom=%5", role, sequence, isADS, sightsIndex, zoomIndex);
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 mainFOV=%3 pipFOV=%4 networkSightFOV=%5 cameraOffset=%6", role, sequence, mainFOV, pipFOV, networkSightFOV, cameraOffset);
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 cameraPosition=%3 cameraRight=%4 cameraUp=%5 cameraForward=%6", role, sequence, cameraWorldTransform[3], cameraWorldTransform[0], cameraWorldTransform[1], cameraWorldTransform[2]);
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 characterPosition=%3 characterUp=%4 characterForward=%5", role, sequence, characterWorldTransform[3], characterWorldTransform[1], characterWorldTransform[2]);

		CharacterAimingComponent aiming = CharacterAimingComponent.Cast(character.FindComponent(CharacterAimingComponent));
		if (aiming)
		{
			vector aimingDirectionWorld = aiming.GetAimingDirectionWorld();
			vector aimingDirectionCamera = Vector(
				vector.Dot(aimingDirectionWorld, cameraWorldTransform[0]),
				vector.Dot(aimingDirectionWorld, cameraWorldTransform[1]),
				vector.Dot(aimingDirectionWorld, cameraWorldTransform[2]));
			PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 aimWorld=%3 aimCamera=%4 rawAimTranslation=%5 rawAimRotation=%6", role, sequence, aimingDirectionWorld, aimingDirectionCamera, aiming.GetRawAimingTranslation(), aiming.GetRawAimingRotation());
		}

		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));
		vector sightsLocalTransform[4];
		float sightsFOV;
		if (!isADS || !weaponManager || !weaponManager.GetCurrentSightsCameraTransform(sightsLocalTransform, sightsFOV))
		{
			PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 sightTransformAvailable=false", role, sequence);
			return;
		}

		vector sightsWorldTransform[4];
		Math3D.MatrixMultiply4(characterWorldTransform, sightsLocalTransform, sightsWorldTransform);
		vector sightDisplacement = sightsWorldTransform[3] - cameraWorldTransform[3];
		vector sightDisplacementCamera = Vector(
			vector.Dot(sightDisplacement, cameraWorldTransform[0]),
			vector.Dot(sightDisplacement, cameraWorldTransform[1]),
			vector.Dot(sightDisplacement, cameraWorldTransform[2]));
		vector sightForwardCamera = Vector(
			vector.Dot(sightsWorldTransform[2], cameraWorldTransform[0]),
			vector.Dot(sightsWorldTransform[2], cameraWorldTransform[1]),
			vector.Dot(sightsWorldTransform[2], cameraWorldTransform[2]));
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 sightFOV=%3 sightLocalPosition=%4 sightWorldPosition=%5", role, sequence, sightsFOV, sightsLocalTransform[3], sightsWorldTransform[3]);
		PrintFormat("GC | Spectator Camera Debug | role=%1 sequence=%2 sightDisplacementCamera=%3 sightForwardCamera=%4 sightWorldForward=%5", role, sequence, sightDisplacementCamera, sightForwardCamera, sightsWorldTransform[2]);
	}
}
