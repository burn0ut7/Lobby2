//! Lobby-owned notifications above the active lobby menu, independent of the hidden HUD.
class GC_AdminMessagePopupComponent : SCR_ScriptedWidgetComponent
{
	protected static Widget s_wPopup;
	protected static ref GC_AdminMessage s_CurrentMessage;
	protected static const ResourceName LAYOUT = "{E48D67FEE0EC4C46}UI/layouts/Admin/Lobby_AdminMessage.layout";

	//------------------------------------------------------------------------------------------------
	static void Show(GC_AdminMessage message)
	{
		if (!message || !GC_LobbyManager.GetManager())
			return;

		CloseAll();
		s_wPopup = GetGame().GetWorkspace().CreateWidgets(LAYOUT, GetGame().GetWorkspace());
		if (!s_wPopup)
			return;

		s_CurrentMessage = message;
		TextWidget title = TextWidget.Cast(s_wPopup.FindAnyWidget("AdminMessageTitle"));
		TextWidget body = TextWidget.Cast(s_wPopup.FindAnyWidget("AdminMessageText"));
		if (title)
			title.SetText("Admin - " + message.m_sSenderName);
		if (body)
			body.SetText(message.m_sText);
		GetGame().GetCallqueue().CallLater(WrapLongWords, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Native wrapping only breaks at whitespace. Measure unbroken runs after layout
	//! and add display-only line breaks, leaving the received message untouched.
	protected static void WrapLongWords()
	{
		if (!s_wPopup || !s_CurrentMessage)
			return;
		TextWidget body = TextWidget.Cast(s_wPopup.FindAnyWidget("AdminMessageText"));
		Widget viewport = s_wPopup.FindAnyWidget("AdminMessageScroll");
		if (!body || !viewport)
			return;

		s_wPopup.Update();
		// Keep authored system-message sections; flatten free-form admin messages so
		// blank lines cannot push the passive banner into the view centre.
		string display = s_CurrentMessage.m_sText;
		display.Replace("\r", " ");
		if (!s_CurrentMessage.m_bPreserveLineBreaks)
			display.Replace("\n", " ");
		display.Replace("\t", " ");
		GC_AdminTextWrapping.Apply(body, viewport, display);
		s_wPopup.Update();
		float textWidth;
		float textHeight;
		body.GetTextSize(textWidth, textHeight);
		float width;
		float height;
		s_wPopup.GetScreenSize(width, height);
		FrameSlot.SetSize(s_wPopup, GetGame().GetWorkspace().DPIUnscale(width), textHeight + 64);
	}

	//------------------------------------------------------------------------------------------------
	static void CloseAll()
	{
		s_CurrentMessage = null;
		GetGame().GetCallqueue().Remove(WrapLongWords);
		if (s_wPopup)
			s_wPopup.RemoveFromHierarchy();
		s_wPopup = null;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		float width;
		float height;
		GetGame().GetWorkspace().GetScreenSize(width, height);
		FrameSlot.SetSize(w, Math.Min(780, GetGame().GetWorkspace().DPIUnscale(width) - 48), 110);
		GetGame().GetCallqueue().CallLater(Expire, 15000);
		GetGame().GetCallqueue().CallLater(FadeOut, 14000);
		AudioSystem.PlaySound("{06C02AFB2CA882EB}Sounds/UI/Samples/Menu/UI_Task_Created.wav");
	}

	//------------------------------------------------------------------------------------------------
	protected void FadeOut()
	{
		AnimateWidget.Opacity(m_wRoot, 0, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void Expire()
	{
		m_wRoot.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Expire);
		GetGame().GetCallqueue().Remove(FadeOut);
		AnimateWidget.StopAllAnimations(w);
		GetGame().GetCallqueue().Remove(WrapLongWords);
		s_wPopup = null;
		s_CurrentMessage = null;
		super.HandlerDeattached(w);
	}
}
