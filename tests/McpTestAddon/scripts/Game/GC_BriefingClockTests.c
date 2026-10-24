//! Read-only observations of production timing and the active header.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestBriefingClockSnapshot()
	{
		string result = string.Format("phase=%1 elapsed=%2", GetPhaseName(GetLobbyPhase()), GetBriefingElapsedSeconds());
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return result + " header=absent";
		TextWidget clock = TextWidget.Cast(menu.GetRootWidget().FindAnyWidget("BriefingElapsed"));
		if (!clock)
			return result + " clock=absent";
		return result + string.Format(" visible=%1 text=%2", clock.IsVisible(), clock.GetText());
	}
}
