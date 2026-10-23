//! Player-row shortcuts into the same server-authoritative voice policy as Admin Tools.
class GC_AdminVoiceAction : GC_ContextMenuAction
{
	protected bool m_bPriorityControl;

	//------------------------------------------------------------------------------------------------
	void GC_AdminVoiceAction(bool priorityControl)
	{
		m_bPriorityControl = priorityControl;
	}

	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return GC_LobbyManager.GetManager() && context && context.GetTargetType() == GC_EContextTargetType.PLAYER && SCR_Global.IsAdmin(context.GetRequesterPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		return !context.GetPlayerIdentity().IsEmpty() && manager.GetPlayerVoiceIdentity(context.GetPlayerId()) == context.GetPlayerIdentity() && GetGame().GetPlayerManager().IsPlayerConnected(context.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !context)
			return string.Empty;
		GC_VoiceChannelAssignment assignment = manager.GetPlayerVoiceAssignment(context.GetPlayerId());
		if (!assignment)
			return string.Empty;
		if (m_bPriorityControl)
		{
			if (assignment.IsPrioritySpeaker())
				return "Remove priority speaker";
			return "Make priority speaker";
		}
		if (assignment.IsAdminMuted())
			return "Unmute lobby voice";
		return "Mute lobby voice";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_VoiceChannelAssignment assignment = GC_LobbyManager.GetManager().GetPlayerVoiceAssignment(context.GetPlayerId());
		if (!controller || !assignment)
			return;
		bool enabled = !assignment.IsAdminMuted();
		if (m_bPriorityControl)
			enabled = !assignment.IsPrioritySpeaker();
		controller.RequestAdminVoice(context.GetPlayerId(), context.GetPlayerIdentity(), m_bPriorityControl, enabled);
	}
}
