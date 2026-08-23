//! Canonical audit entry point for Lobby2 admin actions.
//! Keep future audit destinations behind this function so callers do not
//! develop separate logging paths.
class GC_AdminActionLogger
{
	static void Log(int playerId, string action, string target, string outcome)
	{
		PrintFormat("GC | Admin Action | player=%1 action=%2 target=%3 outcome=%4", playerId, action, target, outcome);
	}
}
