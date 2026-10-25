//! Explicit layer observations and native notification/input probes; test addon only.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestUILayers(string action)
	{
		if (GetManager() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE active Lobby2";

		if (action == "notify")
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("Notification layer test", 60, "Must remain behind the lobby");
			SCR_NotificationsComponent.SendLocal(ENotification.PLAYER_JOINED, GetGame().GetPlayerController().GetPlayerId());
		}
		else if (action == "back-down")
			GetGame().GetInputManager().SetActionValue("MenuBack", 1);
		else if (action == "back-up")
			GetGame().GetInputManager().SetActionValue("MenuBack", 0);
		else if (action == "close-menu")
		{
			MenuBase closing = GetGame().GetMenuManager().GetTopMenu();
			if (closing)
				closing.Close();
		}
		else if (action != "snapshot")
			return "REJECTED unknown action";

		string result;
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (menu)
			result += string.Format("top=%1 focused=%2\n", menu.Type().ToString(), menu.IsFocused());
		Widget root = GetGame().GetWorkspace().GetChildren();
		while (root)
		{
			result += GC_TestUILayerWidget(root);
			Widget child = root.GetChildren();
			while (child)
			{
				result += "  " + GC_TestUILayerWidget(child);
				child = child.GetSibling();
			}
			root = root.GetSibling();
		}

		SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
		if (hud)
		{
			Widget notification = hud.FindLayoutByResourceName("{7A376F90E18453C4}UI/layouts/HUD/Notifications/Notifications_Log.layout");
			while (notification)
			{
				result += "notification: " + GC_TestUILayerWidget(notification);
				notification = notification.GetParent();
			}
		}
		Widget popup = GetGame().GetWorkspace().FindAnyWidget("Popup");
		while (popup)
		{
			result += "popup: " + GC_TestUILayerWidget(popup);
			popup = popup.GetParent();
		}
		Print("GC | UI layers | " + result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestUILayerWidget(Widget widget)
	{
		string text;
		RichTextWidget richText = RichTextWidget.Cast(widget);
		if (richText)
			text = richText.GetText();
		return string.Format("%1 z=%2 visible=%3 enabled=%4 opacity=%5 text=%6\n", widget.GetName(), widget.GetZOrder(), widget.IsVisible(), widget.IsEnabled(), widget.GetOpacity(), text);
	}
}
