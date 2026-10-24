//! Optional semantic checks; no roles are granted and nothing runs automatically.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Native UI hit test followed by the resolved button's normal click handler.
	string GC_TestReadinessSurface(string factionKey, string surface, bool activate)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget root = menu.GetRootWidget();
		Widget button = root.FindAnyWidget("ReadinessAction_" + factionKey);
		if (!button)
			return "UNAVAILABLE faction tile";
		Widget target = button;
		if (!surface.IsEmpty())
			target = button.GetParent().FindAnyWidget(surface);
		if (!target)
			return "UNAVAILABLE tile surface";
		float x, y, width, height;
		target.GetScreenPos(x, y);
		target.GetScreenSize(width, height);
		int mouseX = x + width * 0.5;
		int mouseY = y + height * 0.5;
		WidgetManager.ReportMouse(mouseX, mouseY, root);
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		string hitName = "none";
		if (hit)
			hitName = hit.GetName();
		string result = string.Format("surface=%1 hit=%2 enabled=%3", surface, hitName, button.IsEnabled());
		if (!activate || hit != button || !button.IsEnabled())
			return result + " no activation";
		SCR_ButtonBaseComponent component = SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent));
		if (!component)
			return result + " UNAVAILABLE native handler";
		component.OnClick(hit, mouseX, mouseY, 0);
		return result + " dispatched native click; " + GC_TestReadinessSnapshot();
	}

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
