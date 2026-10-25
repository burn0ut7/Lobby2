//! The preview and briefing screens share the same menu implementation and
//! layout structure. The concrete menu class supplies local view identity so
//! reopening Briefing during GAME or POSTGAME does not fall back to Preview
//! merely because the authoritative mission phase has advanced.
class GC_Lobby_BriefingMenu : GC_Lobby_PreviewMenu
{
	protected Widget m_wBriefingView;
	protected SCR_InputButtonComponent m_BriefingToggle;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wBriefingView = GetRootWidget().FindAnyWidget("BriefingView");
		Widget button = GetRootWidget().FindAnyWidget("BriefingToggle");
		if (button)
		{
			button.SetVisible(true);
			m_BriefingToggle = SCR_InputButtonComponent.Cast(button.FindHandler(SCR_InputButtonComponent));
		}
		if (m_BriefingToggle)
			m_BriefingToggle.m_OnActivated.Insert(OnToggleBriefing);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggleBriefing()
	{
		if (!IsFocused() || !m_wBriefingView)
			return;

		// Keep the menu and map alive, preserving the selected page and scroll.
		// The native button remains subscribed while its panel is hidden.
		m_wBriefingView.SetVisible(!m_wBriefingView.IsVisible());
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_BriefingToggle)
			m_BriefingToggle.m_OnActivated.Remove(OnToggleBriefing);
		m_BriefingToggle = null;
		m_wBriefingView = null;
		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	override protected bool IsBriefingView()
	{
		return true;
	}
}
