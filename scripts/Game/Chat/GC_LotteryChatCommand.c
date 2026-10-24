//! Use native slash-command parsing. Unrelated modes retain their command handler.
modded class SCR_ChatPanelManager
{
	//------------------------------------------------------------------------------------------------
	override void Internal_OnChatCommand(SCR_ChatPanel panel, string command, string otherData)
	{
		if (command != "join" || !GC_LobbyManager.GetManager())
		{
			super.Internal_OnChatCommand(panel, command, otherData);
			return;
		}
		otherData.TrimInPlace();
		if (!otherData.IsEmpty())
		{
			SCR_HintManagerComponent.ShowCustomHint("Type /join to enter the newest open lottery.", "Slot lottery", 8);
			return;
		}
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.RequestEnterSlotLottery();
	}
}
