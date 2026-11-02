#ifdef WORKBENCH
//! Explicit fixture; production admission and serialization remain authoritative.
modded class GC_SessionLogComponent
{
	string GC_TestRecordIconEntity(IEntity entity)
	{
		if (GetInstance() != this || !Replication.IsServer() || !m_Telemetry)
			return string.Empty;
		return m_Telemetry.EntityId(entity);
	}
}

modded class GC_LobbyManager
{
	protected ref array<IEntity> m_aGC_IconFixtures = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryIcons(float yaw)
	{
		PlayerController player = GetGame().GetPlayerController();
		GC_SessionLogComponent session = GC_SessionLogComponent.GetInstance();
		if (GetManager() != this || !Replication.IsServer() || !player || !session)
			return "unavailable";
		if (!AuthorizeAdminAction(player.GetPlayerId(), "telemetry-icon-probe", yaw.ToString()))
			return "denied";
		if (m_aGC_IconFixtures.IsEmpty())
		{
			array<ResourceName> prefabs = {
				"{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et",
				"{70BAEEFC2D3FEE64}Prefabs/Vehicles/Helicopters/UH1H/UH1H.et",
				"{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et"
			};
			foreach (int index, ResourceName name : prefabs)
			{
				EntitySpawnParams parameters = new EntitySpawnParams();
				parameters.TransformMode = ETransformMode.WORLD;
				parameters.Transform[3] = Vector(1150 + index * 15, 40, 2730);
				IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(name), GetGame().GetWorld(), parameters);
				if (entity)
					m_aGC_IconFixtures.Insert(entity);
			}
		}
		string result;
		foreach (IEntity entity : m_aGC_IconFixtures)
		{
			entity.SetAngles(Vector(0, yaw, 0));
			entity.Update();
			vector transform[4];
			entity.GetTransform(transform);
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
			if (editable && editable.GetInfo())
				result += string.Format(" id=%1 angles=%2 forward=%3 resource=%4 name=%5", session.GC_TestRecordIconEntity(entity), entity.GetAngles(), transform[2], editable.GetInfo().GetIcon(), editable.GetInfo().GetIconSetName());
		}
		GC_AdminActionLogger.Log(player.GetPlayerId(), "telemetry-icon-probe", yaw.ToString(), "applied");
		return result;
	}
}
#endif
