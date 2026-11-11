#ifdef WORKBENCH
modded class GC_LobbyPlayerController
{
	void GC_TestSelectNearbyCrate()
	{
		GC_RpcAskGearCrate(false);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestNoSphereHelpers()
	{
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!player || !player.GetControlledEntity() || !AuthorizeAdminAction(player.GetPlayerId(), "test-spatial-helpers", "isolated fixtures"))
			return "Admin and controlled entity required";
		BaseWorld world = GetGame().GetWorld();
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		vector centre = "1190 100 2700";
		parameters.Transform[3] = centre;
		Resource vehicleResource = Resource.Load("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
		IEntity nearVehicle = GetGame().SpawnEntityPrefab(vehicleResource, world, parameters);
		parameters.Transform[3] = centre + "20 0 0";
		IEntity farVehicle = GetGame().SpawnEntityPrefab(vehicleResource, world, parameters);
		Resource crateResource = Resource.Load("{EBBC859CCD94D96D}Prefabs/Props/EmptyStorages/USSR_AmmoBox_empty.et");
		parameters.Transform[3] = centre + "2 0 0";
		IEntity nearCrate = GetGame().SpawnEntityPrefab(crateResource, world, parameters);
		parameters.Transform[3] = centre + "4 0 0";
		IEntity farCrate = GetGame().SpawnEntityPrefab(crateResource, world, parameters);
		if (!nearVehicle || !farVehicle || !nearCrate || !farCrate)
		{
			if (nearVehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(nearVehicle);
			if (farVehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(farVehicle);
			if (nearCrate)
				SCR_EntityHelper.DeleteEntityAndChildren(nearCrate);
			if (farCrate)
				SCR_EntityHelper.DeleteEntityAndChildren(farCrate);
			return "Native fixtures unavailable";
		}
		SCR_VehicleDamageManagerComponent nearDamage = SCR_VehicleDamageManagerComponent.Cast(nearVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		SCR_VehicleDamageManagerComponent farDamage = SCR_VehicleDamageManagerComponent.Cast(farVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		nearDamage.SetHealthScaled(0.6);
		farDamage.SetHealthScaled(0.6);
		GC_AdminNearbyVehicleRepair repair = new GC_AdminNearbyVehicleRepair();
		string repaired = repair.Execute(player.GetPlayerId(), centre, 10, "test-spatial-helpers");
		bool vehicles = nearDamage.GetHealthScaled() > 0.99 && farDamage.GetHealthScaled() < 0.99;
		nearDamage.SetHealthScaled(0.6);
		GC_ExecuteAdminHeal(-1, GC_EAdminHealScope.NEARBY, {}, {}, 0, 10, "");
		bool deniedHeal = nearDamage.GetHealthScaled() < 0.99;
		vector original = player.GetControlledEntity().GetOrigin();
		player.GetControlledEntity().SetOrigin(centre);
		player.GC_TestSelectNearbyCrate();
		player.GetControlledEntity().SetOrigin(original);
		SCR_UniversalInventoryStorageComponent nearStorage = SCR_UniversalInventoryStorageComponent.Cast(nearCrate.FindComponent(SCR_UniversalInventoryStorageComponent));
		SCR_UniversalInventoryStorageComponent farStorage = SCR_UniversalInventoryStorageComponent.Cast(farCrate.FindComponent(SCR_UniversalInventoryStorageComponent));
		bool nearest = nearStorage.GC_HasAdminGearCapacity() && !farStorage.GC_HasAdminGearCapacity();
		GC_LobbyPlayerController unresolved = GC_LobbyPlayerController.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{FE3C731AAB2FB003}Prefabs/Characters/Core/LobbyPlayerController.et"), world));
		bool deniedCrate;
		if (unresolved && unresolved.GetPlayerId() <= 0)
		{
			unresolved.GC_TestSelectNearbyCrate();
			deniedCrate = !farStorage.GC_HasAdminGearCapacity();
		}
		if (unresolved)
			SCR_EntityHelper.DeleteEntityAndChildren(unresolved);
		SCR_EntityHelper.DeleteEntityAndChildren(nearVehicle);
		SCR_EntityHelper.DeleteEntityAndChildren(farVehicle);
		SCR_EntityHelper.DeleteEntityAndChildren(nearCrate);
		SCR_EntityHelper.DeleteEntityAndChildren(farCrate);
		return string.Format("Physics helper checks: vehicles=%1 deniedHeal=%2 nearestCrate=%3 deniedCrate=%4; %5", vehicles, deniedHeal, nearest, deniedCrate, repaired);
	}
}
#endif
