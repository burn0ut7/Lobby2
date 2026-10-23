//! Optional semantic checks; no roles are granted and nothing runs automatically.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Presentation-only fixture; does not rename a player or change readiness.
	string GC_TestReadinessLongName()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		TextWidget label = TextWidget.Cast(menu.GetRootWidget().FindAnyWidget("ReadinessCommander"));
		if (!label)
			return "UNAVAILABLE commander label";
		label.SetText("CO: Lieutenant Colonel VeryLongCommanderName");
		return "FIXTURE commander label only; normal refresh restores it";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestReadinessSnapshot()
	{
		string result = string.Format("phase=%1 revision=%2", GetPhaseName(GetLobbyPhase()), GetFactionReadinessRevision());
		foreach (Faction faction : GetRegisteredFactions())
		{
			string key = faction.GetFactionKey();
			GC_Slot slot = GetFactionCommanderSlot(key);
			int slotId = -1;
			if (slot)
				slotId = slot.GetID();
			result += string.Format("\nfaction=%1 commandSlot=%2 commander=%3 ready=%4", key, slotId, GetFactionCommanderPlayerId(key), IsFactionReady(key));
		}
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			result += string.Format("\nlocalPlayer=%1 pending=%2 error=%3", controller.GetPlayerId(), controller.IsFactionReadinessPending(), controller.GetFactionReadinessError());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Send through the local owning controller, including deliberately stale inputs.
	string GC_TestReadinessRequest(string factionKey, int slotId, int revision, bool ready)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || GetManager() != this)
			return "UNAVAILABLE local Lobby2 controller";
		controller.RequestFactionReadiness(factionKey, slotId, revision, ready);
		return "DISPATCHED production controller request; inspect receipt and snapshot";
	}

	//------------------------------------------------------------------------------------------------
	//! Exercises the existing disconnect reconciliation without faking a network disconnect.
	string GC_TestReadinessDisconnect()
	{
		if (!Replication.IsServer() || GetManager() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE local authority";
		HandlePlayerDisconnected(GetGame().GetPlayerController().GetPlayerId());
		return "SIMULATED production disconnect handler; " + GC_TestReadinessSnapshot();
	}
}
