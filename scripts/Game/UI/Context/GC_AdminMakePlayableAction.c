class GC_AdminMakePlayableAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Make playable...";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && GC_LobbyManager.GetManager() && context.GetTargetType() == GC_EContextTargetType.ENTITY && SCR_Global.IsAdmin(context.GetRequesterPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		return IsVisible(context) && GC_LobbyManager.GetManager().GetPlayableCharacterGroup(context.GetEntity()) != null;
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (CanExecute(context))
			GC_AdminToolsComponent.ReviewMakePlayable(context.GetEntity());
	}
}
