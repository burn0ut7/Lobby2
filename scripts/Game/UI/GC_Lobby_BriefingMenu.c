//! The preview and briefing screens share the same menu implementation and
//! layout structure. The concrete menu class supplies local view identity so
//! reopening Briefing during GAME or POSTGAME does not fall back to Preview
//! merely because the authoritative mission phase has advanced.
class GC_Lobby_BriefingMenu : GC_Lobby_PreviewMenu
{
	override protected bool IsBriefingView()
	{
		return true;
	}
}
