modded class SCR_MapMarkersUI
{
	protected static const int GC_LOBBY_MARKER_DIALOG_Z_ORDER = 10;

	override protected void CreateMarkerEditDialog(bool isEditing = false, int tabID = 0, int selectedIconEntry = -1, int selectedColorEntry = -1)
	{
		super.CreateMarkerEditDialog(isEditing, tabID, selectedIconEntry, selectedColorEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		super.CreateMilitaryMarkerEditDialog(isEditing, selectedFactionEntry, selectedDimensionEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	protected void GC_RaiseLobbyMarkerDialog()
	{
		if (!m_MarkerEditRoot || !m_RootWidget)
			return;

		// The stock marker editor is created below the Lobby2 side panels because
		// those panels use an explicit Z-order. Restrict the adjustment to the
		// Lobby2 briefing hierarchy so other game modes keep stock map behavior.
		if (!m_RootWidget.FindAnyWidget("BriefingView") || !m_RootWidget.FindAnyWidget("PlayerListPanel"))
			return;

		m_MarkerEditRoot.SetZOrder(GC_LOBBY_MARKER_DIALOG_Z_ORDER);
	}
}
