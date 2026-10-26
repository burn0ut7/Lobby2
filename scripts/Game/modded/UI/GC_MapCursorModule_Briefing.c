//! Map action listeners receive the wheel independently of widget event consumption.
[BaseContainerProps()]
modded class SCR_MapCursorModule
{
	//------------------------------------------------------------------------------------------------
	protected void GC_GetWheelPointer(out int mouseX, out int mouseY)
	{
		WidgetManager.GetMousePos(mouseX, mouseY);
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_BriefingOwnsWheel()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return false;

		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return false;

		int mouseX, mouseY;
		GC_GetWheelPointer(mouseX, mouseY);
		return controller.GC_IsPointerOverMapBriefing(mouseX, mouseY);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputZoomWheelUp(float value, EActionTrigger reason)
	{
		if (!GC_BriefingOwnsWheel())
			super.OnInputZoomWheelUp(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputZoomWheelDown(float value, EActionTrigger reason)
	{
		if (!GC_BriefingOwnsWheel())
			super.OnInputZoomWheelDown(value, reason);
	}
}
