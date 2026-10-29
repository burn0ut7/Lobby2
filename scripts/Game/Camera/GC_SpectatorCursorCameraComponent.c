//! Supplies the same cursor-ownership flag used by the stock editor camera.
[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
class GC_SpectatorCursorCameraComponent : SCR_BaseManualCameraComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!GC_LobbyGameMode.GetGameMode() || !menu || !GetInputManager().IsUsingMouseAndKeyboard())
			return;

		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		if (menu.IsPointerOverScrollPanel(mouseX, mouseY))
			param.isCursorEnabled = false;
	}

	//------------------------------------------------------------------------------------------------
	override bool EOnCameraInit()
	{
		return true;
	}
}
