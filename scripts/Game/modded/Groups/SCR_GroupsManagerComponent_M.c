//! Lobby2-only exception for authored playable groups that must remain available
//! while players change or release lobby slots.
modded class SCR_GroupsManagerComponent
{
	override void OnGroupPlayerRemoved(SCR_AIGroup group, int playerID)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (IsProxy() || !manager || !group || !group.IsPlayable() || group.GetPlayerCount() > 0 || !group.GetDeleteIfNoPlayer())
		{
			super.OnGroupPlayerRemoved(group, playerID);
			return;
		}

		// Reuse the native empty-group preservation branch so privacy cleanup and
		// future base behavior remain intact, then restore the authored setting.
		group.SetCanDeleteIfNoPlayer(false);
		super.OnGroupPlayerRemoved(group, playerID);
		group.SetCanDeleteIfNoPlayer(true);
		PrintFormat("GC | Lifecycle | preserved empty playable group: group=%1 player=%2", group, playerID);
	}
}
