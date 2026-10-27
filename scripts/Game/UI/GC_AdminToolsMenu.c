//! Native menu lifetime, focus and input ownership for the full-screen tools.
class GC_AdminToolsMenu : ChimeraMenuBase
{
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.GC_RequestAdminViewOpen(false);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (IsFocused())
			GetGame().GetCallqueue().CallLater(Close, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GC_AdminToolsComponent panel = GC_AdminToolsComponent.Cast(GetRootWidget().FindHandler(GC_AdminToolsComponent));
		if (panel)
			panel.HidePlayerLists();
		GetGame().GetCallqueue().Remove(Close);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		super.OnMenuClose();
	}
}
