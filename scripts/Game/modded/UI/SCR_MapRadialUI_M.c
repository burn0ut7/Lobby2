modded class SCR_MapRadialUI
{
	//------------------------------------------------------------------------------------------------
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		// Reject the input before the controller opens an empty placement menu.
		SCR_MapMarkersUI markers = SCR_MapMarkersUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapMarkersUI));
		if (markers && markers.GC_IsMarkerPlacementBlocked())
			return;

		super.OnInputMenuOpen(value, reason);
	}
}
