//! Optional policy checks; role inputs never grant a live player admin privileges.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestVoicePriorityImmunity()
	{
		GC_LobbyVoiceState state = new GC_LobbyVoiceState();
		GC_VoiceChannelAssignment first;
		GC_VoiceChannelAssignment second;
		GC_VoiceChannelAssignment admin;
		GC_VoiceChannelAssignment regular;
		state.SetChannel(1, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", first);
		state.SetChannel(2, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", second);
		state.SetChannel(3, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", admin);
		state.SetChannel(4, GC_VoiceChannelType.GLOBAL, "", RplId.Invalid(), "", regular);

		if (!state.CanTransmit(3, true) || !state.CanTransmit(4) || state.CanTransmit(99, true))
			return "FAIL admin eligibility";
		first.SetPrioritySpeaker(true);
		second.SetPrioritySpeaker(true);
		first.SetPriorityTransmitting(true);
		second.SetPriorityTransmitting(true);
		if (!state.CanTransmit(1) || !state.CanTransmit(2) || !state.CanTransmit(3, true) || state.CanTransmit(4))
			return "FAIL concurrent priority / admin immunity / regular suppression";
		if (state.CanTransmit(3, false))
			return "FAIL immunity retained after logout";
		admin.SetAdminMuted(true);
		if (state.CanTransmit(3, true))
			return "FAIL explicit mute precedence";
		admin.SetAdminMuted(false);

		first.SetPriorityTransmitting(false);
		if (state.CanTransmit(4))
			return "FAIL remaining priority";
		second.SetPriorityTransmitting(false);
		// Even a stale transmission flag cannot suppress without explicit designation.
		admin.SetPriorityTransmitting(true);
		if (!state.CanTransmit(3, true) || !state.CanTransmit(4) || admin.IsPrioritySpeaker())
			return "FAIL ordinary admin suppresses";
		admin.SetPrioritySpeaker(true);
		if (!state.CanTransmit(1) || !state.CanTransmit(2) || !state.CanTransmit(3, true) || state.CanTransmit(4))
			return "FAIL explicitly designated admin priority";
		admin.SetPrioritySpeaker(false);
		if (!state.CanTransmit(4))
			return "FAIL removed designation still suppresses";

		return "PASS priority concurrency, admin immunity, logout, explicit mute, designation and release";
	}
}
#endif
