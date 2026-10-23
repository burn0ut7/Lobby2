#ifdef WORKBENCH
//! Optional fixtures and observations; actions run through production context/RPC.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSlotRecovery(int slotId, string operation)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || GetManager() != this || !Replication.IsServer())
			return "UNAVAILABLE slot/authority";
		if (operation == "open")
			return string.Format("opened=%1", GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForSlot(slot), GC_TestMenuRoot()));
		if (operation == "close")
			GC_ContextMenuComponent.Close();
		if (operation == "remove" || operation == "restore")
		{
			GC_EAdminSlotAction actionType = GC_EAdminSlotAction.HEAL_OR_RESPAWN;
			if (operation == "remove")
				actionType = GC_EAdminSlotAction.FREE;
			GC_AdminSlotPlayerAction action = new GC_AdminSlotPlayerAction(actionType);
			GC_ContextMenuContext context = GC_ContextMenuContext.ForSlot(slot);
			bool enabled = action.CanExecute(context);
			action.Execute(context);
			return string.Format("label=%1 enabled=%2; inspect state and audit", action.GetLabel(context), enabled);
		}
		if (operation == "spawn-fixture" && !slot.GetEntity())
			SpawnSlotEntity(slot, true);
		SCR_CharacterDamageManagerComponent damage;
		if (slot.GetEntity())
			damage = SCR_CharacterDamageManagerComponent.Cast(slot.GetEntity().FindComponent(SCR_CharacterDamageManagerComponent));
		if (damage && operation == "damage-fixture")
			damage.GetDefaultHitZone().SetHealthScaled(0.6);
		if (damage && operation == "kill-fixture")
			damage.SetHealthScaled(0);
		string result = GC_TestAdminSlotSnapshot(slotId);
		if (damage)
		{
			array<HitZone> zones = {};
			damage.GetAllHitZonesInHierarchy(zones);
			int damaged;
			foreach (HitZone zone : zones)
			{
				if (zone && zone.GetDamageState() != EDamageState.UNDAMAGED)
					damaged++;
			}
			result += string.Format(" health=%1 damaged=%2", damage.GetHealthScaled(), damaged);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSlotRecoveryReject(int requesterId, int slotId, int action, bool stale)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!slot || !Replication.IsServer())
			return "UNAVAILABLE slot/authority";
		string before = GC_TestSlotRecovery(slotId, "snapshot");
		string claim = slot.GetUUID();
		if (stale)
			claim = "00000000-0000-0000-0000-000000000001";
		string outcome = ExecuteAdminSlotAction(requesterId, slotId, claim, action);
		return string.Format("outcome=%1 unchanged=%2", outcome, before == GC_TestSlotRecovery(slotId, "snapshot"));
	}
}
#endif
