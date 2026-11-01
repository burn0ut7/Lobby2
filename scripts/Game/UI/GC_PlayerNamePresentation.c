//! One disabled-by-default switch for transition-level UI event diagnostics.
class GC_UIEventDiagnostics
{
	static const bool ENABLED = false;
}

//! Canonical player-name styling for every Lobby2 presentation surface.
//! Administration is queried live from the base player manager and is never cached.
class GC_PlayerNamePresentation
{
	static void Apply(Widget nameWidget, int playerId)
	{
		if (!nameWidget)
			return;

		bool isAdmin = playerId > 0 && SCR_Global.IsAdmin(playerId);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		bool isNew = manager && manager.IsNewPlayer(playerId);
		nameWidget.SetColor(ResolveColor(isAdmin, isNew));
	}

	//------------------------------------------------------------------------------------------------
	//! Admin identity takes priority over the first three hours of recorded playtime.
	static Color ResolveColor(bool isAdmin, bool isNewPlayer = false)
	{
		if (isAdmin)
			return GC_LobbyPalette.AdminName();
		if (isNewPlayer)
			return GC_LobbyPalette.NewPlayerName();
		return GC_LobbyPalette.Text();
	}
}
