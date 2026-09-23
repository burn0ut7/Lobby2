//! Read-only mission marker interaction matching the native map-marker hover
//! pattern: raise the marker, reveal its label, and show an orange icon glow.
class GC_BriefingMarkerWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected Widget m_wLabel;
	protected ImageWidget m_wGlow;
	protected int m_iDefaultZOrder;
	protected string m_sMarkerName = "<unconfigured>";
	protected ResourceName m_sHighlightImage;
	protected string m_sHighlightQuad;
	protected bool m_bHighlightLoaded;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wLabel = w.FindAnyWidget("BriefingMarkerLabel");
		m_wGlow = ImageWidget.Cast(w.FindAnyWidget("BriefingMarkerGlow"));
		m_iDefaultZOrder = w.GetZOrder();
		if (m_wLabel)
			m_wLabel.SetVisible(true);
		SetHovered(false);
		PrintFormat("GC | Briefing Marker | event=handler-attached marker=%1 root=%2 label=%3 glow=%4 defaultZ=%5 flags=%6",
			m_sMarkerName, w, m_wLabel, m_wGlow, m_iDefaultZOrder, w.GetFlags());
	}

	void ConfigureDiagnostics(string markerName, ResourceName highlightImage, string highlightQuad, bool highlightLoaded)
	{
		m_sMarkerName = markerName;
		m_sHighlightImage = highlightImage;
		m_sHighlightQuad = highlightQuad;
		m_bHighlightLoaded = highlightLoaded;
		PrintFormat("GC | Briefing Marker | event=handler-configured marker=%1 highlightResource=%2 quad=%3 highlightLoaded=%4",
			m_sMarkerName, m_sHighlightImage, m_sHighlightQuad, m_bHighlightLoaded);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		SetHovered(true);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		SetHovered(false);
		return true;
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		return OnMouseEnter(w, x, y);
	}

	override bool OnFocusLost(Widget w, int x, int y)
	{
		return OnMouseLeave(w, null, x, y);
	}

	protected void SetHovered(bool hovered)
	{
		if (m_wGlow)
			m_wGlow.SetVisible(hovered);
		if (m_wRoot)
		{
			if (hovered)
				m_wRoot.SetZOrder(1000);
			else
				m_wRoot.SetZOrder(m_iDefaultZOrder);
		}

		PrintFormat("GC | Briefing Marker | event=hover-state marker=%1 hovered=%2 root=%3 glow=%4 glowVisible=%5 highlightLoaded=%6 highlightResource=%7 quad=%8 zOrder=%9",
			m_sMarkerName, hovered, m_wRoot, m_wGlow, m_wGlow && m_wGlow.IsVisible(), m_bHighlightLoaded,
			m_sHighlightImage, m_sHighlightQuad, m_wRoot.GetZOrder());
	}
}

