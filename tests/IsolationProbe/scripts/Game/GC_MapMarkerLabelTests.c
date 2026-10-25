//! Native widget/callback probes; fixtures run only when explicitly requested through MCP.
#ifdef WORKBENCH
class GC_LabelTestMarker : SCR_MapMarkerBase
{
	//------------------------------------------------------------------------------------------------
	//! Stock-mode MPTest has no map. Exercise its native layout and entry configuration directly.
	bool GC_TestCreateLabelFixture(SCR_MapMarkerEntryConfig entry)
	{
		m_ConfigEntry = entry;
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(entry.GetMarkerLayout());
		if (!m_wRoot)
			return false;

		FrameSlot.SetPos(m_wRoot, 600, 400);
		m_MarkerWidgetComp = SCR_MapMarkerWidgetComponent.Cast(m_wRoot.FindHandler(SCR_MapMarkerWidgetComponent));
		m_MarkerWidgetComp.SetMarkerObject(this);
		entry.InitClientSettings(this, m_MarkerWidgetComp);
		m_MarkerWidgetComp.SetAuthor("Marker label test owner");
		m_MarkerWidgetComp.SetAuthorVisible(true);
		OnMapLayerChanged(0);
		return true;
	}
}

modded class SCR_MapMarkerWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestLabelSnapshot()
	{
		float iconX, iconY, textX, textY, authorX, authorY, width, height;
		if (m_bIsSymbolMode)
			m_wSymbolRoot.GetScreenPos(iconX, iconY);
		else
			m_wMarkerIcon.GetScreenPos(iconX, iconY);
		m_wMarkerText.GetScreenPos(textX, textY);
		m_wMarkerAuthor.GetScreenPos(authorX, authorY);
		m_wRoot.GetScreenSize(width, height);
		return string.Format("textVisible=%1 text=%2 authorVisible=%3 authorOpacity=%4 platformVisible=%5 platformOpacity=%6",
			m_wMarkerText.IsVisible(), m_wMarkerText.GetText(), m_wMarkerAuthor.IsVisible(), m_wMarkerAuthor.GetOpacity(),
			m_wAuthorPlatformIcon.IsVisible(), m_wAuthorPlatformIcon.GetOpacity())
			+ string.Format(" iconXY=%1,%2 textXY=%3,%4 authorXY=%5,%6 rootSize=%7,%8",
			iconX, iconY, textX, textY, authorX, authorY, width, height)
			+ string.Format(" detailsVisible=%1 detailsOpacity=%2 layer=%3", m_wTypeIconRoot.IsVisible(), m_wTypeIconRoot.GetOpacity(), m_iLayerID);
	}
}

modded class SCR_MapMarkerManagerComponent
{
	protected ref SCR_MapMarkerBase m_GC_LabelTestMarker;
	protected bool m_bGC_LabelTestFixture;

	//------------------------------------------------------------------------------------------------
	string GC_TestMarkerLabels(string action, bool military)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (action == "fixture" || action == "place")
		{
			if (m_GC_LabelTestMarker)
				return "REJECTED existing test marker";
			if (action == "place" && (!mapEntity || !mapEntity.IsOpen()))
				return "UNAVAILABLE open map";

			m_bGC_LabelTestFixture = action == "fixture";
			GC_LabelTestMarker fixture;
			if (m_bGC_LabelTestFixture)
			{
				fixture = new GC_LabelTestMarker();
				m_GC_LabelTestMarker = fixture;
			}
			else
				m_GC_LabelTestMarker = new SCR_MapMarkerBase();
			SCR_EMapMarkerType markerType = SCR_EMapMarkerType.PLACED_CUSTOM;
			if (military)
				markerType = SCR_EMapMarkerType.PLACED_MILITARY;
			m_GC_LabelTestMarker.SetType(markerType);
			m_GC_LabelTestMarker.SetMarkerConfigID(0);
			m_GC_LabelTestMarker.SetCustomText("Rally at the northern crossroads");
			if (m_bGC_LabelTestFixture)
			{
				SCR_MapMarkerEntryConfig entry = GetMarkerConfig().GetMarkerEntryConfigByType(markerType);
				if (!entry || !fixture.GC_TestCreateLabelFixture(entry))
					return "UNAVAILABLE native marker fixture";
			}
			else
			{
				float worldX, worldY;
				mapEntity.GetMapCenterWorldPosition(worldX, worldY);
				m_GC_LabelTestMarker.SetWorldPos(worldX, worldY);
				InsertStaticMarker(m_GC_LabelTestMarker, true);
			}

			return "dispatched native marker creation; snapshot after layout";
		}

		if (!m_GC_LabelTestMarker)
			return "UNAVAILABLE test marker";
		SCR_MapMarkerWidgetComponent component = m_GC_LabelTestMarker.GetMarkerComponent();
		Widget root = m_GC_LabelTestMarker.GetRootWidget();
		if (!component || !root)
			return "UNAVAILABLE marker widget";

		if (action == "hover")
			component.OnMouseEnter(root, 0, 0);
		else if (action == "leave")
			component.OnMouseLeave(root, null, 0, 0);
		else if (action == "focus")
			component.OnFocus(root, 0, 0);
		else if (action == "blur")
			component.OnFocusLost(root, 0, 0);
		else if (action == "far" || action == "near")
		{
			int layer = 0;
			if (action == "far")
				layer = 3;
			component.SetLayerID(layer);
			m_GC_LabelTestMarker.LayerChangeLogic(layer);
		}
		else if (action == "zoom-in" || action == "zoom-out")
		{
			if (!mapEntity || !mapEntity.IsOpen())
				return "UNAVAILABLE open map";
			float worldX, worldY;
			mapEntity.GetMapCenterWorldPosition(worldX, worldY);
			float zoom = mapEntity.GetMaxZoom();
			if (action == "zoom-out")
				zoom = mapEntity.GetMinZoom();
			mapEntity.ZoomPanSmooth(zoom, worldX, worldY);
		}
		else if (action == "remove-fixture" && m_bGC_LabelTestFixture)
		{
			m_GC_LabelTestMarker.OnDelete();
			m_GC_LabelTestMarker = null;
			return "removed fixture";
		}
		else if (action == "remove" && !m_bGC_LabelTestFixture)
		{
			RemoveStaticMarker(m_GC_LabelTestMarker);
			m_GC_LabelTestMarker = null;
			return "dispatched native marker removal";
		}
		else if (action != "snapshot")
			return "REJECTED action";

		return component.GC_TestLabelSnapshot();
	}
}
#endif
