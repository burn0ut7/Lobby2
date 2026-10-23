//! Explicit optional fixtures; all healing goes through the production request.
modded class GC_LobbyManager
{
	protected ref array<IEntity> m_aGC_HealVehicles = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestHealVehicleByRpl(int id)
	{
		IEntity vehicle = IEntity.Cast(Replication.FindItem(id));
		if (!vehicle)
			return "UNAVAILABLE replicated vehicle";
		SCR_VehicleDamageManagerComponent damage = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!damage)
			return "UNAVAILABLE vehicle damage";
		array<HitZone> zones = {};
		damage.GetAllHitZonesInHierarchy(zones);
		int damaged;
		foreach (HitZone zone : zones)
		{
			if (zone && zone.GetHealthScaled() < 0.999)
				damaged++;
		}
		return string.Format("vehicleRpl=%1 state=%2 zones=%3 damaged=%4", id, damage.GetState(), zones.Count(), damaged);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealVehicleBoundary(int index, int distanceMeters, int radiusMeters)
	{
		if (!Replication.IsServer() || !m_aGC_HealVehicles.IsIndexValid(index))
			return "UNAVAILABLE fixture";
		IEntity origin = GetGame().GetPlayerController().GetControlledEntity();
		m_aGC_HealVehicles[index].SetOrigin(origin.GetOrigin() + Vector(distanceMeters, 0, 0));
		string before = GC_TestHealVehicleDamage(index, false);
		string dispatch = GC_TestAdminHeal(GC_EAdminHealScope.NEARBY, 0, "", radiusMeters);
		return string.Format("before=[%1] request=[%2] after=[%3]", before, dispatch, GC_TestHealVehicleSnapshot(index));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealDeleteCorpse(int playerId)
	{
		GC_Slot slot = GetSlotByPlayerId(playerId);
		if (!Replication.IsServer() || !slot || !slot.IsKIA() || !slot.GetEntity())
			return "UNAVAILABLE dead slot";
		SCR_EntityHelper.DeleteEntityAndChildren(slot.GetEntity());
		return "Corpse deleted; inspect retained claim and heal behavior.";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealKill(int playerId)
	{
		ChimeraCharacter character = GC_GetAdminToolCharacter(playerId);
		if (!Replication.IsServer() || !character)
			return "UNAVAILABLE living authority target";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		damage.SetHealthScaled(0);
		return "Death arranged; observe slot and possession before requesting heal.";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealSlot(int playerId)
	{
		GC_Slot slot = GetSlotByPlayerId(playerId);
		if (!slot)
			return "NO SLOT";
		IEntity entity = slot.GetEntity();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		return string.Format("player=%1 identity=%2 slot=%3 kia=%4 entity=%5 possessed=%6 state=%7",
			playerId, GetPlayerVoiceIdentity(playerId), slot.GetID(), slot.IsKIA(), entity,
			entity && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId) == entity, controller.GetLobbyPlayerState());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealVehicleSpawn(int distanceMeters)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity() || m_aGC_HealVehicles.Count() >= 3)
			return "UNAVAILABLE authority or fixture limit";
		Resource prefab = Resource.Load("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = player.GetControlledEntity().GetOrigin() + Vector(distanceMeters, 0, 0);
		IEntity vehicle = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!vehicle)
			return "FAILED spawn";
		m_aGC_HealVehicles.Insert(vehicle);
		return string.Format("spawned index=%1 rpl=%2", m_aGC_HealVehicles.Count() - 1, Replication.FindItemId(vehicle));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealVehicleDamage(int index, bool destroy)
	{
		if (!Replication.IsServer() || !m_aGC_HealVehicles.IsIndexValid(index))
			return "UNAVAILABLE fixture";
		SCR_VehicleDamageManagerComponent damage = SCR_VehicleDamageManagerComponent.Cast(m_aGC_HealVehicles[index].FindComponent(SCR_VehicleDamageManagerComponent));
		if (destroy)
			damage.SetHealthScaled(0);
		else
		{
			array<HitZone> zones = {};
			damage.GetAllHitZonesInHierarchy(zones);
			foreach (HitZone zone : zones)
			{
				if (zone)
					zone.SetHealthScaled(0.6);
			}
		}
		return GC_TestHealVehicleSnapshot(index);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestHealVehicleSnapshot(int index)
	{
		if (!m_aGC_HealVehicles.IsIndexValid(index) || !m_aGC_HealVehicles[index])
			return "UNAVAILABLE fixture";
		IEntity vehicle = m_aGC_HealVehicles[index];
		SCR_VehicleDamageManagerComponent damage = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		array<HitZone> zones = {};
		damage.GetAllHitZonesInHierarchy(zones);
		int damaged;
		foreach (HitZone zone : zones)
		{
			if (zone && zone.GetHealthScaled() < 0.999)
				damaged++;
		}
		return string.Format("vehicle=%1 state=%2 zones=%3 damaged=%4 distance=%5 position=%6", index,
			damage.GetState(), zones.Count(), damaged,
			vector.Distance(vehicle.GetOrigin(), GetGame().GetPlayerController().GetControlledEntity().GetOrigin()), vehicle.GetOrigin());
	}
}
