[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class GC_LobbyHUDManagerComponentClass : SCR_HUDManagerComponentClass
{
}

//! Lobby2 HUD stays below the native menu stack, including menus opened before HUD initialization.
class GC_LobbyHUDManagerComponent : SCR_HUDManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (!GC_LobbyGameMode.GetGameMode())
			return;

		// Both native HUD roots otherwise tie with menus at zero, making their
		// creation order decide whether notifications cover the lobby.
		Widget hudRoot = GetHUDRootWidget();
		if (hudRoot)
			hudRoot.SetZOrder(-1);

		SCR_HUDManagerLayoutHandler layoutHandler = SCR_HUDManagerLayoutHandler.Cast(FindHandler(SCR_HUDManagerLayoutHandler));
		if (!layoutHandler)
			return;

		SCR_HUDLayout mainLayout = layoutHandler.GetActiveLayout();
		if (mainLayout && mainLayout.GetRootWidget())
			mainLayout.GetRootWidget().SetZOrder(-2);
	}
}
