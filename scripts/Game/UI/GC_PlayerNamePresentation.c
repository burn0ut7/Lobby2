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
		ApplyGameMasterIcon(nameWidget, playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Optional lobby-row badge; native roles remain the only authority.
	static void ApplyGameMasterIcon(Widget nameWidget, int playerId)
	{
		if (!nameWidget || !nameWidget.GetParent())
			return;
		Widget icon = nameWidget.GetParent().FindAnyWidget("GameMasterIcon");
		if (!icon)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		bool isGameMaster = GC_LobbyManager.GetManager() && players && playerId > 0
			&& players.IsPlayerConnected(playerId) && players.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER);
		icon.SetVisible(isGameMaster);
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
