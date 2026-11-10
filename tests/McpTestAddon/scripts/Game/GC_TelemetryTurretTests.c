#ifdef WORKBENCH
//! Explicit native fixtures. Uses production capture/serialization and current admin authorization.
modded class GC_LobbyManager
{
	protected ref array<IEntity> m_aGC_TurretFixtures = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryTurrets(string action, float yaw)
	{
		PlayerController player = GetGame().GetPlayerController();
		GC_SessionLogComponent session = GC_SessionLogComponent.GetInstance();
		if (GetManager() != this || !Replication.IsServer() || !player || !session)
			return "UNAVAILABLE authority/session";
		if (!AuthorizeAdminAction(player.GetPlayerId(), "telemetry-turret-test", action))
			return "DENIED";
		if (action == "clear")
		{
			foreach (IEntity previous : m_aGC_TurretFixtures)
				SCR_EntityHelper.DeleteEntityAndChildren(previous);
			m_aGC_TurretFixtures.Clear();
			return "CLEARED";
		}
		if (action == "spawn" && m_aGC_TurretFixtures.IsEmpty())
		{
			array<ResourceName> prefabs = {
				"Prefabs/Vehicles/Wheeled/BTR70/BTR70.et",
				"Prefabs/Vehicles/Helicopters/UH1H/UH1H_armed.et",
				"Prefabs/Weapons/Tripods/Tripod_M3_M2HB.et",
				"Prefabs/Vehicles/Wheeled/M998/M998_covered.et"
			};
			foreach (int index, ResourceName prefab : prefabs)
			{
				EntitySpawnParams parameters = new EntitySpawnParams();
				parameters.TransformMode = ETransformMode.WORLD;
				parameters.Transform[3] = Vector(1150 + index * 20, 40, 2730);
				IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), parameters);
				if (!entity)
					return "FAIL spawn " + prefab;
				m_aGC_TurretFixtures.Insert(entity);
			}
		}
		string result;
		foreach (IEntity entity : m_aGC_TurretFixtures)
		{
			if (action == "hull")
			{
				entity.SetAngles(Vector(0, yaw, 0));
				entity.Update();
			}
			if (action == "aim")
			{
				BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(entity.FindComponent(BaseCompartmentManagerComponent));
				array<BaseCompartmentSlot> slots = {};
				if (manager)
					manager.GetCompartments(slots);
				foreach (BaseCompartmentSlot slot : slots)
				{
					TurretControllerComponent controller = TurretControllerComponent.Cast(slot.GetController());
					if (controller && controller.GetTurretComponent())
						controller.GetTurretComponent().SetAimingRotation(Vector(yaw * Math.DEG2RAD, 0.15, 0));
				}
			}
			GC_TelemetryTurrets snapshot = new GC_TelemetryTurrets();
			if (!snapshot.Capture(entity))
				return "FAIL capture limit";
			JsonSaveContext context = new JsonSaveContext();
			snapshot.Write(context);
			result += string.Format(" prefab=%1 id=%2 hull=%3 snapshot=%4", entity.GetPrefabData().GetPrefabName(), session.GC_TestRecordIconEntity(entity), entity.GetAngles(), context.SaveToString());
		}
		GC_AdminActionLogger.Log(player.GetPlayerId(), "telemetry-turret-test", action, "applied");
		return result + " " + session.GetTelemetryReport();
	}
}
#endif
