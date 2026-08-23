//! Read-only mission marker interaction matching the native map-marker hover
//! pattern: raise the marker, reveal its label, and show an orange icon glow.
class GC_BriefingMarkerWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected Widget m_wLabel;
	protected ImageWidget m_wGlow;
	protected int m_iDefaultZOrder;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wLabel = w.FindAnyWidget("BriefingMarkerLabel");
		m_wGlow = ImageWidget.Cast(w.FindAnyWidget("BriefingMarkerGlow"));
		m_iDefaultZOrder = w.GetZOrder();
		if (m_wLabel)
			m_wLabel.SetVisible(true);
		SetHovered(false);
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
	}
}

//! Reusable briefing-map marker presentation. Attach it to any briefing view
//! that shares a widget tree with an open SCR_MapEntity MapFrame.
class GC_BriefingMarkerLayerComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName MARKER_LAYOUT = "{6A4E000000000001}UI/layouts/Briefing/Lobby_Briefing_Marker.layout";
	protected static const float MIN_LABEL_WIDTH = 180;
	protected static const float LABEL_HEIGHT = 30;

	protected GC_LobbyManager m_LobbyManager;
	protected SCR_MapEntity m_MapEntity;
	protected ref array<Widget> m_aMarkerRoots = {};
	protected ref array<ImageWidget> m_aMarkerImages = {};
	protected ref array<ImageWidget> m_aMarkerGlows = {};
	protected ref array<Widget> m_aMarkerLabels = {};
	protected ref array<TextWidget> m_aMarkerDescriptions = {};
	protected ref array<ref GC_BriefingMarker> m_aMarkers = {};

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
			return;

		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		Widget mapRoot = m_MapEntity.GetMapMenuRoot();
		Widget mapFrame;
		if (mapRoot)
			mapFrame = mapRoot.FindAnyWidget("MapFrame");
		if (!mapFrame)
			return;

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		array<ref GC_BriefingMarker> visibleMarkers = m_LobbyManager.GetBriefingMarkersForPlayer(playerController.GetPlayerId());
		foreach (GC_BriefingMarker marker : visibleMarkers)
		{
			Widget markerRoot = GetGame().GetWorkspace().CreateWidgets(MARKER_LAYOUT, mapFrame);
			if (!markerRoot)
				continue;

			ImageWidget markerImage = ImageWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerImage"));
			ImageWidget markerGlow = ImageWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerGlow"));
			Widget markerLabel = markerRoot.FindAnyWidget("BriefingMarkerLabel");
			TextWidget markerDescription = TextWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerDescription"));
			if (!markerImage || !markerGlow || !markerLabel || !markerDescription)
			{
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
			if (!highlightImage.IsEmpty() && !markerGlow.LoadImageFromSet(0, highlightImage, marker.GetQuadName()))
			{
				PrintFormat("GC | Failed to load briefing marker highlight: marker=%1 resource=%2 quad=%3",
					marker.GetName(), highlightImage, marker.GetQuadName(), level: LogLevel.WARNING);
			}

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
		}

		UpdateMarkerTransforms();
	}

	protected void UpdateMarkerTransforms()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
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

			vector worldPosition = marker.GetWorldPosition();
			int screenX;
			int screenY;
			m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2], screenX, screenY, true);
			float localX = workspace.DPIUnscale(screenX) - displayWidth * 0.5;
			float localY = workspace.DPIUnscale(screenY) - displayHeight * 0.5;

			FrameSlot.SetPos(markerRoot, localX, localY);
			FrameSlot.SetSize(markerRoot, displayWidth, displayHeight);
			FrameSlot.SetPos(markerGlow, 0, 0);
			FrameSlot.SetSize(markerGlow, displayWidth, displayHeight);
			FrameSlot.SetPos(markerImage, 0, 0);
			FrameSlot.SetSize(markerImage, displayWidth, displayHeight);
			FrameSlot.SetPos(markerLabel, (displayWidth - labelWidth) * 0.5, displayHeight);
			FrameSlot.SetSize(markerLabel, labelWidth, LABEL_HEIGHT);
			FrameSlot.SetPos(markerDescription, 0, 0);
			FrameSlot.SetSize(markerDescription, labelWidth, LABEL_HEIGHT);
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
	}
}
