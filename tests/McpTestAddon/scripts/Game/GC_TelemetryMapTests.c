#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref array<SCR_MapMarkerBase> m_aGC_TelemetryMapMarkers = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryMap(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "telemetry-map-test", action))
			return "denied";
		SCR_MapMarkerManagerComponent markers = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markers)
			return "markers-unavailable";
		if (action == "add")
		{
			if (!m_aGC_TelemetryMapMarkers.IsEmpty())
				return "already-added";
			array<string> factionKeys = {"US", "USSR"};
			foreach (int index, string key : factionKeys)
			{
				SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
				marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
				marker.SetWorldPos(1190 + index * 70, 2780);
				marker.SetMarkerFactionFlags(1 << GetGame().GetFactionManager().GetFactionIndex(GetGame().GetFactionManager().GetFactionByKey(key)));
				marker.SetColorEntry(index + 1);
				marker.SetIconEntry(index + 1);
				marker.SetRotation(index * 90);
				marker.SetCustomText("Telemetry " + key);
				markers.InsertStaticMarker(marker, false, true);
				m_aGC_TelemetryMapMarkers.Insert(marker);
			}
			bool placed = SetBriefingMarker("TelemetryMission", "{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset", "", "objective", "1200 39 2760", 45, false, 48, 0, "Mission objective", null, SCR_EGameModeState.SLOTTING);
			GC_AdminActionLogger.Log(player.GetPlayerId(), "telemetry-map-test", action, "applied");
			return "mission=" + placed + " shared=" + m_aGC_TelemetryMapMarkers.Count();
		}
		if (action == "remove")
		{
			foreach (SCR_MapMarkerBase marker : m_aGC_TelemetryMapMarkers)
				markers.OnAskRemoveStaticMarker(marker.GetMarkerID());
			m_aGC_TelemetryMapMarkers.Clear();
			RemoveBriefingMarker("TelemetryMission");
			GC_AdminActionLogger.Log(player.GetPlayerId(), "telemetry-map-test", action, "applied");
			return "removed";
		}
		return "unknown-action";
	}
}
#endif
