//! Explicit Lobby2 boundary movement; ordinary GM transforms keep their vanilla feedback.
modded class SCR_EditableEntityComponent
{
	//------------------------------------------------------------------------------------------------
	//! The caller owns the safe pose. Never transfer a character's pose to a vehicle.
	bool GC_SetBoundaryTransform(vector transform[4])
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		BaseGameEntity body = BaseGameEntity.Cast(GetOwner());
		if (!Replication.IsServer() || !mode || mode.GetState() != SCR_EGameModeState.GAME || !body)
			return false;
		RplComponent replication = body.GetRplComponent();
		if (!replication || (!ChimeraCharacter.Cast(body) && !Vehicle.Cast(body)))
			return false;

		vector previous = body.GetOrigin();
		// Same ownership route as vanilla SetTransform, without editor feedback.
		Rpc(GC_RpcDoBoundaryTransform, transform);
		if (Vehicle.Cast(body) && !System.IsCLIParam("clientVehicles"))
			GC_RpcDoBoundaryTransform(transform);
		replication.ForceNodeMovement(previous);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoBoundaryTransform(vector transform[4])
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		BaseGameEntity body = BaseGameEntity.Cast(GetOwner());
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !body)
			return;
		if (ChimeraCharacter.Cast(body))
		{
			if (CompartmentAccessComponent.GetVehicleIn(body))
				return;
			// A boundary stops translation; it must not snap the player's view around.
			vector current[4];
			body.GetWorldTransform(current);
			current[3] = transform[3];
			Math3D.MatrixCopy(current, transform);
		}
		Physics physics = body.GetPhysics();
		if (physics)
		{
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
		}
		// Use vanilla's native body operation; do not move only its rendered transform.
		body.Teleport(transform);
		if (physics)
		{
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
		}
	}
}
