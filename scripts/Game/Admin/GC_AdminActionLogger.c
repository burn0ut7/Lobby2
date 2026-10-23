//! Canonical audit entry point for Lobby2 admin actions.
//! Keep future audit destinations behind this function so callers do not
//! develop separate logging paths.
class GC_AdminActionLogger
{
	static void Log(int playerId, string action, string target, string outcome)
	{
		UUID actorIdentity;
		if (Replication.IsServer() && playerId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			actorIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		PrintFormat("GC | Admin Action | player=%1 identity=%2 action=%3 target=%4 outcome=%5", playerId, actorIdentity, Clean(action), Clean(target), Clean(outcome));
	}

	//------------------------------------------------------------------------------------------------
	protected static string Clean(string value)
	{
		value.Replace("\r", "\\r");
		value.Replace("\n", "\\n");
		value.Replace("\t", "\\t");
		return value;
	}
}
