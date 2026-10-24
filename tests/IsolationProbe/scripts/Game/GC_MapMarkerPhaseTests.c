//! Optional marker probes shared by addon-free and addon-loaded test runs.
#ifdef WORKBENCH
modded class SCR_MapMarkersUI
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMarkerEditor(string action)
	{
		if (action == "open")
			CreateMarkerEditDialog();
		else if (action == "military")
			CreateMilitaryMarkerEditDialog();
		else if (action == "cancel")
			OnEditCancelled(null);
		else if (action == "confirm" && m_MarkerEditRoot)
			OnPlaceMarkerConfirmed(null);
		else if (action != "snapshot")
			return "UNAVAILABLE action or editor";

		return string.Format("dialog=%1", m_MarkerEditRoot != null);
	}
}

modded class SCR_MapMarkerSyncComponent
{
	//------------------------------------------------------------------------------------------------
	void GC_TestMarkerServerRequest(SCR_MapMarkerBase marker)
	{
		if (Replication.IsServer())
			RPC_AskAddStaticMarker(marker);
	}
}

modded class SCR_MapMarkerManagerComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMarkerEditor(string action)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return "UNAVAILABLE open map";
		SCR_MapMarkersUI markerUI = SCR_MapMarkersUI.Cast(mapEntity.GetMapUIComponent(SCR_MapMarkersUI));
		if (!markerUI)
			return "UNAVAILABLE marker UI";
		return markerUI.GC_TestMarkerEditor(action);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMarkerPlacement(string route)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !Replication.IsServer())
			return "UNAVAILABLE embedded host required";

		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetWorldPos(1000, 1000);
		marker.SetCustomText("Marker phase test " + route);
		int before = GetStaticMarkers().Count() + GetDisabledMarkers().Count();
		if (route == "private")
			InsertStaticMarker(marker, true);
		else if (route == "shared")
			InsertStaticMarker(marker, false);
		else if (route == "server")
			InsertStaticMarker(marker, false, true);
		else if (route == "rpc")
		{
			SCR_MapMarkerSyncComponent sync = SCR_MapMarkerSyncComponent.Cast(controller.FindComponent(SCR_MapMarkerSyncComponent));
			if (!sync)
				return "UNAVAILABLE marker sync component";
			sync.GC_TestMarkerServerRequest(marker);
		}
		else
			return "REJECTED route";

		return string.Format("route=%1 before=%2 after=%3 owner=%4", route, before,
			GetStaticMarkers().Count() + GetDisabledMarkers().Count(), marker.GetMarkerOwnerID());
	}
}
#endif
