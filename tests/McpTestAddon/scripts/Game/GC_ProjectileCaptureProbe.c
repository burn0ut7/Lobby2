//! Explicit native-flight integration probe; does not activate on addon load.
class GC_ProjectileCaptureProbe
{
	static bool s_bActive;
	static int s_iEffects;
	static string s_sEvidence;
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestImpactIsolation(bool fire)
	{
		if (GC_SessionLogComponent.GetInstance() || !Replication.IsServer())
			return "unavailable";
		if (!fire)
			return string.Format("sessionAbsent=1 effects=%1 evidence=%2", GC_ProjectileCaptureProbe.s_iEffects, GC_ProjectileCaptureProbe.s_sEvidence);
		GC_ProjectileCaptureProbe.s_bActive = true;
		Resource prefab = Resource.Load("{AC26AB660097633D}Prefabs/Weapons/Ammo/Ammo_556x45_Ball_M855.et");
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "130 10 130";
		IEntity projectile = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters);
		if (!projectile)
			return "spawn-failed";
		ProjectileMoveComponent movement = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (!movement)
			return "movement-unavailable";
		movement.Launch("0 -1 0", vector.Zero, 1, projectile, GetOwner(), null, null, null);
		return "launched; inspect contact and unchanged outbox";
	}
}

modded class GC_TelemetryImpactEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		super.OnEffect(pHitEntity, outMat, damageSource, instigator, colliderName, speed);
		if (!GC_ProjectileCaptureProbe.s_bActive || GC_ProjectileCaptureProbe.s_iEffects >= 12)
			return;
		GC_ProjectileCaptureProbe.s_iEffects++;
		GC_ProjectileCaptureProbe.s_sEvidence += string.Format(" effect=%1 pHitEntity=%2 matrix0=%3 matrix1=%4 matrix2=%5 speed=%6 damageSource=%7", Type(), pHitEntity, outMat[0], outMat[1], outMat[2], speed, damageSource);
	}
}

modded class GC_LobbyManager
{
	string GC_TestProjectileCaptureProbe(string action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || GetManager() != this || !controller || !logs)
			return "unavailable";
		if (action == "advance")
		{
			controller.RequestLobbyPhaseAdvance(GC_LobbyPhase.GetNext(GetLobbyPhase()));
			return GetPhaseName(GetLobbyPhase());
		}
		if (action == "record" || action == "stop")
			return logs.SetTelemetry(controller.GetPlayerId(), action == "record", 64).ToString() + " " + logs.GetTelemetryReport();
		if (action == "status")
			return logs.GetSessionId() + " " + logs.GetTelemetryReport();
		if (action == "snapshot")
			return string.Format("effects=%1 evidence=%2", GC_ProjectileCaptureProbe.s_iEffects, GC_ProjectileCaptureProbe.s_sEvidence);
		if (!Replication.IsServer() || GetManager() != this || action != "fire")
			return "unavailable";
		GC_ProjectileCaptureProbe.s_bActive = true;
		Resource prefab = Resource.Load("{AC26AB660097633D}Prefabs/Weapons/Ammo/Ammo_556x45_Ball_M855.et");
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "1190 45 2730";
		IEntity projectile = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters);
		if (!projectile)
			return "spawn-failed";
		ProjectileMoveComponent movement = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (!movement)
			return "movement-unavailable";
		movement.Launch("0 -1 0", vector.Zero, 1, projectile, GetOwner(), null, null, null);
		if (logs)
			logs.RecordShot("shot", GetGame().GetPlayerController().GetPlayerId(), null, projectile);
		return string.Format("launched=%1 velocity=%2", projectile.GetOrigin(), movement.GetVelocity());
	}
}

