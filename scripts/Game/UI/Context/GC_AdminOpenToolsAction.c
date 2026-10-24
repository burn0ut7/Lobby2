class GC_AdminOpenToolsAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Open Admin Tools";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(context.GetRequesterPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		int playerId = context.GetPlayerId();
		if (context.GetSlot())
		{
			GC_Slot slot = context.GetSlot();
			if (!slot || manager.GetSlot(slot.GetID()) != slot || slot.GetUUID() != context.GetPlayerIdentity())
				return false;
			playerId = slot.GetPlayerId();
			if (playerId <= 0)
				return context.GetPlayerIdentity().IsEmpty();
		}
		else if (context.GetTargetType() != GC_EContextTargetType.PLAYER)
			return true;

		return !context.GetPlayerIdentity().IsEmpty()
			&& GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			&& manager.GetPlayerVoiceIdentity(playerId) == context.GetPlayerIdentity();
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		int targetId = context.GetPlayerId();
		if (context.GetSlot())
			targetId = context.GetSlot().GetPlayerId();
		GC_AdminToolsComponent.Open(targetId, false);
	}
}
