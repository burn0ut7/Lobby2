#ifdef WORKBENCH
//! Opt-in production UI handlers and delivery observations; no role grants.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMessageComposer(string operation, string value)
	{
		if (operation == "reject-invalid")
		{
			array<string> nobody = {};
			int requester = GetGame().GetPlayerController().GetPlayerId();
			return SendAdminMessage(requester, GC_EAdminMessageAudience.ALL_PLAYERS, value, nobody);
		}
		if (operation == "context")
		{
			GC_ContextMenuContext context = GC_ContextMenuContext.ForPlayer(value.ToInt());
			GC_AdminSendMessageAction action = new GC_AdminSendMessageAction();
			if (!action.CanExecute(context))
				return "UNAVAILABLE context";
			action.Execute(context);
		}
		Widget root = GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		if (operation == "text")
		{
			MultilineEditBoxWidget field = MultilineEditBoxWidget.Cast(root.FindAnyWidget("MessageText"));
			if (!field)
				return "UNAVAILABLE field";
			field.SetText(value);
			SCR_EventHandlerComponent.Cast(field.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(field);
		}
		if (operation == "faction")
		{
			Widget rows = root.FindAnyWidget("MessageFactionRows");
			Widget row = rows.GetChildren();
			int index = value.ToInt();
			for (int i; row && i < index; i++)
				row = row.GetSibling();
			if (!row)
				return "UNAVAILABLE faction";
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
			button.m_OnClicked.Invoke(button);
		}
		Widget popup = GetGame().GetWorkspace().FindAnyWidget("AdminMessagePopup");
		string report = GC_TestAdminMessageSnapshot();
		if (popup)
			report += string.Format(" popup=1 opacity=%1", popup.GetOpacity());
		else
			report += " popup=0";
		Widget title = root.FindAnyWidget("ToolTitle");
		if (title)
		{
			report += " title=" + TextWidget.Cast(title).GetText();
			report += " status=" + TextWidget.Cast(root.FindAnyWidget("ToolStatus")).GetText();
			report += string.Format(" apply=%1", root.FindAnyWidget("ToolApply").IsEnabled());
		}
		return report;
	}
}
#endif
