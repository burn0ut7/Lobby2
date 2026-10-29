//! One small fixed display; world bars use the same authoritative cached values.
class GC_SpectatorHealthComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (m_Controller)
		{
			m_Controller.m_OnGC_SpectatorFocus.Insert(Refresh);
			m_Controller.m_OnGC_SpectatorHealth.Insert(Refresh);
		}
		Refresh();
	}

	protected void Refresh()
	{
		bool visible = m_Controller && m_Controller.GC_GetFocusedPlayer() > 0;
		m_wRoot.SetVisible(visible);
		if (!visible)
			return;
		int id = m_Controller.GC_GetFocusedPlayer();
		TextWidget.Cast(m_wRoot.FindAnyWidget("FocusName")).SetText(GetGame().GetPlayerManager().GetPlayerName(id));
		int value = m_Controller.GC_GetPlayerHealth(id);
		string label = "Unavailable";
		int percent;
		if (value >= 0)
		{
			percent = value % 300;
			label = percent.ToString() + "%";
			if (value >= 900)
				label = "Bleeding  " + label;
			else if (value >= 600)
				label = "Unconscious  " + label;
			else if (value >= 300)
				label = "Dead";
		}
		TextWidget.Cast(m_wRoot.FindAnyWidget("FocusHealthText")).SetText(label);
		Widget fill = m_wRoot.FindAnyWidget("FocusHealthFill");
		fill.SetVisible(percent > 0);
		fill.SetColor(HealthColor(value));
		m_wRoot.FindAnyWidget("FocusHealthTrack").SetColor(HealthTrackColor(value));
		FrameSlot.SetSize(fill, 296 * percent * 0.01, 6);
	}

	static Color HealthColor(int value)
	{
		if (value < 0)
			return Color.FromRGBA(120, 120, 120, 255);
		int percent = value % 300;
		if (value >= 300 && value < 600)
			return Color.FromRGBA(220, 55, 45, 255);
		if (percent > 66)
			return Color.FromRGBA(110, 205, 115, 255);
		if (percent > 33)
			return Color.FromRGBA(235, 184, 65, 255);
		return Color.FromRGBA(230, 95, 45, 255);
	}

	static Color HealthTrackColor(int value)
	{
		if (value >= 300 && value < 600)
			return HealthColor(value);
		return Color.FromRGBA(45, 45, 45, 220);
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_Controller)
		{
			m_Controller.m_OnGC_SpectatorFocus.Remove(Refresh);
			m_Controller.m_OnGC_SpectatorHealth.Remove(Refresh);
		}
		super.HandlerDeattached(w);
	}
}
