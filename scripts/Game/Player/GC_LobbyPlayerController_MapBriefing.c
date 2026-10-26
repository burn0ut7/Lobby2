//! The local Lobby2 controller owns the briefing drawer for the gameplay map.
modded class GC_LobbyPlayerController
{
	protected Widget m_wGC_MapBriefing;
	protected static const ResourceName GC_MAP_BRIEFING_LAYOUT = "{9D73A4B260E918C5}UI/layouts/Briefing/Lobby_Map_Briefing.layout";

	//------------------------------------------------------------------------------------------------
	protected void GC_OpenMapBriefing(MapConfiguration config)
	{
		if (m_wGC_MapBriefing || !config || config.MapEntityMode != EMapEntityMode.FULLSCREEN)
			return;
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;

		m_wGC_MapBriefing = GetGame().GetWorkspace().CreateWidgets(GC_MAP_BRIEFING_LAYOUT, config.RootWidgetRef);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsPointerOverMapBriefing(int mouseX, int mouseY)
	{
		if (!GC_LobbyGameMode.GetGameMode() || !m_wGC_MapBriefing)
			return false;

		GC_SlideOutComponent drawer = GC_SlideOutComponent.Cast(m_wGC_MapBriefing.FindHandler(GC_SlideOutComponent));
		return drawer && drawer.ContainsPointer(mouseX, mouseY);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_CloseMapBriefing()
	{
		if (m_wGC_MapBriefing)
			m_wGC_MapBriefing.RemoveFromHierarchy();
		m_wGC_MapBriefing = null;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_CloseMapBriefing();
		super.ShutdownLobbyPresentation();
	}
}
