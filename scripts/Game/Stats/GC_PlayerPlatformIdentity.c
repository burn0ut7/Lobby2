//! Server-observed platform identity. Never accept a Steam ID supplied through a player RPC.
class GC_PlayerPlatformIdentity
{
	//------------------------------------------------------------------------------------------------
	static string SteamId(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return string.Empty;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !players.IsPlayerConnected(playerId) || players.GetPlatformKind(playerId) != PlatformKind.STEAM)
			return string.Empty;
		// The installed API is obsolete with no replacement platform-ID accessor.
		// A missing/malformed result stays unavailable; the website validates SteamID64 bits.
		string steamId = GetGame().GetBackendApi().GetPlayerPlatformId(playerId);
		if (steamId.Length() != 17)
			return string.Empty;
		for (int i; i < steamId.Length(); i++)
		{
			if (!"0123456789".Contains(steamId.Substring(i, 1)))
				return string.Empty;
		}
		return steamId;
	}
}
