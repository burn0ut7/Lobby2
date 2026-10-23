#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected IEntity m_GC_TestRememberedSlotEntity;

	//------------------------------------------------------------------------------------------------
	//! Zero selects all groups; a positive slot selects its actual group.
	string GC_TestAdminGroupLock(int slotId, bool locked)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";

		RplId groupId = RplId.Invalid();
		if (slotId != 0)
		{
			GC_Slot slot = GetSlot(slotId);
			if (!slot || slot.GetGroupRplId() == RplId.Invalid())
				return "UNAVAILABLE slot/group";
			groupId = slot.GetGroupRplId();
		}

		controller.RequestAdminGroupLock(groupId, locked);
		return "DISPATCHED inspect roster and central audit";
	}

	//------------------------------------------------------------------------------------------------
	//! Fixture preparation, never used to authorize or implement the tested action.
	string GC_TestActivateRememberedAI()
	{
		IEntity entity = m_GC_TestRememberedSlotEntity;
		if (!Replication.IsServer() || !entity || GetSlotByEntity(entity))
			return "UNAVAILABLE unregistered fixture required";
		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!aiControl)
			return "UNAVAILABLE AI control";
		aiControl.ActivateAI();
		return string.Format("fixture-ai=%1", aiControl.IsAIActivated());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDuplicatePlayableReject()
	{
		IEntity entity = m_GC_TestRememberedSlotEntity;
		GC_Slot slot = GetSlotByEntity(entity);
		if (!Replication.IsServer() || !slot || !GetGame().GetPlayerController())
			return "UNAVAILABLE converted fixture";
		int before = GetSlots().Count();
		string result = ExecuteAdminMakePlayable(GetGame().GetPlayerController().GetPlayerId(), slot.GetEntityRplId(), slot.GetGroupRplId());
		if (result != "rejected-incompatible-character" || GetSlots().Count() != before || GetSlotByEntity(entity) != slot)
			return "FAIL duplicate conversion";
		return "PASS duplicate conversion rejected with same slot and character";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMakeRememberedPlayable(bool review)
	{
		IEntity entity = m_GC_TestRememberedSlotEntity;
		SCR_AIGroup group = GetPlayableCharacterGroup(entity);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!group || !controller)
			return "UNAVAILABLE compatible remembered character/controller";
		RplComponent replication = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (review)
		{
			GC_AdminMakePlayableAction action = new GC_AdminMakePlayableAction();
			action.Execute(GC_ContextMenuContext.ForEntity(entity));
			return "DISPATCHED production context confirmation";
		}
		controller.RequestAdminMakePlayable(replication.Id(), Replication.FindItemId(group));
		return GC_TestConvertedCharacterState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestConvertedCharacterState()
	{
		IEntity entity = m_GC_TestRememberedSlotEntity;
		if (!entity)
			return "entity=absent";
		GC_Slot slot = GetSlotByEntity(entity);
		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		int slotId = -1;
		if (slot)
			slotId = slot.GetID();
		return string.Format("slots=%1 entity=present slot=%2 position=%3 ai=%4", GetSlots().Count(), slotId, entity.GetOrigin(), aiControl && aiControl.IsAIActivated());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMakePlayableReject()
	{
		IEntity entity = m_GC_TestRememberedSlotEntity;
		if (!entity || !GetGame().GetPlayerController())
			return "UNAVAILABLE fixture";
		RplComponent replication = RplComponent.Cast(entity.FindComponent(RplComponent));
		int countBefore = GetSlots().Count();
		vector positionBefore = entity.GetOrigin();
		int requester = GetGame().GetPlayerController().GetPlayerId();
		string unauthorized = ExecuteAdminMakePlayable(-1, replication.Id(), RplId.Invalid());
		string stale = ExecuteAdminMakePlayable(requester, RplId.Invalid(), RplId.Invalid());
		string incompatible = ExecuteAdminMakePlayable(requester, replication.Id(), RplId.Invalid());
		if (unauthorized != "rejected-not-logged-in" || stale != "rejected-stale-character" || incompatible != "rejected-incompatible-character")
			return "FAIL outcome";
		if (GetSlots().Count() != countBefore || entity.GetOrigin() != positionBefore)
			return "FAIL rejection mutated roster/character";
		return "PASS unauthorized, stale character and stale group rejected without mutation";
	}

	//------------------------------------------------------------------------------------------------
	//! Fixture movement only: subsequent respawn uses the production controller action.
	string GC_TestMoveSlotCharacter(int slotId, vector position)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!Replication.IsServer() || !slot || !slot.GetEntity())
			return "UNAVAILABLE authority/slot/entity";
		ChimeraCharacter character = ChimeraCharacter.Cast(slot.GetEntity());
		if (!character || character.GetCharacterController().IsPlayerControlled())
			return "UNAVAILABLE uncontrolled character required";
		character.SetOrigin(position);
		return string.Format("fixture-position=%1", character.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminCreateSlot(int templateSlotId, vector position)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot source = GetSlot(templateSlotId);
		if (!controller || !source)
			return "UNAVAILABLE controller/template";
		int before = GetSlots().Count();
		controller.RequestAdminCreateSlot(templateSlotId, source.GetGroupRplId(), position);
		return string.Format("DISPATCHED slotsBefore=%1 slotsNow=%2 inspect roster/log", before, GetSlots().Count());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminCreateReject(int templateSlotId)
	{
		GC_Slot source = GetSlot(templateSlotId);
		PlayerController controller = GetGame().GetPlayerController();
		if (!source || !controller || !Replication.IsServer())
			return "UNAVAILABLE authority/controller/template";
		int before = GetSlots().Count();
		string denied = ExecuteAdminCreateSlot(-1, templateSlotId, source.GetGroupRplId(), vector.Zero);
		string stale = ExecuteAdminCreateSlot(controller.GetPlayerId(), templateSlotId, RplId.Invalid(), vector.Zero);
		string outside = ExecuteAdminCreateSlot(controller.GetPlayerId(), templateSlotId, source.GetGroupRplId(), "100000000 100000000 100000000");
		if (before != GetSlots().Count() || denied != "rejected-not-logged-in" || stale != "rejected-stale-template" || outside != "rejected-spawn-position")
			return string.Format("FAIL before=%1 after=%2 denied=%3 stale=%4 outside=%5", before, GetSlots().Count(), denied, stale, outside);
		return "PASS invalid requester, stale template group and outside-world position leave roster unchanged";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSlotCreationState()
	{
		vector minimum;
		vector maximum;
		GetGame().GetWorld().GetBoundBox(minimum, maximum);
		string result = string.Format("slots=%1 bounds=%2..%3", GetSlots().Count(), minimum, maximum);
		foreach (GC_Slot slot : GetSlots())
		{
			if (slot.GetAdminSpawnPoint())
				result += string.Format(" | created=%1 entity=%2 original=%3", slot.GetID(), slot.GetEntityRplId(), slot.GetAdminSpawnPoint().CreateParams().Transform[3]);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminAssignSelf(int slotId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot slot = GetSlot(slotId);
		if (!controller || !slot)
			return "UNAVAILABLE controller/slot";
		controller.RequestAdminAssignSlot(controller.GetPlayerId(), GetPlayerVoiceIdentity(controller.GetPlayerId()), slotId, slot.GetUUID());
		return "DISPATCHED inspect assignment";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminAssignPlayer(int playerId, int slotId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot slot = GetSlot(slotId);
		if (!controller || !slot)
			return "UNAVAILABLE controller/slot";
		controller.RequestAdminAssignSlot(playerId, GetPlayerVoiceIdentity(playerId), slotId, slot.GetUUID());
		return "DISPATCHED inspect authority and remote assignment";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSlotEntityDetails(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !slot.GetEntity())
			return "UNAVAILABLE slot/entity";
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(slot.GetPlayerId());
		bool controlled = controller && controller.GetControlledEntity() == slot.GetEntity();
		return string.Format("entity=%1 origin=%2 controlled=%3 group=%4", slot.GetEntityRplId(), slot.GetEntity().GetOrigin(), controlled, slot.GetGroupRplId());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRememberSlotEntity(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !slot.GetEntity())
			return "UNAVAILABLE slot/entity";
		m_GC_TestRememberedSlotEntity = slot.GetEntity();
		return "remembered";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRememberedSlotEntity()
	{
		if (!m_GC_TestRememberedSlotEntity)
			return "entity=absent";
		return string.Format("entity=present origin=%1", m_GC_TestRememberedSlotEntity.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminSlot(int slotId, int action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot slot = GetSlot(slotId);
		if (!controller || !slot)
			return "UNAVAILABLE controller or slot";
		controller.RequestAdminSlot(slotId, slot.GetUUID(), action);
		return "DISPATCHED inspect slot snapshot and central log";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminSlotSnapshot(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot)
			return "slot=absent";
		PlayerController controller = GetGame().GetPlayerController();
		int playerId = -1;
		if (controller)
			playerId = controller.GetPlayerId();
		return string.Format("slot=%1 player=%2 state=%3 locked=%4 pinned=%5 selectable=%6 entity=%7", slotId, slot.GetPlayerId(), slot.GetState(), slot.IsAdminLocked(), slot.IsAssignmentPinned(), CanPlayerRequestSlot(playerId, slot), slot.GetEntityRplId());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminSlotReject(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !Replication.IsServer())
			return "UNAVAILABLE slot or authority";
		bool before = slot.IsAdminLocked();
		GC_EAdminSlotAction action = GC_EAdminSlotAction.LOCK;
		if (before)
			action = GC_EAdminSlotAction.UNLOCK;
		string outcome = ExecuteAdminSlotAction(-1, slotId, slot.GetUUID(), action);
		if (outcome != "rejected-not-logged-in" || slot.IsAdminLocked() != before)
			return "FAIL invalid requester changed slot";
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return "UNAVAILABLE local controller";
		outcome = ExecuteAdminSlotAction(controller.GetPlayerId(), slotId, "00000000-0000-0000-0000-000000000001", action);
		if (outcome != "rejected-stale-slot" || slot.IsAdminLocked() != before)
			return "FAIL stale claimant changed slot";
		return "PASS unknown requester and stale claimant rejected without mutation";
	}
}
#endif
