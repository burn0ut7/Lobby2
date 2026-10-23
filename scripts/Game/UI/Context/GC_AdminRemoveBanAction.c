//! The server's native ban service owns whether this identity currently has a ban.
class GC_AdminRemoveBanAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Remove ban";
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
			&& local && SCR_Global.IsAdmin(local.GetPlayerId())
			&& context.GetModerationForm() && !context.GetModerationForm().IsBusy();
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (CanExecute(context))
			context.GetModerationForm().RemoveBan(context.GetPlayerIdentity());
	}
}
