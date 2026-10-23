//! Player-owned voice selection; the server rechecks the shared channel policy.
class GC_JoinGroupVoiceAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Join group voice channel";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return CanExecute(context);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!context || context.GetTargetType() != GC_EContextTargetType.GROUP || !context.GetGroup())
			return false;

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!manager || !controller || controller.GetPlayerId() != context.GetRequesterPlayerId())
			return false;

		RplId groupId = Replication.FindItemId(context.GetGroup());
		int playerId = controller.GetPlayerId();
		bool canJoin = manager.CanPlayerJoinGroupVoiceChannel(playerId, groupId);
		if (SCR_Global.IsAdmin(playerId) && context.GetGroup().GetFaction())
			canJoin = manager.IsAdminVoiceDestinationValid(GC_VoiceChannelType.GROUP,
				context.GetGroup().GetFaction().GetFactionKey(), groupId, string.Empty);
		if (!canJoin)
			return false;

		GC_VoiceChannelAssignment assignment = manager.GetPlayerVoiceAssignment(playerId);
		return !assignment || assignment.GetChannelType() != GC_VoiceChannelType.GROUP
			|| assignment.GetGroupRplId() != groupId;
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;

		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		controller.RequestVoiceChannel(GC_VoiceChannelType.GROUP, string.Empty, Replication.FindItemId(context.GetGroup()));
	}
}
