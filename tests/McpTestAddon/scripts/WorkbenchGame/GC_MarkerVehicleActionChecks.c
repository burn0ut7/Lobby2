#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Check marker and vehicle actions", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_MarkerVehicleActionChecks : WorkbenchPlugin
{
	protected int m_iChecks;
	protected int m_iFailures;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_iChecks = 0;
		m_iFailures = 0;
		Resource vehicleResource = BaseContainerTools.CreateContainer("GC_VehicleAction");
		Resource markerResource = BaseContainerTools.CreateContainer("GC_MapMarkerVisibilityAction");
		if (!vehicleResource || !vehicleResource.IsValid() || !markerResource || !markerResource.IsValid())
		{
			Check(false, "create editor definitions");
			return;
		}
		GC_VehicleAction vehicle = GC_VehicleAction.Cast(BaseContainerTools.CreateInstanceFromContainer(vehicleResource.GetResource().ToBaseContainer()));
		GC_MapMarkerVisibilityAction definition = GC_MapMarkerVisibilityAction.Cast(BaseContainerTools.CreateInstanceFromContainer(markerResource.GetResource().ToBaseContainer()));
		Check(!vehicle.BuildAction().IsValid() && !vehicle.m_bChangeFuel && !vehicle.m_bChangeSupplies && !vehicle.m_bRepair && vehicle.m_fFuelRatio == 1, "vehicle starts with no requested changes");
		vehicle.m_aEntities = {"Truck"};
		vehicle.m_bChangeFuel = true;
		Check(vehicle.BuildAction().IsValid(), "fuel only supported");
		vehicle.m_fFuelRatio = -0.1;
		Check(!vehicle.BuildAction().IsValid(), "negative fuel rejected");
		vehicle.m_fFuelRatio = 1.1;
		Check(!vehicle.BuildAction().IsValid(), "fuel above full rejected");
		vehicle.m_bChangeFuel = false;
		vehicle.m_bChangeSupplies = true;
		vehicle.m_iSuppliesAmount = 0;
		Check(vehicle.BuildAction().IsValid(), "empty supplies supported and unused fuel ignored");
		vehicle.m_iSuppliesAmount = -1;
		Check(!vehicle.BuildAction().IsValid(), "negative supplies rejected");
		vehicle.m_bChangeSupplies = false;
		vehicle.m_bRepair = true;
		GC_RuleAction vehicleAction = vehicle.BuildAction();
		Check(vehicleAction.IsValid(), "repair only ignores unused numeric values");
		vehicle.m_aEntities[0] = "Changed";
		Check(vehicleAction.m_aFlagNames[0] == "Truck", "compiled vehicle targets are independent");
		vehicle.m_aEntities = {""};
		Check(!vehicle.BuildAction().IsValid(), "blank target rejected");
		definition.m_aMarkers = {"Objective"};
		Check(definition.BuildAction().IsValid(), "factionless visibility can change alone");
		GC_MarkerFactionVisibility row = new GC_MarkerFactionVisibility();
		row.m_sFaction = "US";
		row.m_bVisible = false;
		definition.m_aFactions = {row};
		GC_RuleAction markerAction = definition.BuildAction();
		Check(markerAction.IsValid(), "single faction change accepted");
		row.m_bVisible = true;
		Check(!markerAction.m_aMarkerFactions[0].m_bVisible, "compiled faction choices are independent");
		definition.m_aFactions.Insert(row);
		Check(!definition.BuildAction().IsValid(), "duplicate faction entries rejected");

		GC_BriefingMarker marker = new GC_BriefingMarker();
		marker.Set("Objective", "image.edds", "", "", "1 2 3", 45, false, 10, 0, "Text");
		Check(marker.IsVisibleToFactionKey("US") && marker.IsVisibleToFactionKey(""), "authored empty audience keeps legacy everybody behavior");
		marker.SetFactionVisibility("US", false);
		marker.SetFactionVisibility("", false);
		Check(!marker.IsVisibleToFactionKey("US") && marker.IsVisibleToFactionKey("USSR") && !marker.IsVisibleToFactionKey(""), "hide one faction preserves others and controls factionless separately");
		marker.SetFactionVisibility("US", true);
		Check(marker.IsVisibleToFactionKey("US") && !marker.IsVisibleToFactionKey(""), "restore faction without revealing to factionless");
		array<string> audience = {"US"};
		marker.Set("Objective", "image.edds", "", "", "1 2 3", 45, false, 10, 0, "Text", audience);
		marker.SetFactionVisibility("US", false);
		Check(!marker.IsVisibleToFactionKey("US") && !marker.IsVisibleToFactionKey("USSR") && !marker.IsVisibleToFactionKey(""), "hiding last allowed faction does not reveal to everybody");
		marker.SetFactionVisibility("", true);
		Check(marker.IsVisibleToFactionKey("") && !marker.IsVisibleToFactionKey("US"), "factionless only is supported");
		Check(!marker.IsVisibleToFaction(null, SCR_EGameModeState.BRIEFING) && marker.IsVisibleToFaction(null, SCR_EGameModeState.GAME), "phase requirement is preserved");
		GC_LobbyBriefingCatalog catalog = new GC_LobbyBriefingCatalog();
		GC_BriefingMarker stored;
		Check(catalog.UpsertMarker(marker, stored), "catalog accepts detached marker update");
		marker.SetFactionVisibility("US", true);
		Check(!stored.IsVisibleToFactionKey("US") && stored.IsVisibleToFactionKey("") && stored.GetDescription() == "Text", "catalog copy preserves explicit audience and marker data");
		GC_LobbyBriefingCatalog recipient = new GC_LobbyBriefingCatalog();
		Check(recipient.ApplyMarker(stored) && recipient.GetMarker("Objective").IsVisibleToFactionKey("") && !recipient.GetMarker("Objective").IsVisibleToFactionKey("US"), "received catalog update preserves audience");
		stored.SetVisible(false);
		Check(!stored.IsVisibleToFaction(null, SCR_EGameModeState.GAME), "global hidden state still applies");
		PrintFormat("GC_MARKER_VEHICLE_CHECK complete: %1 checks, %2 failures. No world changes.", m_iChecks, m_iFailures);
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string description)
	{
		m_iChecks++;
		if (passed)
			Print("GC_MARKER_VEHICLE_CHECK PASS " + description);
		else
		{
			m_iFailures++;
			Print("GC_MARKER_VEHICLE_CHECK FAIL " + description, LogLevel.ERROR);
		}
	}
}
#endif
