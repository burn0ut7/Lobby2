modded class SCR_MapMarkersUI
{
	protected static const int GC_LOBBY_MARKER_DIALOG_Z_ORDER = 10;

	//------------------------------------------------------------------------------------------------
	bool GC_IsMarkerPlacementBlocked()
	{
		if (GC_LobbyPhase.ArePlayerMapMarkersLocked())
			return true;

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
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.OnRadialMenuInit();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.OnInputQuickMarkerMenu(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void DelayedQuickMarkerMenu()
	{
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.DelayedQuickMarkerMenu();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInputMapSelect(float value, EActionTrigger reason)
	{
		// Editing a private marker inserts a new marker in the stock handler.
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.OnInputMapSelect(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMarkerEditDialog(bool isEditing = false, int tabID = 0, int selectedIconEntry = -1, int selectedColorEntry = -1)
	{
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.CreateMarkerEditDialog(isEditing, tabID, selectedIconEntry, selectedColorEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.CreateMilitaryMarkerEditDialog(isEditing, selectedFactionEntry, selectedDimensionEntry);
		GC_RaiseLobbyMarkerDialog();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPlaceMarkerConfirmed(SCR_InputButtonComponent button)
	{
		// An editor opened during briefing must not remove its original marker
		// or submit a replacement after the authoritative phase advances.
		if (GC_IsMarkerPlacementBlocked())
		{
			OnEditCancelled(button);
			return;
		}

		super.OnPlaceMarkerConfirmed(button);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnInsertMarker(bool isLocal)
	{
		if (GC_IsMarkerPlacementBlocked())
		{
			OnEditCancelled(null);
			return;
		}

		super.OnInsertMarker(isLocal);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnDragWidget(Widget widget)
	{
		if (GC_IsMarkerPlacementBlocked())
			return;

		super.OnDragWidget(widget);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnDragEnd(Widget widget, bool wasDragged)
	{
		if (GC_IsMarkerPlacementBlocked())
		{
			SCR_MapMarkerBase marker = m_MarkerMgr.GetMarkerByWidget(widget);
			if (marker)
				marker.SetDragged(false);
			return;
		}

		super.OnDragEnd(widget, wasDragged);
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
