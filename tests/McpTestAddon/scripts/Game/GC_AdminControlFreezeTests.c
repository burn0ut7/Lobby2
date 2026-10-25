//! Optional fixtures invoke production requests and observe state; never grant permissions.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestControlFreezeRequest(int scope, int targetId, bool frozen, int radiusMeters, string factionKey, bool staleIdentity)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE active local controller";
		array<int> players = {};
		array<string> identities = {};
		if (scope == GC_EAdminHealScope.SELECTED)
		{
			players.Insert(targetId);
			string identity = GetPlayerVoiceIdentity(targetId);
			if (staleIdentity)
				identity = "stale-control-freeze-fixture";
			identities.Insert(identity);
		}
		int requestId = controller.ReserveAdminPlayerToolRequest();
		controller.GC_RequestAdminControlFreeze(scope, players, identities, requestId, radiusMeters, factionKey, frozen);
		return string.Format("DISPATCHED request=%1; inspect owner/server state and central audit", requestId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestControlFreezeSnapshot(int playerId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (GetManager() != this || !controller)
			return "UNAVAILABLE active player controller";
		ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
		string result = string.Format("player=%1 local=%2 server=%3 frozen=%4 combatRestricted=%5 context=%6",
			playerId, controller == GetGame().GetPlayerController(), Replication.IsServer(), controller.GC_AreAdminControlsFrozen(),
			controller.GC_IsCombatRestricted(), GetGame().GetInputManager().IsContextActive("GC_AdminControlFreeze"));
		if (!character)
			return result + " character=none";
		result += " position=" + character.GetOrigin().ToString();
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (access && access.GetCompartment())
			result += " seat=" + access.GetCompartment().GetCompartmentName();
		else
			result += " seat=on-foot";
		CharacterControllerComponent controls = character.GetCharacterController();
		BaseWeaponManagerComponent weapons = controls.GetWeaponManagerComponent();
		if (weapons && weapons.GetCurrentWeapon() && weapons.GetCurrentWeapon().GetCurrentMagazine())
			result += " ammo=" + weapons.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount().ToString();
		IEntity body = GC_GetAdminFreezeBody(controller);
		if (body && body.GetPhysics())
		{
			Physics physics = body.GetPhysics();
			result += string.Format(" heldBody=%1 simulation=%2 dynamic=%3",
				body.GetName(), physics.GetSimulationState(), physics.IsDynamic());
			if (physics.IsDynamic())
				result += string.Format(" velocity=%1 angularVelocity=%2", physics.GetVelocity(), physics.GetAngularVelocity());
		}
		return result;
	}
}
