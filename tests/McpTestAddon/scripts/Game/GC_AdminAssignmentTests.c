#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAssignmentState(int playerId)
	{
		GC_Slot slot = GetSlotByPlayerId(playerId);
		SCR_AIGroup group = GetPlayerGroup(playerId);
		Faction faction = GetPlayerFaction(playerId);
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		int claims;
		foreach (GC_Slot candidate : GetSlots())
		{
			if (candidate.GetPlayerId() == playerId)
				claims++;
		}
		if (!slot)
			return string.Format("claims=%1 slot=none group=%2 faction=%3", claims, group != null, faction != null);
		SCR_AIGroup expected = ResolveSlotGroup(slot);
		GC_VoiceChannelAssignment voice = GetPlayerVoiceAssignment(playerId);
		bool groupMatches = group && group == expected && group.GetPlayerIDs().Contains(playerId);
		bool factionMatches = expected && faction == expected.GetFaction();
		bool controlled = controller && slot.GetEntity() && controller.GetControlledEntity() == slot.GetEntity();
		bool voiceMatches = voice && expected && voice.GetGroupRplId() == Replication.FindItemId(expected);
		return string.Format("claims=%1 slot=%2 groupMatches=%3 factionMatches=%4 controlled=%5 voiceMatches=%6", claims, slot.GetID(), groupMatches, factionMatches, controlled, voiceMatches);
	}

	//------------------------------------------------------------------------------------------------
	int GC_TestDifferentGroupSlot(int sourceId)
	{
		GC_Slot source = GetSlot(sourceId);
		if (!source)
			return -1;
		foreach (GC_Slot candidate : GetSlots())
		{
			if (!candidate.HasClaim() && !candidate.IsKIA() && ResolveSlotGroup(candidate) != ResolveSlotGroup(source))
				return candidate.GetID();
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAssignmentStaleReject(int playerId, int slotId)
	{
		PlayerController requester = GetGame().GetPlayerController();
		GC_Slot target = GetSlot(slotId);
		if (!requester || !target)
			return "unavailable-fixture";
		string before = GC_TestAssignmentState(playerId);
		string result = ExecuteAdminAssignSlot(requester.GetPlayerId(), playerId, "00000000-0000-0000-0000-000000000001", slotId, target.GetUUID());
		return string.Format("result=%1 unchanged=%2 state=%3", result, before == GC_TestAssignmentState(playerId), GC_TestAssignmentState(playerId));
	}
}
#endif
