//! Optional real-compartment fixture. Routing remains entirely in production chat.
modded class GC_LobbyManager
{
	protected ref array<IEntity> m_aGC_ChatVehicles = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestChatVehicleSpawn()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity() || m_aGC_ChatVehicles.Count() >= 2)
			return "UNAVAILABLE authority/character or two fixtures already exist";
		Resource prefab = Resource.Load("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
		if (!prefab || !prefab.IsValid())
			return "UNAVAILABLE stock vehicle";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = player.GetControlledEntity().GetOrigin() + "10 0 10";
		if (!m_aGC_ChatVehicles.IsEmpty())
			params.Transform[3] = params.Transform[3] + "10 0 0";
		IEntity vehicle = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!vehicle)
			return "FAILED vehicle spawn";
		m_aGC_ChatVehicles.Insert(vehicle);
		return string.Format("spawned index=%1 rpl=%2", m_aGC_ChatVehicles.Count() - 1, Replication.FindItemId(vehicle));
	}

	//------------------------------------------------------------------------------------------------
	//! Negative seat exits; entry is a separate call after observing completion.
	string GC_TestChatVehicleEnter(int playerId, int vehicleIndex, int seat)
	{
		if (!Replication.IsServer() || !m_aGC_ChatVehicles.IsIndexValid(vehicleIndex))
			return "UNAVAILABLE authority/vehicle";
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		RplComponent replication = RplComponent.Cast(m_aGC_ChatVehicles[vehicleIndex].FindComponent(RplComponent));
		if (!player || !replication)
			return "UNAVAILABLE player/replication";
		player.GC_TestChatVehicleDispatch(replication.Id(), seat);
		return "DISPATCHED inspect actual vehicle membership on both peers";
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_TestChatVehicleDispatch(RplId vehicleId, int seat)
	{
		if (!Replication.IsServer())
			return;
		if (GetGame().GetPlayerController() == this)
			GC_TestChatVehicleOwner(vehicleId, seat);
		else
			Rpc(GC_TestChatVehicleOwner, vehicleId, seat);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_TestChatVehicleOwner(RplId vehicleId, int seat)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyManager.GetManager())
			return;
		ChimeraCharacter character = ChimeraCharacter.Cast(GetControlledEntity());
		RplComponent replication = RplComponent.Cast(Replication.FindItem(vehicleId));
		if (!character || !replication)
			return;
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		IEntity vehicle = replication.GetEntity();
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!access || !compartments)
			return;
		if (seat < 0)
		{
			vector transform[4];
			character.GetTransform(transform);
			transform[3] = character.GetOrigin() + "0 0 5";
			access.GetOutVehicle_NoDoor(transform, false, false, true);
			return;
		}
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		if (slots.IsIndexValid(seat))
			access.GetInVehicle(vehicle, slots[seat], true, -1, ECloseDoorAfterActions.INVALID, false);
	}
}
