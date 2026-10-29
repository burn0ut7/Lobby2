modded class GC_LobbyPlayerController
{
	protected int m_iGC_TestHealthPackets;
	protected int m_iGC_TestFocusPackets;

	string GC_TestFocus(string action, int playerId)
	{
		if (action == "select")
			GC_SelectSpectatorPlayer(playerId);
		else if (action == "world")
			GC_ToggleWorldHealth();
		else if (action == "more")
			GC_LoadFocusedEventPage();
		else if (action == "unauthorized")
			Rpc(GC_RpcAsk_FocusSettings, m_iGC_FocusGeneration, playerId);
		else if (action == "away" && m_SpectatorCamera)
		{
			m_SpectatorCamera.SetFreeMode();
			m_SpectatorCamera.SetOrigin(m_SpectatorCamera.GetOrigin() + "3000 100 3000");
		}
		else if (action == "jump")
			GC_RequestSpectatorPlayerPosition(playerId);
		else if (action == "frame" && m_SpectatorCamera)
		{
			GC_Slot slot = GC_LobbyManager.GetManager().GetSlotByPlayerId(playerId);
			if (!slot || !slot.GetEntity())
				return "NO CHARACTER";
			vector focus = slot.GetEntity().GetOrigin() + "0 2.1 0";
			vector transform[4];
			transform[3] = focus + "0 1 -10";
			Math3D.AnglesToMatrix((focus - transform[3]).VectorToAngles(), transform);
			m_SpectatorCamera.SetFreeMode();
			m_SpectatorCamera.SetTransform(transform);
		}
		string report = string.Format("state=%1 focus=%2 health=%3 world=%4 rows=%5 next=%6", GetLobbyPlayerState(), m_iGC_FocusedPlayer, GC_GetPlayerHealth(playerId), m_bGC_WorldHealth, m_aGC_FocusEvents.Count() / 7, m_iGC_FocusNext);
		if (m_SpectatorCamera)
			report += string.Format(" camera=%1 request=%2 localTarget=%3", m_SpectatorCamera.GetOrigin(), m_iGC_SpectatorJumpRequest, GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId) != null);
		report += string.Format(" healthPackets=%1 eventPackets=%2 serverFocus=%3 serverHealth=%4", m_iGC_TestHealthPackets, m_iGC_TestFocusPackets, m_iGC_ServerFocus, m_mGC_ServerHealth.Count());
		if (!m_aGC_FocusEvents.IsEmpty())
			report += " first=" + m_aGC_FocusEvents[2] + " " + m_aGC_FocusEvents[3];
		return report;
	}


	override protected void GC_RpcDo_HealthChanges(int generation, array<int> changes)
	{
		m_iGC_TestHealthPackets++;
		super.GC_RpcDo_HealthChanges(generation, changes);
	}

	override protected void GC_RpcDo_FocusEvents(int generation, int before, array<string> rows, int next, string status)
	{
		m_iGC_TestFocusPackets++;
		super.GC_RpcDo_FocusEvents(generation, before, rows, next, status);
	}
}

#ifdef WORKBENCH
modded class GC_Lobby_SpectatorMenu
{
	string GC_TestWorldFocusClick(string widgetName)
	{
		if (widgetName == "release")
		{
			GetGame().GetInputManager().SetActionValue("GC_SpectatorWorldClick", 0);
			return "RELEASED";
		}
		float x = 200, y = 200;
		if (widgetName != "world")
		{
			Widget widget = GetGame().GetWorkspace().FindAnyWidget(widgetName);
			if (!widget || !widget.IsVisibleInHierarchy())
				return "NO VISIBLE WIDGET";
			float width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			x += width * 0.5;
			y += height * 0.5;
		}
		GetGame().GetInputManager().GetMouseDeviceHandler().SetCursorPosition(x, y);
		WidgetManager.ReportMouse(x, y, GetGame().GetWorkspace());
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		string hitName = "none";
		if (hit)
			hitName = hit.GetName();
		GetGame().GetInputManager().SetActionValue("GC_SpectatorWorldClick", 1);
		return "hit=" + hitName + " focus=" + m_HealthController.GC_GetFocusedPlayer().ToString();
	}
}

