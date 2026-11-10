//! One shared, bounded notification stack for admin and mission messages. No gameplay input capture.
class GC_AdminMessagePopupComponent : SCR_ScriptedWidgetComponent
{
	protected static ref array<GC_AdminMessagePopupComponent> s_aVisible = {};
	protected static ref array<ref GC_AdminMessage> s_aWaiting = {};
	protected static bool s_bClosing;
	protected static bool s_bFilling;
	protected static const ResourceName LAYOUT = "{E48D67FEE0EC4C46}UI/layouts/Admin/Lobby_AdminMessage.layout";
	protected ref GC_AdminMessage m_Message;
	protected AudioHandle m_Sound = AudioHandle.Invalid;
	protected float m_fHeight = 110;
	protected bool m_bPlaced;

	//------------------------------------------------------------------------------------------------
	static void Show(GC_AdminMessage message)
	{
		if (!message || !GC_LobbyManager.GetManager())
			return;
		// History already owns the full message. Never evict a currently visible notice.
		if (s_aWaiting.Count() >= 100)
		{
			Print("GC | Notifications | Waiting queue full; message retained in history.", LogLevel.WARNING);
			return;
		}
		s_aWaiting.Insert(message);
		FillStack();
	}

	//------------------------------------------------------------------------------------------------
	protected static void FillStack()
	{
		if (s_bClosing || s_bFilling)
			return;
		if (!GC_LobbyManager.GetManager())
		{
			CloseAll();
			return;
		}
		s_bFilling = true;
		while (s_aVisible.Count() < 3 && !s_aWaiting.IsEmpty())
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets(LAYOUT, GetGame().GetWorkspace());
			if (!widget)
				break;
			GC_AdminMessagePopupComponent popup = GC_AdminMessagePopupComponent.Cast(widget.FindHandler(GC_AdminMessagePopupComponent));
			if (!popup)
			{
				widget.RemoveFromHierarchy();
				break;
			}
			popup.Prepare(s_aWaiting[0]);
			float occupied = 86;
			foreach (GC_AdminMessagePopupComponent visible : s_aVisible)
				occupied += visible.m_fHeight + 10;
			float width;
			float height;
			GetGame().GetWorkspace().GetScreenSize(width, height);
			float bottom = GetGame().GetWorkspace().DPIUnscale(height) - 40;
			if (!s_aVisible.IsEmpty() && occupied + popup.m_fHeight > bottom)
			{
				widget.RemoveFromHierarchy();
				break;
			}
			s_aWaiting.RemoveOrdered(0);
			s_aVisible.Insert(popup);
			Reposition();
			popup.BeginDisplay();
		}
		s_bFilling = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void Prepare(GC_AdminMessage message)
	{
		m_Message = message;
		TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("AdminMessageTitle"));
		TextWidget body = TextWidget.Cast(m_wRoot.FindAnyWidget("AdminMessageText"));
		string heading = "Admin - " + message.m_sSenderName;
		if (message.m_bWarning)
			heading = "Admin Warning - " + message.m_sSenderName;
		if (message.m_bMission)
		{
			heading = "Mission";
			if (message.m_bWarning)
				heading = "Mission Warning";
			if (!message.m_sTitle.IsEmpty())
				heading = message.m_sTitle;
		}
		title.SetText(heading);
		if (message.m_bWarning)
		{
			title.SetColor(Color.FromRGBA(242, 186, 77, 255));
			FrameSlot.SetOffsets(title, 54, 14, 20, -40);
			m_wRoot.FindAnyWidget("AdminMessageBackground").SetColor(Color.FromRGBA(45, 37, 18, 235));
		}
		m_wRoot.FindAnyWidget("AdminWarningIcon").SetVisible(message.m_bWarning);
		m_wRoot.FindAnyWidget("AdminWarningAccent").SetVisible(message.m_bWarning);
		body.SetText(message.m_sText);
		m_wRoot.Update();
		Widget viewport = m_wRoot.FindAnyWidget("AdminMessageScroll");
		string display = message.m_sText;
		display.Replace("\r", " ");
		if (!message.m_bPreserveLineBreaks)
			display.Replace("\n", " ");
		display.Replace("\t", " ");
		GC_AdminTextWrapping.Apply(body, viewport, display);
		m_wRoot.Update();
		float textWidth;
		float textHeight;
		body.GetTextSize(textWidth, textHeight);
		float width;
		float height;
		GetGame().GetWorkspace().GetScreenSize(width, height);
		// Keep pathological line breaks or narrow screens inside the viewport. History stays complete.
		float maximumTextHeight = Math.Max(46, GetGame().GetWorkspace().DPIUnscale(height) - 190);
		string fitted = body.GetText();
		while (textHeight > maximumTextHeight && fitted.Length() > 0)
		{
			int cut = Math.Max(0, fitted.Length() - 16);
			while (cut > 0 && (fitted.ToAscii(cut) & 192) == 128)
				cut--;
			fitted = fitted.Substring(0, cut);
			body.SetText(fitted + "...");
			body.GetTextSize(textWidth, textHeight);
		}
		m_fHeight = Math.Max(110, textHeight + 64);
		FrameSlot.SetSize(m_wRoot, Math.Min(780, GetGame().GetWorkspace().DPIUnscale(width) - 48), m_fHeight);
	}

	//------------------------------------------------------------------------------------------------
	protected void BeginDisplay()
	{
		AnimateWidget.Opacity(m_wRoot, 1, 4);
		if (m_Message.m_bWarning)
			m_Sound = AudioSystem.PlaySound("{4819D5A581AF66BB}Sounds/UI/Samples/Editor/UI_E_Notification_Warning.wav");
		else
			m_Sound = AudioSystem.PlaySound("{06C02AFB2CA882EB}Sounds/UI/Samples/Menu/UI_Task_Created.wav");
		GetGame().GetCallqueue().CallLater(FadeOut, 14000);
		GetGame().GetCallqueue().CallLater(Expire, 15000);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Reposition()
	{
		float top = 86;
		foreach (GC_AdminMessagePopupComponent popup : s_aVisible)
		{
			if (popup.m_bPlaced)
			{
				float position[2] = {0, top};
				AnimateWidget.Position(popup.m_wRoot, position, 8);
			}
			else
			{
				FrameSlot.SetPos(popup.m_wRoot, 0, top);
				popup.m_bPlaced = true;
			}
			top += popup.m_fHeight + 10;
		}
	}

	//------------------------------------------------------------------------------------------------
	static void CloseAll()
	{
		s_bClosing = true;
		s_aWaiting.Clear();
		GetGame().GetCallqueue().Remove(FillStack);
		while (!s_aVisible.IsEmpty())
		{
			GC_AdminMessagePopupComponent popup = s_aVisible[0];
			s_aVisible.RemoveOrdered(0);
			if (popup && popup.m_wRoot)
				popup.m_wRoot.RemoveFromHierarchy();
		}
		s_bClosing = false;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		float width;
		float height;
		GetGame().GetWorkspace().GetScreenSize(width, height);
		FrameSlot.SetSize(w, Math.Min(780, GetGame().GetWorkspace().DPIUnscale(width) - 48), 110);
		w.SetOpacity(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void FadeOut()
	{
		if (!GC_LobbyManager.GetManager())
		{
			CloseAll();
			return;
		}
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
		if (m_Sound != AudioHandle.Invalid)
			AudioSystem.TerminateSound(m_Sound);
		bool displayed = s_aVisible.Contains(this);
		s_aVisible.RemoveItem(this);
		m_Message = null;
		if (displayed && !s_bClosing)
		{
			Reposition();
			GetGame().GetCallqueue().CallLater(FillStack, 0);
		}
		super.HandlerDeattached(w);
	}
}
