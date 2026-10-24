modded class SCR_MapMarkerManagerComponent
{
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