modded class GC_LobbyManager
{
	string GC_TestWorldFocusClick(string widgetName)
	{
		GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!menu)
			return "NO SPECTATOR MENU";
		return menu.GC_TestWorldFocusClick(widgetName);
	}
	//! Observe native hit routing, then separately exercise its production handler.
	string GC_TestFocusWidget(string rowName, string cellName, bool activate)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "NO MENU";
		Widget root = GetGame().GetWorkspace();
		Widget row = root.FindAnyWidget(rowName);
		return GC_TestFocusHit(row, cellName, activate);
	}

	string GC_TestFocusMarker(int playerId)
	{
		Widget row = GC_TestFindFocusMarker(GetGame().GetWorkspace(), playerId);
		return GC_TestFocusHit(row, "LobbySpectatorMarkerRoleIcon", true);
	}

	protected Widget GC_TestFindFocusMarker(Widget widget, int playerId)
	{
		GC_SpectatorCharacterEditableEntityUIComponent marker = GC_SpectatorCharacterEditableEntityUIComponent.Cast(widget.FindHandler(GC_SpectatorCharacterEditableEntityUIComponent));
		if (marker && marker.GetEntity() && GC_LobbyPlayerController.GC_GetSpectatorEntityPlayer(marker.GetEntity().GetOwner()) == playerId)
			return widget;
		Widget child = widget.GetChildren();
		while (child)
		{
			Widget found = GC_TestFindFocusMarker(child, playerId);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	protected string GC_TestFocusHit(Widget row, string cellName, bool activate)
	{
		Widget root = GetGame().GetWorkspace();
		if (!row)
			return "NO ROW";
		Widget cell = row.FindAnyWidget(cellName);
		if (!cell)
			return "NO CELL";
		float x, y, width, height;
		cell.GetScreenPos(x, y);
		cell.GetScreenSize(width, height);
		WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, root);
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		if (!hit)
			return "NO HIT";
		string report = "hit=" + hit.GetName() + " button=" + (ButtonWidget.Cast(hit) != null).ToString();
		if (!activate || !ButtonWidget.Cast(hit))
			return report;
		Widget current = hit;
		while (current)
		{
			GC_VoicePlayerRowComponent playerRow = GC_VoicePlayerRowComponent.Cast(current.FindHandler(GC_VoicePlayerRowComponent));
			if (playerRow)
				return report + " handled=" + playerRow.OnClick(hit, x, y, 0).ToString();
			GC_SpectatorCharacterEditableEntityUIComponent marker = GC_SpectatorCharacterEditableEntityUIComponent.Cast(current.FindHandler(GC_SpectatorCharacterEditableEntityUIComponent));
			if (marker)
				return report + " player=" + GC_LobbyPlayerController.GC_GetSpectatorEntityPlayer(marker.GetEntity().GetOwner()).ToString() + " handled=" + marker.OnClick(hit, x, y, 0).ToString();
			current = current.GetParent();
		}
		return report + " NO HANDLER";
	}

	string GC_TestFocus(int requester, string action, int target)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requester));
		if (!controller)
			return "NO CONTROLLER";
		return controller.GC_TestFocus(action, target);
	}

	string GC_TestFocusRow(int target)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "NO MENU";
		Widget row = menu.GetRootWidget().FindAnyWidget("FactionPlayer_" + target.ToString());
		if (!row)
			return "NO ROW";
		GC_VoicePlayerRowComponent handler = GC_VoicePlayerRowComponent.Cast(row.FindHandler(GC_VoicePlayerRowComponent));
		if (!handler)
			return "NO HANDLER";
		return "handled=" + handler.OnClick(row, 0, 0, 0).ToString();
	}

	string GC_TestFocusBurst(int target, int count)
	{
		if (count < 1 || count > 50)
			return "INVALID COUNT";
		for (int i; i < count; i++)
		{
			GC_TestFocusHealth(target, 1);
			GC_TestLogDamage(target, 1);
		}
		return "DISPATCHED native damage count=" + count.ToString();
	}

	//! Native authoritative hit-zone mutation for health/healing display verification.
	string GC_TestFocusHealth(int target, float health)
	{
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "DENIED";
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(target);
		if (!entity || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return "NO CHARACTER";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "NO DAMAGE MANAGER";
		damage.SetHealthScaled(Math.Clamp(health, 0, 1));
		return "health=" + damage.GetHealthScaled().ToString();
	}
}
#endif
