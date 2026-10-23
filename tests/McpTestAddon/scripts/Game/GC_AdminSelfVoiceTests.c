//! Optional tests invoke production requests without granting administrator roles.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSelfVoiceTargets()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE local controller";

		int playerId = controller.GetPlayerId();
		string result = string.Format("player=%1 admin=%2 phase=%3 identity=%4 ", playerId,
			SCR_Global.IsAdmin(playerId), GetLobbyPhase(), GetPlayerVoiceIdentity(playerId));
		foreach (SCR_AIGroup group : GetRegisteredGroups())
		{
			if (group && group.GetFaction())
				result += string.Format("[group=%1 faction=%2 ordinaryJoin=%3] ", Replication.FindItemId(group),
					group.GetFaction().GetFactionKey(), CanPlayerJoinGroupVoiceChannel(playerId, Replication.FindItemId(group)));
		}
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		GC_Slot slot = GetSlotByPlayerId(playerId);
		int slotId = -1;
		if (slot)
			slotId = slot.GetID();
		RplId nativeGroupId = RplId.Invalid();
		SCR_AIGroup nativeGroup = GetPlayerGroup(playerId);
		if (nativeGroup)
			nativeGroupId = Replication.FindItemId(nativeGroup);
		result += string.Format("slot=%1 nativeGroup=%2 ", slotId, nativeGroupId);
		if (assignment)
			result += string.Format("channel=%1 faction=%2 group=%3 owner=%4 adminAssigned=%5", assignment.GetChannelType(),
				assignment.GetFactionKey(), assignment.GetGroupRplId(), assignment.GetPersonalOwnerIdentity(), assignment.IsAdminAssigned());
		BaseRadioComponent radio = GC_LobbyRadio.FindRadio(controller.GetControlledEntity());
		if (radio && radio.TransceiversCount() >= 3)
			result += string.Format(" radioChat=%1 radioFaction=%2", radio.GetTransceiver(GC_LobbyRadio.CHAT).GetFrequency(),
				radio.GetTransceiver(GC_LobbyRadio.FACTION).GetFrequency());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelfVoiceReject(int requesterId, int groupId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		SCR_AIGroup group = GetRegisteredGroupByRplId(groupId);
		if (GetManager() != this || !controller || !group || !group.GetFaction())
			return "UNAVAILABLE target";
		return ExecuteAdminVoiceChannel(requesterId, controller.GetPlayerId(), GetPlayerVoiceIdentity(controller.GetPlayerId()),
			GC_VoiceChannelType.GROUP, group.GetFaction().GetFactionKey(), groupId, string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSelfVoiceJoin(int channelType, string factionKey, int groupId, string ownerIdentity, bool contextMenu)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (GetManager() != this || !controller)
			return "UNAVAILABLE local controller";

		if (contextMenu)
		{
			GC_ContextMenuContext context = GC_ContextMenuContext.ForGroup(GetRegisteredGroupByRplId(groupId));
			GC_JoinGroupVoiceAction action = new GC_JoinGroupVoiceAction();
			if (!action.IsVisible(context) || !action.CanExecute(context))
				return "HIDDEN context action";
			action.Execute(context);
		}
		else
			controller.RequestVoiceChannel(channelType, factionKey, groupId, ownerIdentity);

		return "DISPATCHED production self-join; inspect resulting assignment and audit";
	}
}
#endif
