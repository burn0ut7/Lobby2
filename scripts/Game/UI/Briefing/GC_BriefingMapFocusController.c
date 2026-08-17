//! Owns the asynchronous map-pan/zoom lifecycle for briefing marker links.
class GC_BriefingMapFocusController
{
	protected static const float PAN_DURATION = 0.4;
	protected static const float ZOOM_DURATION = 0.7;
	protected static const float TARGET_SCREEN_FRACTION = 0.34;
	protected static const float PANEL_MARGIN_FRACTION = 0.015;
	protected static const float MIN_AVAILABLE_WIDTH_FRACTION = 0.25;
	protected static const float DESCRIPTION_HEIGHT = 30;

	protected SCR_MapEntity m_MarkerFocusMap;
	protected vector m_vMarkerFocusWorld;
	protected float m_fMarkerFocusZoom;
	protected string m_sMarkerFocusName;

	void Focus(string markerName, GC_LobbyManager lobbyManager, SizeLayoutWidget viewSize)
	{
	    PlayerController playerController = GetGame().GetPlayerController();
	    if (!lobbyManager || !playerController)
	    {
	        PrintFormat("GC | Briefing marker focus failed: marker '%1' cannot be resolved because briefing player data is unavailable", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    GC_BriefingMarker focusMarker;
	    array<ref GC_BriefingMarker> visibleMarkers = lobbyManager.GetBriefingMarkersForPlayer(playerController.GetPlayerId());
	    foreach (GC_BriefingMarker marker : visibleMarkers)
	    {
	        if (marker && marker.GetName() == markerName)
	        {
	            focusMarker = marker;
	            break;
	        }
	    }

	    if (!focusMarker)
	    {
	        PrintFormat("GC | Briefing marker focus failed: marker '%1' was not found or is not visible to the current player", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
	    if (!mapEntity || !mapEntity.IsOpen())
	    {
	        PrintFormat("GC | Briefing marker focus failed: map is not open for marker '%1'", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    Widget mapRoot = mapEntity.GetMapMenuRoot();
	    Widget mapFrame;
	    if (mapRoot)
	        mapFrame = mapRoot.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
	    if (!mapFrame)
	    {
	        PrintFormat("GC | Briefing marker focus failed: map frame is unavailable for marker '%1'", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    float mapX;
	    float mapY;
	    float mapWidth;
	    float mapHeight;
	    mapFrame.GetScreenPos(mapX, mapY);
	    mapFrame.GetScreenSize(mapWidth, mapHeight);
	    if (mapWidth <= 0 || mapHeight <= 0)
	    {
	        PrintFormat("GC | Briefing marker focus failed: map viewport has no size for marker '%1'", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    float availableLeft = mapX;
	    float availableRight = mapX + mapWidth;
	    float availableTop = mapY;
	    float availableBottom = mapY + mapHeight;
	    float mapCenterX = mapX + mapWidth * 0.5;
	    float mapCenterY = mapY + mapHeight * 0.5;

	    if (viewSize)
	    {
	        float panelX;
	        float panelY;
	        float panelWidth;
	        float panelHeight;
	        viewSize.GetScreenPos(panelX, panelY);
	        viewSize.GetScreenSize(panelWidth, panelHeight);

	        bool overlapsVertically = panelY < mapY + mapHeight && panelY + panelHeight > mapY;
	        bool overlapsHorizontally = panelX < mapX + mapWidth && panelX + panelWidth > mapX;
	        if (overlapsVertically && overlapsHorizontally)
	        {
	            float margin = mapWidth * PANEL_MARGIN_FRACTION;
	            if (panelX + panelWidth * 0.5 <= mapCenterX)
	                availableLeft = panelX + panelWidth + margin;
	            else
	                availableRight = panelX - margin;

	            if (availableRight - availableLeft < mapWidth * MIN_AVAILABLE_WIDTH_FRACTION)
	            {
	                availableLeft = mapX;
	                availableRight = mapX + mapWidth;
	            }
	        }
	    }

	    float availableWidth = availableRight - availableLeft;
	    float availableShortSide = availableWidth;
	    if (mapHeight < availableShortSide)
	        availableShortSide = mapHeight;

	    float currentZoom = mapEntity.GetCurrentZoom();
	    if (currentZoom <= 0)
	    {
	        PrintFormat("GC | Briefing marker focus failed: map zoom is invalid for marker '%1'", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    float targetZoom = currentZoom;
	    if (focusMarker.GetSize() > 0)
	    {
	        float currentMarkerPixels = GetProjectedMarkerSize(mapEntity, focusMarker);
	        if (currentMarkerPixels <= 0)
	        {
	            PrintFormat("GC | Briefing marker focus failed: projected size is invalid for marker '%1'", markerName, level: LogLevel.WARNING);
	            return;
	        }

	        float angleRadians = focusMarker.GetRotation() * Math.DEG2RAD;
	        float rotationCos = Math.Cos(angleRadians);
	        if (rotationCos < 0)
	            rotationCos = -rotationCos;
	        float rotationSin = Math.Sin(angleRadians);
	        if (rotationSin < 0)
	            rotationSin = -rotationSin;

	        // A rotated square occupies |cos| + |sin| times its unrotated size.
	        float rotationFootprint = rotationCos + rotationSin;
	        if (rotationFootprint < 1)
	            rotationFootprint = 1;

	        float currentVisualPixels = currentMarkerPixels * rotationFootprint;
	        float targetVisualPixels = availableShortSide * TARGET_SCREEN_FRACTION;
	        targetZoom = currentZoom * targetVisualPixels / currentVisualPixels;

	        if (targetZoom < mapEntity.GetMinZoom())
	            targetZoom = mapEntity.GetMinZoom();
	        else if (targetZoom > mapEntity.GetMaxZoom())
	            targetZoom = mapEntity.GetMaxZoom();
	    }

	    float focusScreenX = (availableLeft + availableRight) * 0.5;
	    WorkspaceWidget workspace = GetGame().GetWorkspace();
	    float descriptionOffset;
	    if (workspace && !focusMarker.GetDescription().IsEmpty())
	        descriptionOffset = workspace.DPIScale(DESCRIPTION_HEIGHT * 0.5);
	    float focusScreenY = (availableTop + availableBottom) * 0.5 - descriptionOffset;
	    float horizontalScreenOffset = focusScreenX - mapCenterX;
	    float verticalScreenOffset = focusScreenY - mapCenterY;
	    vector markerPosition = focusMarker.GetWorldPosition();

	    float currentFocusWorldX = markerPosition[0] - horizontalScreenOffset / currentZoom;
	    float currentFocusWorldZ = markerPosition[2] + verticalScreenOffset / currentZoom;
	    int currentPanX;
	    int currentPanY;
	    mapEntity.WorldToScreen(currentFocusWorldX, currentFocusWorldZ, currentPanX, currentPanY, false);

	    Cancel();
	    m_MarkerFocusMap = mapEntity;
	    m_fMarkerFocusZoom = targetZoom;
	    m_vMarkerFocusWorld = {
	        markerPosition[0] - horizontalScreenOffset / targetZoom,
	        markerPosition[1],
	        markerPosition[2] + verticalScreenOffset / targetZoom
	    };
	    m_sMarkerFocusName = markerName;
	    SCR_MapEntity.GetOnMapPanEnd().Insert(OnMarkerFocusPanComplete);
	    mapEntity.PanSmooth(currentPanX, currentPanY, PAN_DURATION);
	}

	//! Measures the marker's authored size as a world footprint through the map
	//! projection. World-scale markers render at this footprint; fixed-pixel
	//! markers use it only to choose a useful contextual focus zoom.
	protected float GetProjectedMarkerSize(SCR_MapEntity mapEntity, GC_BriefingMarker marker)
	{
	    vector worldPosition = marker.GetWorldPosition();
	    float halfWorldSize = marker.GetSize() * 0.5;

	    int leftX;
	    int leftY;
	    int rightX;
	    int rightY;
	    int bottomX;
	    int bottomY;
	    int topX;
	    int topY;
	    mapEntity.WorldToScreen(worldPosition[0] - halfWorldSize, worldPosition[2], leftX, leftY, false);
	    mapEntity.WorldToScreen(worldPosition[0] + halfWorldSize, worldPosition[2], rightX, rightY, false);
	    mapEntity.WorldToScreen(worldPosition[0], worldPosition[2] - halfWorldSize, bottomX, bottomY, false);
	    mapEntity.WorldToScreen(worldPosition[0], worldPosition[2] + halfWorldSize, topX, topY, false);

	    float projectedWidth = rightX - leftX;
	    if (projectedWidth < 0)
	        projectedWidth = -projectedWidth;
	    float projectedHeight = topY - bottomY;
	    if (projectedHeight < 0)
	        projectedHeight = -projectedHeight;

	    return (projectedWidth + projectedHeight) * 0.5;
	}

	protected void OnMarkerFocusPanComplete(float panX, float panY)
	{
	    SCR_MapEntity mapEntity = m_MarkerFocusMap;
	    vector focusWorld = m_vMarkerFocusWorld;
	    float focusZoom = m_fMarkerFocusZoom;
	    string markerName = m_sMarkerFocusName;
	    Cancel();

	    if (!mapEntity || !mapEntity.IsOpen())
	    {
	        PrintFormat("GC | Briefing marker focus stopped after panning because the map closed: marker='%1'", markerName, level: LogLevel.WARNING);
	        return;
	    }

	    mapEntity.ZoomPanSmooth(focusZoom, focusWorld[0], focusWorld[2], ZOOM_DURATION);
	}

	void Cancel()
	{
	    SCR_MapEntity.GetOnMapPanEnd().Remove(OnMarkerFocusPanComplete);
	    m_MarkerFocusMap = null;
	    m_vMarkerFocusWorld = vector.Zero;
	    m_fMarkerFocusZoom = 0;
	    m_sMarkerFocusName = string.Empty;
	}

}

