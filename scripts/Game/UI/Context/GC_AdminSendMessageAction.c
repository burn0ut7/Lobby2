//! Opens the composer; delivery still passes through the server's admin gate.
class GC_AdminSendMessageAction : GC_AdminOpenToolsAction
{
	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Send message";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return super.IsVisible(context) && (context.GetTargetType() == GC_EContextTargetType.PLAYER
			|| context.GetTargetType() == GC_EContextTargetType.SLOT);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!super.CanExecute(context))
			return false;
		if (context.GetSlot())
			return context.GetSlot().GetPlayerId() > 0;
		return context.GetPlayerId() > 0;
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		int playerId = context.GetPlayerId();
		if (context.GetSlot())
			playerId = context.GetSlot().GetPlayerId();
		GC_AdminToolsComponent.OpenMessage(playerId);
	}
}
