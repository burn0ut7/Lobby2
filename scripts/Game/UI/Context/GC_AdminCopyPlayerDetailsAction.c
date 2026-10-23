//! Copy uses an identity-bound server snapshot, including disconnected session players.
class GC_AdminCopyPlayerDetailsAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Copy player details";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && context.GetTargetType() == GC_EContextTargetType.MODERATION_PLAYER && CanExecute(context);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		PlayerController local = GetGame().GetPlayerController();
		return context && !context.GetPlayerIdentity().IsEmpty() && GC_LobbyManager.GetManager()
			&& local && SCR_Global.IsAdmin(local.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller && CanExecute(context))
			controller.RequestCopyPlayerDetails(context.GetPlayerIdentity());
	}
}
