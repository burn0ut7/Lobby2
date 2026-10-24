//! One disabled-by-default switch for transition-level UI event diagnostics.
class GC_UIEventDiagnostics
{
	static const bool ENABLED = false;
}

//! Canonical player-name styling for every Lobby2 presentation surface.
//! Administration is queried live from the base player manager and is never cached.
class GC_PlayerNamePresentation
{
	//------------------------------------------------------------------------------------------------
	static void Apply(Widget nameWidget, int playerId, int normalColor = 0xFFFFFFFF)
	{
		if (!nameWidget)
			return;

		if (playerId > 0 && SCR_Global.IsAdmin(playerId))
			nameWidget.SetColor(Color.FromRGBA(255, 196, 64, 255));
		else
			nameWidget.SetColorInt(normalColor);
	}
}