//! Reusable briefing-map marker presentation. Attach it to any briefing view
//! that shares a widget tree with an open SCR_MapEntity MapFrame.
class GC_BriefingMarkerLayerComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName MARKER_LAYOUT = "{6A4E000000000001}UI/layouts/Briefing/Lobby_Briefing_Marker.layout";
	protected static const float MIN_LABEL_WIDTH = 180;
	protected static const float LABEL_HEIGHT = 30;
	protected static const int CURSOR_TRACE_INTERVAL_MS = 250;
	protected static const int CURSOR_TRACE_MAX_TICKS = 120;

	protected GC_LobbyManager m_LobbyManager;
	protected SCR_MapEntity m_MapEntity;
	protected ref array<Widget> m_aMarkerRoots = {};
	protected ref array<ImageWidget> m_aMarkerImages = {};
	protected ref array<ImageWidget> m_aMarkerGlows = {};
	protected ref array<Widget> m_aMarkerLabels = {};
	protected ref array<TextWidget> m_aMarkerDescriptions = {};
	protected ref array<ref GC_BriefingMarker> m_aMarkers = {};
	protected bool m_bTransformsLogged;
	protected int m_iCursorTraceTicks;
	protected Widget m_wLastCursorWidget;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnBriefingMarkersChanged.Insert(OnMarkersChanged);
			m_LobbyManager.m_OnRegistryReset.Insert(OnRegistryReset);
			m_LobbyManager.m_OnPlayerAffiliationChanged.Insert(OnPlayerAffiliationChanged);
		}

		SCR_MapEntity.GetOnMapOpenComplete().Insert(OnMapOpenComplete);
		SCR_MapEntity.GetOnMapClose().Insert(OnMapClose);
		SCR_MapEntity.GetOnMapPan().Insert(OnMapPan);
		SCR_MapEntity.GetOnMapZoom().Insert(OnMapZoom);
		RefreshMarkers();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnBriefingMarkersChanged.Remove(OnMarkersChanged);
			m_LobbyManager.m_OnRegistryReset.Remove(OnRegistryReset);
			m_LobbyManager.m_OnPlayerAffiliationChanged.Remove(OnPlayerAffiliationChanged);
		}

		SCR_MapEntity.GetOnMapOpenComplete().Remove(OnMapOpenComplete);
		SCR_MapEntity.GetOnMapClose().Remove(OnMapClose);
		SCR_MapEntity.GetOnMapPan().Remove(OnMapPan);
		SCR_MapEntity.GetOnMapZoom().Remove(OnMapZoom);
		GetGame().GetCallqueue().Remove(LogTransformReadbacks);
		GetGame().GetCallqueue().Remove(TraceCursorTarget);
		ClearMarkers();
		m_LobbyManager = null;
		m_MapEntity = null;
		super.HandlerDeattached(w);
	}

	protected void OnMarkersChanged(int revision) { RefreshMarkers(); }
	protected void OnRegistryReset(int revision) { RefreshMarkers(); }
	protected void OnPlayerAffiliationChanged(int playerId)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController && playerController.GetPlayerId() == playerId)
			RefreshMarkers();
	}
	protected void OnMapOpenComplete(MapConfiguration config) { RefreshMarkers(); }
	protected void OnMapClose(MapConfiguration config) { ClearMarkers(); }
	protected void OnMapPan(float x, float y, bool isPanStart) { UpdateMarkerTransforms(); }
	protected void OnMapZoom(float zoom) { UpdateMarkerTransforms(); }

	protected void RefreshMarkers()
	{
		ClearMarkers();
		if (!m_LobbyManager)
			m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_LobbyManager)
		{
			Print("GC | Briefing Marker | event=refresh result=rejected reason=lobby-manager");
			return;
		}

		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity || !m_MapEntity.IsOpen())
		{
			PrintFormat("GC | Briefing Marker | event=refresh result=deferred reason=map-closed map=%1", m_MapEntity);
			return;
		}

		Widget mapRoot = m_MapEntity.GetMapMenuRoot();
		Widget mapFrame;
		if (mapRoot)
			mapFrame = mapRoot.FindAnyWidget("MapFrame");
		if (!mapFrame)
		{
			PrintFormat("GC | Briefing Marker | event=refresh result=rejected reason=map-frame mapRoot=%1", mapRoot);
			return;
		}

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
		{
			Print("GC | Briefing Marker | event=refresh result=rejected reason=player-controller");
			return;
		}

		array<ref GC_BriefingMarker> visibleMarkers = m_LobbyManager.GetBriefingMarkersForPlayer(playerController.GetPlayerId());
		PrintFormat("GC | Briefing Marker | event=refresh result=building player=%1 visibleMarkers=%2 mapRoot=%3 mapFrame=%4",
			playerController.GetPlayerId(), visibleMarkers.Count(), mapRoot, mapFrame);
		foreach (GC_BriefingMarker marker : visibleMarkers)
		{
			Widget markerRoot = GetGame().GetWorkspace().CreateWidgets(MARKER_LAYOUT, mapFrame);
			if (!markerRoot)
			{
				PrintFormat("GC | Briefing Marker | event=marker-create result=rejected marker=%1 reason=layout", marker.GetName(), level: LogLevel.WARNING);
				continue;
			}

			ImageWidget markerImage = ImageWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerImage"));
			ImageWidget markerGlow = ImageWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerGlow"));
			Widget markerLabel = markerRoot.FindAnyWidget("BriefingMarkerLabel");
			TextWidget markerDescription = TextWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerDescription"));
			if (!markerImage || !markerGlow || !markerLabel || !markerDescription)
			{
				PrintFormat("GC | Briefing Marker | event=marker-create result=rejected marker=%1 reason=widgets root=%2 image=%3 glow=%4 label=%5 description=%6",
					marker.GetName(), markerRoot, markerImage, markerGlow, markerLabel, markerDescription, level: LogLevel.WARNING);
				markerRoot.RemoveFromHierarchy();
				continue;
			}

			bool loaded;
			if (marker.GetImage().EndsWith(".imageset"))
				loaded = markerImage.LoadImageFromSet(0, marker.GetImage(), marker.GetQuadName());
			else
				loaded = markerImage.LoadImageTexture(0, marker.GetImage());

			if (!loaded)
			{
				PrintFormat("GC | Failed to load briefing marker image: marker=%1 resource=%2 quad=%3",
					marker.GetName(), marker.GetImage(), marker.GetQuadName(), level: LogLevel.WARNING);
				markerRoot.RemoveFromHierarchy();
				continue;
			}

			ResourceName highlightImage = marker.GetHighlightImage();
			bool highlightLoaded;
			if (!highlightImage.IsEmpty())
				highlightLoaded = markerGlow.LoadImageFromSet(0, highlightImage, marker.GetQuadName());
			if (!highlightImage.IsEmpty() && !highlightLoaded)
			{
				PrintFormat("GC | Failed to load briefing marker highlight: marker=%1 resource=%2 quad=%3",
					marker.GetName(), highlightImage, marker.GetQuadName(), level: LogLevel.WARNING);
			}

			GC_BriefingMarkerWidgetComponent markerHandler = GC_BriefingMarkerWidgetComponent.Cast(markerRoot.FindHandler(GC_BriefingMarkerWidgetComponent));
			if (markerHandler)
				markerHandler.ConfigureDiagnostics(marker.GetName(), highlightImage, marker.GetQuadName(), highlightLoaded);
			else
				PrintFormat("GC | Briefing Marker | event=marker-create result=warning marker=%1 reason=handler", marker.GetName(), level: LogLevel.WARNING);

			markerImage.SetRotation(marker.GetRotation());
			markerImage.SetColorInt(marker.GetColor());
			markerGlow.SetRotation(marker.GetRotation());
			markerGlow.SetColor(Color.FromSRGBA(226, 168, 79, 155));
			markerDescription.SetText(marker.GetDescription());
			m_aMarkerRoots.Insert(markerRoot);
			m_aMarkerImages.Insert(markerImage);
			m_aMarkerGlows.Insert(markerGlow);
			m_aMarkerLabels.Insert(markerLabel);
			m_aMarkerDescriptions.Insert(markerDescription);
			m_aMarkers.Insert(marker);
			PrintFormat("GC | Briefing Marker | event=marker-create result=success marker=%1 image=%2 highlight=%3 quad=%4 baseLoaded=%5 highlightLoaded=%6 root=%7 handler=%8 description=%9",
				marker.GetName(), marker.GetImage(), highlightImage, marker.GetQuadName(), loaded, highlightLoaded, markerRoot, markerHandler, marker.GetDescription());
		}

		UpdateMarkerTransforms();
		m_iCursorTraceTicks = 0;
		m_wLastCursorWidget = null;
		GetGame().GetCallqueue().CallLater(TraceCursorTarget, CURSOR_TRACE_INTERVAL_MS, true);
	}

	protected void UpdateMarkerTransforms()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		bool shouldLogTransforms = !m_bTransformsLogged;
		for (int markerIndex; markerIndex < m_aMarkers.Count(); markerIndex++)
		{
			GC_BriefingMarker marker = m_aMarkers[markerIndex];
			Widget markerRoot = m_aMarkerRoots[markerIndex];
			ImageWidget markerImage = m_aMarkerImages[markerIndex];
			ImageWidget markerGlow = m_aMarkerGlows[markerIndex];
			Widget markerLabel = m_aMarkerLabels[markerIndex];
			TextWidget markerDescription = m_aMarkerDescriptions[markerIndex];
			if (!marker || !markerRoot || !markerImage || !markerGlow || !markerLabel || !markerDescription)
				continue;

			float displayWidth = marker.GetSize();
			float displayHeight = marker.GetSize();
			if (marker.IsWorldScale())
			{
				vector projectedSize = GetProjectedWorldSize(marker, workspace);
				displayWidth = projectedSize[0];
				displayHeight = projectedSize[1];
			}

			float minSize = marker.GetMinSize();
			if (minSize > 0)
			{
				if (displayWidth < minSize) displayWidth = minSize;
				if (displayHeight < minSize) displayHeight = minSize;
			}

			int imageWidth;
			int imageHeight;
			markerImage.GetImageSize(0, imageWidth, imageHeight);
			if (imageWidth > 0 && imageHeight > 0)
				displayHeight *= (float)imageHeight / (float)imageWidth;

			float labelWidth = displayWidth;
			if (labelWidth < MIN_LABEL_WIDTH) labelWidth = MIN_LABEL_WIDTH;
			float iconOffsetX = (labelWidth - displayWidth) * 0.5;

			vector worldPosition = marker.GetWorldPosition();
			int screenX;
			int screenY;
			m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2], screenX, screenY, true);
			float localX = workspace.DPIUnscale(screenX) - labelWidth * 0.5;
			float localY = workspace.DPIUnscale(screenY) - displayHeight * 0.5;

			FrameSlot.SetPos(markerRoot, localX, localY);
			FrameSlot.SetSize(markerRoot, labelWidth, displayHeight + LABEL_HEIGHT);
			FrameSlot.SetPos(markerGlow, iconOffsetX, 0);
			FrameSlot.SetSize(markerGlow, displayWidth, displayHeight);
			FrameSlot.SetPos(markerImage, iconOffsetX, 0);
			FrameSlot.SetSize(markerImage, displayWidth, displayHeight);
			FrameSlot.SetPos(markerLabel, 0, displayHeight);
			FrameSlot.SetSize(markerLabel, labelWidth, LABEL_HEIGHT);
			FrameSlot.SetPos(markerDescription, 0, 0);
			FrameSlot.SetSize(markerDescription, labelWidth, LABEL_HEIGHT);

			if (shouldLogTransforms)
			{
				float screenRootX;
				float screenRootY;
				float screenRootWidth;
				float screenRootHeight;
				markerRoot.GetScreenPos(screenRootX, screenRootY);
				markerRoot.GetScreenSize(screenRootWidth, screenRootHeight);
				PrintFormat("GC | Briefing Marker | event=transform marker=%1 world=%2 screenMap=%3,%4 local=%5,%6 display=%7,%8 worldScale=%9",
					marker.GetName(), worldPosition, screenX, screenY, localX, localY, displayWidth, displayHeight, marker.IsWorldScale());
				PrintFormat("GC | Briefing Marker | event=transform-readback marker=%1 screenRoot=%2,%3 screenSize=%4,%5 flags=%6",
					marker.GetName(), screenRootX, screenRootY, screenRootWidth, screenRootHeight, markerRoot.GetFlags());
			}
		}

		if (shouldLogTransforms)
		{
			m_bTransformsLogged = true;
			GetGame().GetCallqueue().CallLater(LogTransformReadbacks, 0);
		}
	}

	protected void LogTransformReadbacks()
	{
		for (int markerIndex; markerIndex < m_aMarkers.Count(); markerIndex++)
		{
			Widget markerRoot = m_aMarkerRoots[markerIndex];
			GC_BriefingMarker marker = m_aMarkers[markerIndex];
			if (!markerRoot || !marker)
				continue;

			float screenRootX;
			float screenRootY;
			float screenRootWidth;
			float screenRootHeight;
			markerRoot.GetScreenPos(screenRootX, screenRootY);
			markerRoot.GetScreenSize(screenRootWidth, screenRootHeight);
			PrintFormat("GC | Briefing Marker | event=settled-transform marker=%1 screenRoot=%2,%3 screenSize=%4,%5 flags=%6",
				marker.GetName(), screenRootX, screenRootY, screenRootWidth, screenRootHeight, markerRoot.GetFlags());
		}
	}

	protected void TraceCursorTarget()
	{
		m_iCursorTraceTicks++;
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		if (cursorWidget != m_wLastCursorWidget)
		{
			int mouseX;
			int mouseY;
			WidgetManager.GetMousePos(mouseX, mouseY);
			string cursorWidgetName = "<none>";
			if (cursorWidget)
				cursorWidgetName = cursorWidget.GetName();
			PrintFormat("GC | Briefing Marker | event=cursor-target tick=%1 mouse=%2,%3 widget=%4 widgetName=%5",
				m_iCursorTraceTicks, mouseX, mouseY, cursorWidget, cursorWidgetName);
			m_wLastCursorWidget = cursorWidget;
		}

		if (m_iCursorTraceTicks >= CURSOR_TRACE_MAX_TICKS)
		{
			GetGame().GetCallqueue().Remove(TraceCursorTarget);
			PrintFormat("GC | Briefing Marker | event=cursor-trace-complete ticks=%1", m_iCursorTraceTicks);
		}
	}

	//! Matches PS_ManualMarkerComponent.SetSlotWorld: project the authored
	//! positive X/Z offset and retain each screen axis independently.
	protected vector GetProjectedWorldSize(GC_BriefingMarker marker, WorkspaceWidget workspace)
	{
		vector worldPosition = marker.GetWorldPosition();
		int screenX;
		int screenY;
		int screenXEnd;
		int screenYEnd;
		m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2], screenX, screenY, true);
		m_MapEntity.WorldToScreen(worldPosition[0] + marker.GetSize(), worldPosition[2] + marker.GetSize(), screenXEnd, screenYEnd, true);

		return {workspace.DPIUnscale(screenXEnd - screenX), workspace.DPIUnscale(screenY - screenYEnd), 0};
	}

	protected void ClearMarkers()
	{
		GetGame().GetCallqueue().Remove(LogTransformReadbacks);
		GetGame().GetCallqueue().Remove(TraceCursorTarget);
		foreach (Widget markerRoot : m_aMarkerRoots)
		{
			if (markerRoot)
				markerRoot.RemoveFromHierarchy();
		}
		m_aMarkerRoots.Clear();
		m_aMarkerImages.Clear();
		m_aMarkerGlows.Clear();
		m_aMarkerLabels.Clear();
		m_aMarkerDescriptions.Clear();
		m_aMarkers.Clear();
		m_bTransformsLogged = false;
		m_iCursorTraceTicks = 0;
		m_wLastCursorWidget = null;
	}
}
