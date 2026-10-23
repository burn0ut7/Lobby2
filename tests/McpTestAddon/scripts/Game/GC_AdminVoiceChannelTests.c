//! Optional MCP fixture: production requests and read-only observations only.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminVoiceChannel(int targetId, string expectedIdentity, int channelType,
		string factionKey, int groupRplId, string personalOwnerIdentity)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE active local controller";
		controller.RequestAdminVoiceChannel(targetId, expectedIdentity, channelType, factionKey,
			groupRplId, personalOwnerIdentity);
		return "DISPATCHED production request; inspect state, response and audit";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceChannelSnapshot(int playerId)
	{
		if (GetManager() != this)
			return "UNAVAILABLE active manager";
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (!assignment)
			return "UNAVAILABLE assignment";
		GC_Slot slot = GetSlotByPlayerId(playerId);
		int slotId = -1;
		if (slot)
			slotId = slot.GetID();
		string factionKey;
		Faction faction = GetPlayerFaction(playerId);
		if (faction)
			factionKey = faction.GetFactionKey();
		RplId groupId = RplId.Invalid();
		SCR_AIGroup group = GetPlayerGroup(playerId);
		if (group)
			groupId = Replication.FindItemId(group);
		string result = string.Format("player=%1 identity=%2 channel=%3 channelFaction=%4 channelGroup=%5 owner=%6 slot=%7 faction=%8 group=%9",
			playerId, assignment.GetPlayerIdentity(), assignment.GetChannelType(), assignment.GetFactionKey(),
			assignment.GetGroupRplId(), assignment.GetPersonalOwnerIdentity(), slotId, factionKey, groupId);
		return result + string.Format(" muted=%1 priority=%2 adminAssigned=%3",
			assignment.IsAdminMuted(), assignment.IsPrioritySpeaker(), assignment.IsAdminAssigned());
	}
}
