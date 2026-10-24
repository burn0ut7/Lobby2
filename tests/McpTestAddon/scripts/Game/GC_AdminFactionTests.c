#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFactionQuotas()
	{
		array<int> quotas;
		GC_FactionBalancePolicy.Calculate({10, 30}, 20, quotas);
		if (quotas[0] != 5 || quotas[1] != 15)
			return "FAIL 1:3 ratio with twenty players";
		GC_FactionBalancePolicy.Calculate({10, 30}, 0, quotas);
		if (quotas[0] != 0 || quotas[1] != 0)
			return "FAIL zero-player quota";
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
	//! Real requester authorization; does not grant a role or bypass the production handler.
	string GC_TestRatioReject(int requesterId)
	{
		if (!Replication.IsServer() || SCR_Global.IsAdmin(requesterId))
			return "UNAVAILABLE authority and logged-out requester required";
		int revision = GetFactionRulesRevision();
		int lockMode = GetFactionLockOverride();
		bool balance = IsFactionBalanceEnforced();
		GC_TestSlotAccessRemember();
		string outcome = ExecuteAdminFactionRules(requesterId, revision, lockMode, !balance);
		if (outcome != "rejected-not-logged-in" || revision != GetFactionRulesRevision()
			|| lockMode != GetFactionLockOverride() || balance != IsFactionBalanceEnforced())
			return "FAIL ratio rejection mutated rules";
		return GC_TestSlotAccessCheck("unchanged", false);
	}

	//------------------------------------------------------------------------------------------------
	//! Inspect actual widgets; use GC_TestClick for the production button handler.
	string GC_TestRatioUI()
	{
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE lobby menu";
		Widget ratio = root.FindAnyWidget("FactionRatioButton");
		Widget locks = root.FindAnyWidget("AllSlotLocksButton");
		TextWidget status = TextWidget.Cast(root.FindAnyWidget("AllSlotLocksStatus"));
		if (!ratio || !locks || !status)
			return "UNAVAILABLE lobby controls";
		return string.Format("ratioVisible=%1 ratioEnabled=%2 lockVisible=%3 lockEnabled=%4 ratioOn=%5 revision=%6 status=%7",
			GC_TestVisible(ratio), ratio.IsEnabled(), GC_TestVisible(locks), locks.IsEnabled(),
			IsFactionBalanceEnforced(), GetFactionRulesRevision(), status.GetText());
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
