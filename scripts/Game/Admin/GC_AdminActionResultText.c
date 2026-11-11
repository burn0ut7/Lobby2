//! Presentation wording; the server keeps precise outcome codes in its audit log.
class GC_AdminActionResultText
{
	//------------------------------------------------------------------------------------------------
	static string Get(string outcome)
	{
		switch (outcome)
		{
			case "rejected-slot-not-empty":
				return "Only empty slots can be locked.";
			case "rejected-playable-occupied":
				return "Release occupied or reserved slots first. Every slot in the selected group must have a character before Playable can be disabled.";
			case "unavailable-follow-camera":
				return "Use Follow while spectating, and select another player with a living character.";
			case "rejected-stale-character":
			case "rejected-incompatible-character":
				return "Select living, unoccupied soldiers in a group with a faction. Playable changes are unavailable after the match.";
			case "rejected-stale-rules":
				return "Another admin changed the faction rules. Review their current state.";
			case "unsupported-local-host-kick":
				return "The local host cannot be kicked. Use a remote player session for this action.";
			case "unsupported-ban-dedicated-only":
				return "Persistent bans require a dedicated server; unavailable in this session.";
			case "unavailable-ban-service":
				return "The server ban service is unavailable.";
			case "rejected-ban-service-busy":
				return "A ban-service request is still pending. Wait for its result.";
			case "uncertain-ban-timeout":
				return "Ban-service response timed out; the outcome is unknown. Check the server ban list before retrying.";
			case "dispatched-player-kick":
				return "Kick request sent. The player list updates when they disconnect.";
			case "rejected-admin-reason":
				return "Enter a single-line reason of 1-256 characters.";
			case "rejected-ban-expiry":
			case "rejected-ban-duration":
				return "Choose a future expiry or a duration up to 10 years.";
			case "rejected-account-identity":
				return "Enter a valid Arma UID. A Steam ID alone cannot be used to ban an unknown account.";
			case "rejected-stale-player":
				return "The player session changed. Select the player again.";
			case "rejected-spawn-position":
				return "The position is outside the world. Review all three coordinates.";
			case "rejected-slot-limit":
				return "The mission has reached the 512-slot limit.";
			case "rejected-stale-template":
				return "The template slot or group changed. Select it again.";
			case "applied":
				return "Action completed.";
			case "rejected-faction":
				return "The selected faction is unavailable.";
			case "rejected-rate-limit":
				return "Please wait a moment before sending again.";
			case "rejected-empty-audience":
				return "No connected players are in that audience. Choose another audience.";
			case "rejected-message":
				return "Enter 1-512 characters, without < or >.";
			case "rejected-stale-recipient":
				return "A recipient disconnected. Clear the list and select the players again.";
			case "rejected-not-logged-in":
				return "This action requires a current admin login.";
			case "assigned-character-entry-failed":
				return "Slot assigned, but character entry failed. Review the player's state.";
		}

		// Parameterized outcomes retain their specific-before-general precedence.
		if (outcome.StartsWith("failed-ai-") || outcome.Contains("changed="))
			return "The playable change could not finish. Check the group's current slots before making another change.";
		if (outcome.StartsWith("pending-ban-operation"))
			return "Request submitted. Waiting for the server ban service.";
		if (outcome.StartsWith("dispatched message="))
		{
			array<string> fields = {};
			outcome.Split(" ", fields, true);
			if (fields.Count() != 4)
				return "Message dispatch result is incomplete. Check the server log.";
			string recipients = fields[2];
			string skipped = fields[3];
			recipients.Replace("recipients=", "");
			skipped.Replace("skipped=", "");
			return string.Format("Message dispatched to %1 player(s). Skipped %2 unavailable player(s).", recipients, skipped);
		}
		if (outcome.StartsWith("dispatched"))
			return "Message sent to the requested audience.";
		if (outcome.StartsWith("failed-purge"))
			return "Purge could not finish. Some slots may already be cleared and locked. Review the remaining players and slots.";
		if (outcome.StartsWith("failed"))
			return "The action failed. Review the player and slot before retrying.";
		return "Action rejected. Review the target, slot and mission phase.";
	}
}
