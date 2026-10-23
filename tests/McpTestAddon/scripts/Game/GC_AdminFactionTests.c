#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFactionQuotas()
	{
		array<int> quotas;
		GC_FactionBalancePolicy.Calculate({10, 32}, 1, quotas);
		if (quotas[0] != 0 || quotas[1] != 1)
			return "FAIL 10:32 one-player quota";
		GC_FactionBalancePolicy.Calculate({10, 32}, 3, quotas);
		if (quotas[0] != 1 || quotas[1] != 2)
			return "FAIL 10:32 three-player quota";
		GC_FactionBalancePolicy.Calculate({1, 1, 1}, 2, quotas);
		if (quotas[0] != 1 || quotas[1] != 1 || quotas[2] != 0)
			return "FAIL tied remainder ordering";
		GC_FactionBalancePolicy.Calculate({0, 0}, 5, quotas);
		if (quotas[0] != 0 || quotas[1] != 0)
			return "FAIL empty capacity";
		return "PASS proportional quotas, deterministic ties and empty capacities";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminFactionRules(int mode, bool balance)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		controller.RequestAdminFactionRules(GetFactionRulesRevision(), mode, balance);
		return string.Format("revision=%1 mode=%2 locked=%3 balance=%4", GetFactionRulesRevision(), GetFactionLockOverride(), IsFactionSwitchLocked(), IsFactionBalanceEnforced());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFactionRuleReject()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller)
			return "UNAVAILABLE authority/controller";
		int revision = GetFactionRulesRevision();
		string denied = ExecuteAdminFactionRules(-1, revision, 1, true);
		string stale = ExecuteAdminFactionRules(controller.GetPlayerId(), revision - 1, 1, true);
		string invalid = ExecuteAdminFactionRules(controller.GetPlayerId(), revision, 3, true);
		if (revision != GetFactionRulesRevision() || denied != "rejected-not-logged-in" || stale != "rejected-stale-rules" || invalid != "rejected-faction-rules")
			return "FAIL rejected requests mutated faction rules";
		return "PASS invalid actor, stale revision, invalid mode preserve rules";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFactionSlotChoices()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return "UNAVAILABLE controller";
		string result = string.Format("phase=%1 revision=%2 mode=%3 locked=%4 balance=%5", GetPhaseName(GetLobbyPhase()), GetFactionRulesRevision(), GetFactionLockOverride(), IsFactionSwitchLocked(), IsFactionBalanceEnforced());
		array<string> seen = {};
		foreach (GC_Slot slot : GetSlots())
		{
			if (!slot.GetGroup() || !slot.GetGroup().GetFaction())
				continue;
			string key = slot.GetGroup().GetFaction().GetFactionKey();
			if (seen.Contains(key))
				continue;
			seen.Insert(key);
			result += string.Format(" | %1 slot=%2 allowed=%3", key, slot.GetID(), CanPlayerRequestSlot(controller.GetPlayerId(), slot));
		}
		GC_Slot current = GetSlotByPlayerId(controller.GetPlayerId());
		if (current)
			result += string.Format(" | current=%1 canLeave=%2", current.GetID(), CanPlayerRequestSlot(controller.GetPlayerId(), current));
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestOrdinarySlot(int slotId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		controller.RequestLobbySlot(slotId);
		GC_Slot current = GetSlotByPlayerId(controller.GetPlayerId());
		if (!current)
			return "current=none";
		return "current=" + current.GetID().ToString();
	}
}
#endif
