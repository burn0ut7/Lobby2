modded class GC_LobbyManager
{
	protected ref array<SCR_EditableEntityComponent> m_aGC_AdminFrozenBodies = {};
	protected ref array<int> m_aGC_AdminFreezeUnavailable = {};

	//------------------------------------------------------------------------------------------------
	override void PublishPlayerRoleChanged(int playerId, EPlayerRole roleFlags)
	{
		super.PublishPlayerRoleChanged(playerId, roleFlags);
		if (!Replication.IsServer() || GetManager() != this || !SCR_Global.IsAdmin(playerId))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller)
			controller.GC_SetAdminControlsFrozen(playerId, false);
		GC_UpdateAdminFrozenBodies();
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve seats from native compartments. A passenger never selects the vehicle body.
	protected IEntity GC_GetAdminFreezeBody(GC_LobbyPlayerController controller)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!character || !SCR_AIDamageHandling.IsAlive(character))
			return null;
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		BaseCompartmentSlot seat;
		if (access)
			seat = access.GetCompartment();
		if (!seat)
			return character;
		if (!PilotCompartmentSlot.Cast(seat) && !TurretCompartmentSlot.Cast(seat))
			return null;
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
		if (vehicle)
		{
			IEntity root = vehicle.GetRootParent();
			SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(root.FindComponent(SCR_DamageManagerComponent));
			if (!damage || damage.GetState() != EDamageState.DESTROYED)
				return root;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Aggregate occupants before releasing bodies: unfreezing one occupant cannot
	//! release a vehicle still held by another frozen driver or gunner.
	protected void GC_UpdateAdminFrozenBodies()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateAdminFrozenBodies);
		array<SCR_EditableEntityComponent> desired = {};
		array<int> unavailable = {};
		bool active;
		if (Replication.IsServer() && GetManager() == this && !m_bShutdown)
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int playerId : players)
			{
				GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				if (!controller || !controller.GC_AreAdminControlsFrozen())
					continue;
				active = true;
				IEntity body = GC_GetAdminFreezeBody(controller);
				if (!body)
					continue;
				SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(body.FindComponent(SCR_EditableEntityComponent));
				if (editable && desired.Contains(editable))
					continue;
				if (!editable || !editable.GC_SetAdminPhysicsFrozen(true))
				{
					unavailable.Insert(playerId);
					if (!m_aGC_AdminFreezeUnavailable.Contains(playerId))
						GC_AdminActionLogger.Log(0, "control-freeze-body", string.Format("player=%1", playerId), "unavailable-body-physics; input-lock-remains-active");
					continue;
				}
				desired.Insert(editable);
			}
		}
		foreach (SCR_EditableEntityComponent previous : m_aGC_AdminFrozenBodies)
		{
			if (previous && !desired.Contains(previous))
				previous.GC_SetAdminPhysicsFrozen(false);
		}
		m_aGC_AdminFrozenBodies = desired;
		m_aGC_AdminFreezeUnavailable = unavailable;
		if (active)
			GetGame().GetCallqueue().CallLater(GC_UpdateAdminFrozenBodies, 100);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateAdminFrozenBodies);
		foreach (SCR_EditableEntityComponent body : m_aGC_AdminFrozenBodies)
		{
			if (body)
				body.GC_SetAdminPhysicsFrozen(false);
		}
		m_aGC_AdminFrozenBodies.Clear();
		m_aGC_AdminFreezeUnavailable.Clear();
		super.Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	string GC_ExecuteAdminControlFreeze(int requesterId, GC_EAdminHealScope scope, array<int> selectedIds, array<string> selectedIdentities, int requestId, int radiusMeters, string factionKey, bool frozen)
	{
		string action = "control-unfreeze";
		if (frozen)
			action = "control-freeze";
		string context = string.Format("scope=%1 radius=%2 request=%3 faction=%4", scope, radiusMeters, requestId, factionKey);
		if (!AuthorizeAdminAction(requesterId, action, context))
			return "Admin login required.";

		array<int> targets;
		ChimeraCharacter origin;
		string error = GC_LobbyManager.Cast(this).GC_ResolveAdminToolTargets(requesterId, scope, selectedIds, selectedIdentities, radiusMeters, factionKey, action, context, targets, origin);
		if (!error.IsEmpty())
			return error;

		int applied;
		int skipped;
		foreach (int targetId : targets)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(targetId));
			IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId);
			if (origin && character && vector.DistanceSq(origin.GetOrigin(), character.GetOrigin()) > radiusMeters * radiusMeters)
				continue;
			string targetContext = string.Format("%1 player=%2 identity=%3", context, targetId, GetPlayerVoiceIdentity(targetId));
			if (frozen && SCR_Global.IsAdmin(targetId))
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, action, targetContext, "skipped-admin-exempt");
				continue;
			}
			if (!controller || (origin && !character))
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, action, targetContext, "skipped-unavailable");
				continue;
			}
			if (!controller.GC_SetAdminControlsFrozen(requesterId, frozen))
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, action, targetContext, "rejected-admin-session");
				continue;
			}
			applied++;
			GC_AdminActionLogger.Log(requesterId, action, targetContext, "input-lock-policy-applied");
		}
		GC_UpdateAdminFrozenBodies();
		string state = "disabled";
		if (frozen)
			state = "enabled";
		string result = string.Format("Control freeze %1 for %2 player(s). Skipped %3.", state, applied, skipped);
		if (!m_aGC_AdminFreezeUnavailable.IsEmpty())
			result += string.Format(" Body hold unavailable for %1 frozen player(s); their inputs remain blocked.", m_aGC_AdminFreezeUnavailable.Count());
		GC_AdminActionLogger.Log(requesterId, action, context, result);
		return result;
	}
}
