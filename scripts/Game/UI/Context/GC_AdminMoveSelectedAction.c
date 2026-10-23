//! Destination context captures the selected identity when opened, never at execution.
class GC_AdminMoveSelectedAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "ADMIN";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		if (!context || !GC_LobbyManager.GetManager() || !SCR_Global.IsAdmin(context.GetRequesterPlayerId()))
			return false;
		GC_EContextTargetType type = context.GetTargetType();
		return context.GetSelectedPlayerId() > 0 && (type == GC_EContextTargetType.SLOT
			|| type == GC_EContextTargetType.VOICE_CHANNEL || type == GC_EContextTargetType.GROUP);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		return manager.GetAdminSelectedPlayer() == context.GetSelectedPlayerId()
			&& manager.GetPlayerVoiceIdentity(context.GetSelectedPlayerId()) == context.GetSelectedIdentity()
			&& !context.GetSelectedIdentity().IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(context.GetSelectedPlayerId());
		if (context.GetTargetType() == GC_EContextTargetType.SLOT)
			return "Assign " + name + " to this slot";
		return "Move " + name + " to this voice channel";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		if (context.GetTargetType() == GC_EContextTargetType.SLOT)
		{
			GC_Slot slot = context.GetSlot();
			if (slot && slot.GetUUID() == context.GetPlayerIdentity())
				controller.RequestAdminAssignSlot(context.GetSelectedPlayerId(), context.GetSelectedIdentity(), slot.GetID(), context.GetPlayerIdentity());
			return;
		}
		GC_VoiceChannelType type = context.GetVoiceChannelType();
		RplId groupId = RplId.Invalid();
		string factionKey = context.GetFactionKey();
		if (context.GetGroup())
		{
			type = GC_VoiceChannelType.GROUP;
			groupId = Replication.FindItemId(context.GetGroup());
			if (context.GetGroup().GetFaction())
				factionKey = context.GetGroup().GetFaction().GetFactionKey();
		}
		if (type == GC_VoiceChannelType.PERSONAL || type == GC_VoiceChannelType.GLOBAL)
			factionKey = string.Empty;
		controller.RequestAdminVoiceChannel(context.GetSelectedPlayerId(), context.GetSelectedIdentity(), type,
			factionKey, groupId, context.GetVoicePersonalOwnerIdentity());
	}
}
