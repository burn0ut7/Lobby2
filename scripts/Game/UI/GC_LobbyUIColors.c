//! Lobby2 presentation palette. Layouts use the matching normalized RGBA values.
//! Faction colors always come from the base-game faction; never modify those colors.
class GC_LobbyUIColors
{
	static const int ACCENT = 0xFFFFD040;
	static const int TEXT = 0xFFFFFFFF;
	static const int TEXT_MUTED = 0xFFD6D6D6;
	static const int PANEL = 0xB8000000;
	static const int PANEL_HOVER = 0xD6000000;
	static const int PANEL_STRONG = 0xEB000000;
	static const int ROW = 0x8C000000;

	//! Derive the dark surface from a copy, preserving the authoritative faction color.
	static Color FactionBackground(Faction faction)
	{
		Color background = Color.FromInt(faction.GetFactionColor().PackToInt());
		background.Lerp(Color.FromRGBA(0, 0, 0, 255), 0.82);
		background.SetA(0.92);
		return background;
	}
}
