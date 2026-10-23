//! Publish Lobby2's existing native group membership to the base controller APIs
//! used by text chat and other base-game consumers. No parallel membership is stored.
modded class SCR_PlayerControllerGroupComponent
{
	//------------------------------------------------------------------------------------------------
	void GC_SynchronizeLobbyMembership()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		PlayerController player = PlayerController.Cast(GetOwner());
		if (!Replication.IsServer() || !manager || !player)
			return;

		SCR_AIGroup group = manager.GetPlayerGroup(player.GetPlayerId());
		Faction faction;
		int groupId = -1;
		if (group)
		{
			faction = group.GetFaction();
			groupId = group.GetGroupID();
		}

		SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(player.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (affiliation && affiliation.GetAffiliatedFaction() != faction)
			affiliation.SetFaction_S(faction);

		if (m_iGroupID == groupId)
			return;

		m_iPreviousGroupID = m_iGroupID;
		m_iUISelectedGroupID = groupId;
		RPC_DoChangeGroupID(groupId);
		Rpc(RPC_DoChangeGroupID, groupId);
	}
}
