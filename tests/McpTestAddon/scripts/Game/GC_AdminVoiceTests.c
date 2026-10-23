//! Optional tests: production policy and authority handlers, never role grants.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceAdminTarget(int targetId, bool priorityControl, bool enabled)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(targetId);
		if (!controller || !assignment)
			return "UNAVAILABLE controller/target";
		controller.RequestAdminVoice(targetId, assignment.GetPlayerIdentity(), priorityControl, enabled);
		return "DISPATCHED inspect authority, peer and central audit";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceServerSnapshot(int targetId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(targetId));
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(targetId);
		if (!Replication.IsServer() || !controller || !assignment)
			return "UNAVAILABLE authority/target";
		return string.Format("player=%1 authorized=%2 priority=%3 muted=%4 transmitting=%5 eligible=%6", targetId,
			controller.HasAuthorizedLobbyCapture(), assignment.IsPrioritySpeaker(), assignment.IsAdminMuted(),
			assignment.IsPriorityTransmitting(), CanTransmitLobbyVoice(targetId));
	}

	//------------------------------------------------------------------------------------------------
	//! Simulates native speech notifications after production capture authorization.
	//! Does not open a microphone or establish audible transmission.
	string GC_TestObserveAuthorizedVoice(int firstId, int secondId)
	{
		if (!Replication.IsServer())
			return "UNAVAILABLE authority";
		ObserveLobbyVoiceTransmission(firstId);
		ObserveLobbyVoiceTransmission(secondId);
		return GC_TestVoiceServerSnapshot(firstId) + " | " + GC_TestVoiceServerSnapshot(secondId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoicePolicy()
	{
		GC_LobbyVoiceState state = new GC_LobbyVoiceState();
		GC_VoiceChannelAssignment first;
		GC_VoiceChannelAssignment second;
		GC_VoiceChannelAssignment regular;
		state.SetChannel(1, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", first);
		state.SetChannel(2, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", second);
		state.SetChannel(3, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", regular);
		if (!state.CanTransmit(3) || state.CanTransmit(99))
			return "FAIL initial eligibility";
		first.SetPrioritySpeaker(true);
		second.SetPrioritySpeaker(true);
		first.SetPriorityTransmitting(true);
		if (!state.CanTransmit(1) || !state.CanTransmit(2) || state.CanTransmit(3))
			return "FAIL simultaneous priority / suppression";
		second.SetPriorityTransmitting(true);
		first.SetPriorityTransmitting(false);
		if (state.CanTransmit(3))
			return "FAIL remaining priority";
		second.SetAdminMuted(true);
		if (state.CanTransmit(2) || !state.CanTransmit(3))
			return "FAIL mute precedence";
		regular.SetAdminMuted(true);
		if (state.CanTransmit(3))
			return "FAIL regular mute";
		GC_VoiceChannelAssignment copy = new GC_VoiceChannelAssignment();
		copy.CopyFrom(second);
		if (!copy.IsAdminMuted() || !copy.IsPrioritySpeaker() || !copy.IsPriorityTransmitting())
			return "FAIL assignment copy";
		state.Clear();
		if (state.CanTransmit(1))
			return "FAIL cleanup";
		return "PASS eligibility, concurrent priority, suppression, release, mute precedence, copy, cleanup";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceAdminRequest(bool priorityControl, bool enabled)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE local controller";
		int playerId = controller.GetPlayerId();
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (!assignment)
			return "UNAVAILABLE assignment";
		controller.RequestAdminVoice(playerId, assignment.GetPlayerIdentity(), priorityControl, enabled);
		return "DISPATCHED inspect GC_TestVoiceSnapshot and correlated admin log";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceSnapshot()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return "UNAVAILABLE controller";
		int playerId = controller.GetPlayerId();
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(playerId);
		if (!assignment)
			return "UNAVAILABLE assignment";
		return string.Format("player=%1 admin=%2 priority=%3 muted=%4 transmitting=%5 eligible=%6", playerId, SCR_Global.IsAdmin(playerId), assignment.IsPrioritySpeaker(), assignment.IsAdminMuted(), assignment.IsPriorityTransmitting(), CanTransmitLobbyVoice(playerId));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceRejectUnknownRequester()
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerController())
			return "UNAVAILABLE authority";
		int targetId = GetGame().GetPlayerController().GetPlayerId();
		GC_VoiceChannelAssignment assignment = GetPlayerVoiceAssignment(targetId);
		if (!assignment)
			return "UNAVAILABLE assignment";
		bool before = assignment.IsAdminMuted();
		bool accepted = TrySetAdminVoice(-1, targetId, assignment.GetPlayerIdentity(), false, !before);
		if (accepted || assignment.IsAdminMuted() != before)
			return "FAIL unknown requester mutated voice";
		return "PASS unknown requester denied without mutation; inspect rejection log";
	}
}
#endif
