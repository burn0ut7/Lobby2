//! Lobby2-only exception for authored playable groups that must remain available
//! while players change or release lobby slots.
modded class SCR_GroupsManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Complete native setup for authored groups before members can tune radios.
	void GC_RegisterLobbyGroup(notnull SCR_AIGroup group)
	{
		if (IsProxy() || !GC_LobbyManager.GetManager())
			return;

		AssignGroupID(group);
		group.GC_PublishLobbyGroupID();
		if (group.GetRadioFrequency() <= 0)
			AssignGroupFrequency(group);
		else
			ClaimFrequency(group.GetRadioFrequency(), group.GetFaction());
		RegisterGroup(group);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerFactionChanged(notnull FactionAffiliationComponent owner, Faction previousFaction, Faction newFaction)
	{
		// Lobby2 already selected the authored group through its slot workflow.
		// Native automatic group creation would introduce an unrelated extra squad.
		if (GC_LobbyManager.GetManager())
			return;

		super.OnPlayerFactionChanged(owner, previousFaction, newFaction);
	}

	//------------------------------------------------------------------------------------------------
	//! Release only a registration recorded as Lobby2-owned by its manager.
	//! Native UnregisterGroup can delete a slave and leaves agent listeners attached;
	//! returning a live GM group to AI must do neither of those things.
	void GC_ReleaseLobbyGroup(SCR_AIGroup group)
	{
		if (!group)
			return;

		for (int index = m_mPlayableGroups.Count() - 1; index >= 0; index--)
		{
			array<SCR_AIGroup> groups = m_mPlayableGroups.GetElement(index);
			if (!groups || !groups.Contains(group))
				continue;
			groups.RemoveItem(group);
			bool frequencyInUse;
			foreach (SCR_AIGroup other : groups)
			{
				if (other && other.GetRadioFrequency() == group.GetRadioFrequency())
					frequencyInUse = true;
			}
			if (!frequencyInUse)
				ReleaseFrequency(group.GetRadioFrequency(), m_mPlayableGroups.GetKey(index));
		}
		group.GetOnAgentAdded().Remove(OnGroupAgentAdded);
		group.GetOnAgentRemoved().Remove(OnGroupAgentRemoved);
	}

	//------------------------------------------------------------------------------------------------
	//! A GM group can change faction; repair the native index without shadow ownership.
	void GC_ReconcileLobbyGroupFaction(SCR_AIGroup group)
	{
		if (!group || !group.GetFaction())
			return;
		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(group.GetFaction());
		if (groups && groups.Contains(group))
			return;
		GC_ReleaseLobbyGroup(group);
		RegisterGroup(group);
		ClaimFrequency(group.GetRadioFrequency(), group.GetFaction());
	}

	//------------------------------------------------------------------------------------------------
	override void OnGroupPlayerRemoved(SCR_AIGroup group, int playerID)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (IsProxy() || !manager || !group || !manager.GetRegisteredGroups().Contains(group) || group.GetPlayerCount() > 0 || !group.GetDeleteIfNoPlayer())
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
