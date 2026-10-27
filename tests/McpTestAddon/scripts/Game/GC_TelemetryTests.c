//! Opt-in observers only. Production handlers remain the owners of recorded events.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Start and change native health in the same call, before scheduled discovery.
	string GC_TestTelemetryImmediateDeath()
	{
		PlayerController player = GetGame().GetPlayerController();
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !player || !logs)
			return "UNAVAILABLE authority/session";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return "UNAVAILABLE playing character";
		if (!logs.SetTelemetry(player.GetPlayerId(), true))
			return "REJECTED recording start";
		character.GetDamageManager().GetDefaultHitZone().SetHealthScaled(0);
		return logs.GetTelemetryReport();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryShotProbe()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || !player.GetControlledEntity())
			return "UNAVAILABLE character";
		IEntity entity = player.GetControlledEntity();
		EventHandlerManagerComponent events = EventHandlerManagerComponent.Cast(entity.FindComponent(EventHandlerManagerComponent));
		if (!events)
			return "UNAVAILABLE events";
		events.RegisterScriptHandler("OnProjectileShot", this, GC_TestTelemetryShot, false, true);
		return "ARMED one native shot observation";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestTelemetryShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		PrintFormat("GC | Telemetry test | native shot player=%1 weapon=%2 projectile=%3", playerId, weapon, projectile);
	}
}

#ifdef WORKBENCH
//! Native-actor load fixture; does not simulate network connections or AI thinking.
modded class GC_SessionLogComponent
{
	protected ref array<IEntity> m_GC_TestTelemetryActors = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryLoad(int count)
	{
		PlayerController player = GetGame().GetPlayerController();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !player || !manager || !manager.AuthorizeAdminAction(player.GetPlayerId(), "telemetry-load-test", count.ToString()))
			return "REJECTED authority/admin";
		if (count == 0)
		{
			foreach (IEntity previous : m_GC_TestTelemetryActors)
			{
				if (previous)
					SCR_EntityHelper.DeleteEntityAndChildren(previous);
			}
			m_GC_TestTelemetryActors.Clear();
			return "CLEARED native fixture actors";
		}
		if (count < 1 || count > 127 || !m_GC_TestTelemetryActors.IsEmpty() || !m_Telemetry || !m_Telemetry.IsRunning() || !player.GetControlledEntity())
			return "UNAVAILABLE count/recording/character/existing fixture";
		Resource prefab = Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et");
		for (int i; i < count; i++)
		{
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			int column = i % 16;
			int row = i / 16;
			parameters.Transform[3] = player.GetControlledEntity().GetOrigin() + Vector(4 + column * 2, 0, 4 + row * 2);
			IEntity actor = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters);
			if (!actor)
				return "FAIL native spawn; cleanup required";
			m_GC_TestTelemetryActors.Insert(actor);
			if (!m_Telemetry.Track(actor))
				return "FAIL production admission; cleanup required";
		}
		return "CREATED native actors=" + m_GC_TestTelemetryActors.Count() + " " + m_Telemetry.Report();
	}
}
#endif
