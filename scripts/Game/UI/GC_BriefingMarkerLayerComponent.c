//! Reusable briefing-map marker presentation. Attach it to any briefing view
//! that shares a widget tree with an open SCR_MapEntity MapFrame.
class GC_BriefingMarkerLayerComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName MARKER_LAYOUT = "{6A4E000000000001}UI/layouts/Briefing/Lobby_Briefing_Marker.layout";
	protected static const float MIN_DISPLAY_SIZE = 4;
	protected static const float MAX_DISPLAY_SIZE = 512;
	protected static const float MIN_DESCRIPTION_WIDTH = 180;
	protected static const float DESCRIPTION_HEIGHT = 30;

	protected GC_LobbyManager m_LobbyManager;
	protected SCR_MapEntity m_MapEntity;
	protected ref array<Widget> m_aMarkerRoots = {};
	protected ref array<ImageWidget> m_aMarkerImages = {};
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
			TextWidget markerDescription = TextWidget.Cast(markerRoot.FindAnyWidget("BriefingMarkerDescription"));
			if (!markerImage || !markerDescription)
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

			markerImage.SetRotation(marker.GetRotation());
			markerImage.SetColorInt(marker.GetColor());
			markerDescription.SetText(marker.GetDescription());
			m_aMarkerRoots.Insert(markerRoot);
			m_aMarkerImages.Insert(markerImage);
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
			TextWidget markerDescription = m_aMarkerDescriptions[markerIndex];
			if (!marker || !markerRoot || !markerImage || !markerDescription)
				continue;

			float displaySize = marker.GetSize();
			if (marker.IsWorldScale())
				displaySize = GetProjectedWorldSize(marker, workspace);
			if (displaySize < MIN_DISPLAY_SIZE) displaySize = MIN_DISPLAY_SIZE;
			if (!marker.IsWorldScale() && displaySize > MAX_DISPLAY_SIZE) displaySize = MAX_DISPLAY_SIZE;

			float displayWidth = displaySize;
			if (displayWidth < MIN_DESCRIPTION_WIDTH) displayWidth = MIN_DESCRIPTION_WIDTH;

			vector worldPosition = marker.GetWorldPosition();
			int screenX;
			int screenY;
			m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2], screenX, screenY, true);
			float localX = workspace.DPIUnscale(screenX) - displayWidth * 0.5;
			float localY = workspace.DPIUnscale(screenY) - displaySize * 0.5;

			FrameSlot.SetPos(markerRoot, localX, localY);
			FrameSlot.SetSize(markerRoot, displayWidth, displaySize + DESCRIPTION_HEIGHT);
			FrameSlot.SetPos(markerImage, (displayWidth - displaySize) * 0.5, 0);
			FrameSlot.SetSize(markerImage, displaySize, displaySize);
			FrameSlot.SetPos(markerDescription, 0, displaySize);
			FrameSlot.SetSize(markerDescription, displayWidth, DESCRIPTION_HEIGHT);
		}
	}

	//! Converts the authored world-metre diameter through the same map projection
	//! used for marker positioning. This remains accurate at extreme zoom levels
	//! where multiplying by the public zoom value accumulates scale error.
	protected float GetProjectedWorldSize(GC_BriefingMarker marker, WorkspaceWidget workspace)
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
		m_MapEntity.WorldToScreen(worldPosition[0] - halfWorldSize, worldPosition[2], leftX, leftY, true);
		m_MapEntity.WorldToScreen(worldPosition[0] + halfWorldSize, worldPosition[2], rightX, rightY, true);
		m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2] - halfWorldSize, bottomX, bottomY, true);
		m_MapEntity.WorldToScreen(worldPosition[0], worldPosition[2] + halfWorldSize, topX, topY, true);

		float projectedWidth = workspace.DPIUnscale(rightX - leftX);
		if (projectedWidth < 0)
			projectedWidth = -projectedWidth;
		float projectedHeight = workspace.DPIUnscale(topY - bottomY);
		if (projectedHeight < 0)
			projectedHeight = -projectedHeight;

		return (projectedWidth + projectedHeight) * 0.5;
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
		m_aMarkerDescriptions.Clear();
		m_aMarkers.Clear();
	}
}
