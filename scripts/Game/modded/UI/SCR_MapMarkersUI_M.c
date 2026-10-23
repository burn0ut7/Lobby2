modded class SCR_MapMarkersUI
{
	protected static const int GC_LOBBY_MARKER_DIALOG_Z_ORDER = 10;

	//------------------------------------------------------------------------------------------------
	protected bool GC_IsPreviewMap()
	{
		if (!GC_LobbyGameMode.GetGameMode() || !m_RootWidget)
			return false;

		MenuBase previewMenu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
		return previewMenu && previewMenu.GetRootWidget() == m_RootWidget;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnRadialMenuInit()
	{
		// Omit player-placement actions, including recon markers, but keep the
		// marker component active so mission and replicated markers still render.
		if (GC_IsPreviewMap())
			return;

		super.OnRadialMenuInit();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		if (GC_IsPreviewMap())
			return;

		super.OnInputQuickMarkerMenu(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void DelayedQuickMarkerMenu()
	{
		if (GC_IsPreviewMap())
			return;

		super.DelayedQuickMarkerMenu();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputMapSelect(float value, EActionTrigger reason)
	{
		// Editing a private marker inserts a new marker in the stock handler.
		if (GC_IsPreviewMap())
			return;

		super.OnInputMapSelect(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMarkerEditDialog(bool isEditing = false, int tabID = 0, int selectedIconEntry = -1, int selectedColorEntry = -1)
	{
		if (GC_IsPreviewMap())
			return;

		super.CreateMarkerEditDialog(isEditing, tabID, selectedIconEntry, selectedColorEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		if (GC_IsPreviewMap())
			return;

		super.CreateMilitaryMarkerEditDialog(isEditing, selectedFactionEntry, selectedDimensionEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RaiseLobbyMarkerDialog()
	{
		if (!GC_LobbyGameMode.GetGameMode() || !m_MarkerEditRoot || !m_RootWidget)
			return;

		// The stock marker editor is created below the Lobby2 side panels because
		// those panels use an explicit Z-order. Restrict the adjustment to the
		// Lobby2 briefing hierarchy so other game modes keep stock map behavior.
		if (!m_RootWidget.FindAnyWidget("BriefingView") || !m_RootWidget.FindAnyWidget("PlayerListPanel"))
			return;

		m_MarkerEditRoot.SetZOrder(GC_LOBBY_MARKER_DIALOG_Z_ORDER);
	}
}
