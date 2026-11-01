//! Shared lobby palette. Selection matches the admin tool layouts.
//! Faction identity and gameplay status colors remain owned by their data.
class GC_LobbyPalette
{
	static Color Surface(int alpha = 199)
	{
		return Color.FromRGBA(1, 1, 1, alpha);
	}

	static Color Blue(int alpha = 242)
	{
		return Color.FromRGBA(18, 26, 28, alpha);
	}

	static Color Text(int alpha = 255)
	{
		return Color.FromRGBA(255, 255, 255, alpha);
	}

	static Color Muted(int alpha = 255)
	{
		return Color.FromRGBA(179, 179, 179, alpha);
	}

	static Color Selection(int alpha = 115)
	{
		return Color.FromRGBA(230, 173, 69, alpha);
	}
}
