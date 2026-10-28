//! Canonical audit entry point for Lobby2 admin actions.
//! Keep future audit destinations behind this function so callers do not
//! develop separate logging paths.
class GC_AdminActionLogger
{
	//------------------------------------------------------------------------------------------------
	static void Log(int playerId, string action, string target, string outcome, int targetId = 0, string targetIdentity = "")
	{
		UUID actorIdentity;
		if (Replication.IsServer() && playerId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			actorIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		PrintFormat("GC | Admin Action | player=%1 identity=%2 action=%3 target=%4 outcome=%5", playerId, actorIdentity, Clean(action), Clean(target), Clean(outcome));
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (Replication.IsServer() && logs)
		{
			string type = "admin";
			if (action == "slot-self-select")
				type = "slot";
			// A recycled player ID must never attribute an earlier action to a new account.
			if (targetId > 0 && (!GetGame().GetPlayerManager().IsPlayerConnected(targetId)
				|| (!targetIdentity.IsEmpty() && SCR_PlayerIdentityUtils.GetPlayerIdentityId(targetId) != targetIdentity)))
				targetId = 0;
			GC_SessionEvent entry = logs.CreateEvent(type, playerId, targetId);
			if (!targetIdentity.IsEmpty())
				entry.m_sTarget = targetIdentity;
			if (!entry.m_sTarget.IsEmpty())
				entry.m_sTargetKind = "player";
			entry.m_sDetail = Clean(action) + " | " + Clean(target) + " | " + Clean(outcome);
			// Match the historical event contract even when another action supplies a long context.
			if (entry.m_sDetail.Length() > 4096)
				entry.m_sDetail = entry.m_sDetail.Substring(0, 4000) + " [truncated]";
			logs.Record(entry);
		}
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
