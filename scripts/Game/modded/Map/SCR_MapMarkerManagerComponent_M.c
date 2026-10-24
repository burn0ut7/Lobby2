modded class SCR_MapMarkerManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{
		super.OnAddSynchedMarker(marker);
		GC_MapPlans plans = GC_MapPlans.GetInstance();
		if (!plans)
			return;
		// Native client filtering can remove opposing-faction markers on a listen
		// host. Retain them in the native disabled collection for server/JIP state.
		if (!m_aStaticMarkers.Contains(marker) && !m_aDisabledMarkers.Contains(marker))
		{
			marker.SetServerDisabled(true);
			m_aDisabledMarkers.Insert(marker);
		}
		plans.Changed();
	}

	//------------------------------------------------------------------------------------------------
	override void OnRemoveSynchedMarker(int markerID)
	{
		super.OnRemoveSynchedMarker(markerID);
		GC_MapPlans plans = GC_MapPlans.GetInstance();
		if (plans)
			plans.Changed();
	}

	//------------------------------------------------------------------------------------------------
	override void InsertStaticMarker(SCR_MapMarkerBase marker, bool isLocal, bool isServerMarker = false)
	{
		// Preserve mission-owned markers and replicated/JIP delivery. Only player
		// insertion (private or shared) is locked when the match starts.
		if ((isLocal || !isServerMarker) && GC_LobbyPhase.ArePlayerMapMarkersLocked())
			return;

		super.InsertStaticMarker(marker, isLocal, isServerMarker);
	}
}
