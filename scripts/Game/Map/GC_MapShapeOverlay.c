//! Passive map overlay. Authority supplies geometry; only projection happens here.
class GC_MapShapeOverlay
{
	protected Widget m_Root;
	protected Widget m_Parent;
	protected ref array<Widget> m_aLabels = {};
	protected CanvasWidget m_Canvas;
	protected ref array<int> m_aCounts = {};
	protected ref array<float> m_aWidths = {};
	protected ref array<vector> m_aPoints = {};
	protected ref array<vector> m_aFrameBounds = {};
	protected float m_fCanvasWidth;
	protected float m_fCanvasHeight;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};
	protected ref array<int> m_aColors = {}, m_aFills = {};
	protected ref array<bool> m_aInverted = {};
	protected ref array<string> m_aNames = {};
	protected bool m_bProjectionDirty;
	protected bool m_bGeometryDirty;
	protected float m_fProjectedX;
	protected float m_fProjectedZ;
	protected float m_fProjectedZoom;
	protected int m_iGeneration;
	protected bool m_bListening;
	protected bool m_bFramed;
	protected bool m_bGameMapOpened;
	protected Widget m_ReadyMapParent;
	protected static const ResourceName LAYOUT = "{8F470B543A20D950}UI/layouts/Mission/Lobby_AOMap.layout";

	//------------------------------------------------------------------------------------------------
	void Update(GC_LobbyPlayerController controller)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!GC_LobbyGameMode.GetGameMode() || !mapEntity || !mapEntity.IsOpen() || !mapEntity.GetMapWidget())
		{
			Close();
			return;
		}
		// Native Map.layout owns the canvas's coordinate frame; menus may rename it.
		Widget parent = mapEntity.GetMapWidget().GetParent();
		if (!parent)
		{
			Close();
			return;
		}
		if (parent != m_Parent)
		{
			Close();
			m_iGeneration++;
			m_Parent = parent;
			m_Root = GetGame().GetWorkspace().CreateWidgets(LAYOUT, parent);
			if (m_Root)
				m_Canvas = CanvasWidget.Cast(m_Root.FindAnyWidget("AOMapCanvas"));
			if (!m_Canvas)
			{
				Close();
				return;
			}
			SCR_MapEntity.GetOnMapPan().Insert(OnMapPan);
			SCR_MapEntity.GetOnMapZoom().Insert(OnMapZoom);
			SCR_MapEntity.GetOnMapClose().Insert(OnMapClose);
			m_bListening = true;
			BuildPresentation();
		}

		// The open map needs sizing/projection only. Geometry arrives on authority changes.
		float width, height;
		m_Canvas.GetScreenSize(width, height);
		if (width > 0 && height > 0 && (m_bProjectionDirty || width != m_fCanvasWidth || height != m_fCanvasHeight))
		{
			m_bProjectionDirty = false;
			Project(mapEntity);
			FrameFirstMap(m_aFrameBounds);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Receive(int generation, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<string> names = null, array<vector> frameBounds = null)
	{
		if (counts.Count() != colors.Count() || counts.Count() != widths.Count() || counts.Count() != fills.Count() || counts.Count() != inverted.Count())
			return;
		int total;
		foreach (int count : counts)
		{
			if (count < 3)
				return;
			total += count;
		}
		if (total != points.Count())
			return;

		m_aCounts.Copy(counts);
		m_aColors.Copy(colors);
		m_aFills.Copy(fills);
		m_aInverted.Copy(inverted);
		m_aWidths.Copy(widths);
		m_aPoints.Copy(points);
		m_aNames.Clear();
		if (names)
			m_aNames.Copy(names);
		m_aFrameBounds.Clear();
		if (frameBounds)
			m_aFrameBounds.Copy(frameBounds);
		if (m_Canvas)
			BuildPresentation();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild widgets only when data or the map root changes, never on a heartbeat.
	protected void BuildPresentation()
	{
		array<int> counts = m_aCounts, colors = m_aColors, fills = m_aFills;
		array<bool> inverted = m_aInverted;
		array<string> names = m_aNames;

		while (m_aLabels.Count() > counts.Count())
		{
			Widget removed = m_aLabels[m_aLabels.Count() - 1];
			if (removed)
				removed.RemoveFromHierarchy();
			m_aLabels.Remove(m_aLabels.Count() - 1);
		}
		m_aLabels.Resize(counts.Count());
		foreach (int labelIndex, int labelCount : counts)
		{
			Widget label = m_aLabels[labelIndex];
			bool named = names && names.IsIndexValid(labelIndex) && !names[labelIndex].IsEmpty();
			if (!named)
			{
				if (label)
					label.RemoveFromHierarchy();
				m_aLabels[labelIndex] = null;
				continue;
			}
			if (!label)
			{
				label = GetGame().GetWorkspace().CreateWidgets("{6A5A00000000B004}UI/layouts/Admin/Lobby_BoundaryMapLabel.layout", m_Root);
				m_aLabels[labelIndex] = label;
			}
			if (label)
				TextWidget.Cast(label.FindAnyWidget("Text")).SetText(names[labelIndex]);
		}
		m_aCommands.Resize(counts.Count() * 2);
		foreach (int index, int count : counts)
		{
			LineDrawCommand line = LineDrawCommand.Cast(m_aCommands[index * 2 + 1]);
			if (!line)
			{
				line = new LineDrawCommand();
				line.m_bShouldEnclose = true;
				line.m_fOutlineWidth = 1;
				line.m_iOutlineColor = 0;
				line.m_Vertices = {};
				m_aCommands[index * 2 + 1] = line;
			}
			if (inverted[index])
			{
				TriMeshDrawCommand outside = TriMeshDrawCommand.Cast(m_aCommands[index * 2]);
				if (!outside)
				{
					outside = new TriMeshDrawCommand();
					outside.m_Vertices = {};
					outside.m_Indices = {};
					m_aCommands[index * 2] = outside;
				}
				outside.m_iColor = fills[index];
			}
			else
			{
				PolygonDrawCommand fill = PolygonDrawCommand.Cast(m_aCommands[index * 2]);
				if (!fill)
				{
					fill = new PolygonDrawCommand();
					m_aCommands[index * 2] = fill;
				}
				fill.m_iColor = fills[index];
				fill.m_Vertices = line.m_Vertices;
			}
			line.m_iColor = colors[index];
			line.m_iOutlineColor = colors[index] & 0x00FFFFFF;
			if (line.m_Vertices.Count() != count * 2)
				line.m_Vertices.Resize(count * 2);
		}
		m_bProjectionDirty = true;
		m_bGeometryDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Native open completion occurs after viewport sizing and initial module centering.
	void OnMapReady(MapConfiguration config)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode)
			return;
		if (mode.GetState() != SCR_EGameModeState.GAME || !m_bGameMapOpened)
			m_bFramed = false;
		if (mode.GetState() == SCR_EGameModeState.GAME)
			m_bGameMapOpened = true;
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.GetMapWidget())
			m_ReadyMapParent = mapEntity.GetMapWidget().GetParent();
		Project(mapEntity);
		FrameFirstMap(m_aFrameBounds);
	}

	//------------------------------------------------------------------------------------------------
	protected void FrameFirstMap(array<vector> bounds)
	{
		if (m_bFramed || !bounds || bounds.Count() != 2 || m_ReadyMapParent != m_Parent)
			return;
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen() || !GC_LobbyGameMode.GetGameMode())
			return;
		float width, height, mapX, mapY;
		mapEntity.GetMapWidget().GetScreenSize(width, height);
		mapEntity.GetMapWidget().GetScreenPos(mapX, mapY);
		if (width <= 0 || height <= 0)
			return;

		// Lobby side panels cover the map. Fit in the readable space between them.
		float left = 0;
		float right = width;
		Widget menu = mapEntity.GetMapMenuRoot();
		array<string> panelNames = {"BriefingView", "PlayerListPanel"};
		foreach (int index, string name : panelNames)
		{
			Widget panel;
			if (menu)
				panel = menu.FindAnyWidget(name);
			if (!panel || !panel.IsVisible())
				continue;
			float x, y, panelWidth, panelHeight;
			panel.GetScreenPos(x, y);
			panel.GetScreenSize(panelWidth, panelHeight);
			if (index == 0)
				left = Math.Clamp(x - mapX + panelWidth, 0, width);
			else
				right = Math.Clamp(x - mapX, 0, width);
		}
		if (right <= left)
			return;
		vector minimum = bounds[0];
		vector maximum = bounds[1];
		float spanX = Math.Max(1, maximum[0] - minimum[0]);
		float spanZ = Math.Max(1, maximum[2] - minimum[2]);
		float zoom = Math.Min((right - left) * 0.84 / spanX, height * 0.84 / spanZ);
		zoom = Math.Clamp(zoom, mapEntity.GetMinZoom(), mapEntity.GetMaxZoom());
		if (zoom <= 0)
			return;
		vector center = (minimum + maximum) * 0.5;
		center[0] = center[0] + (width * 0.5 - (left + right) * 0.5) / zoom;
		// Mark before the native animation emits pan/zoom callbacks; never reframe on polling.
		m_bFramed = true;
		mapEntity.ZoomPanSmooth(zoom, center[0], center[2], 0.25);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapPan(float x, float y, bool adjusted)
	{
		m_bProjectionDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapZoom(float zoom)
	{
		m_bProjectionDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapClose(MapConfiguration config)
	{
		m_ReadyMapParent = null;
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void Project(SCR_MapEntity mapEntity)
	{
		if (!m_Canvas || !mapEntity || !mapEntity.IsOpen() || !GC_LobbyGameMode.GetGameMode())
			return;
		float canvasWidth, canvasHeight;
		m_Canvas.GetScreenSize(canvasWidth, canvasHeight);
		if (canvasWidth <= 0 || canvasHeight <= 0)
			return;

		// Use the native inverse transform to retain fractional screen positions.
		// WorldToScreen exposes integer outputs, which quantize slow movement.
		float originX, originZ;
		mapEntity.ScreenToWorld(0, 0, originX, originZ);
		float zoom = mapEntity.GetCurrentZoom();
		// Native map modules can emit pan/zoom callbacks even when the view is unchanged.
		if (!m_bGeometryDirty && originX == m_fProjectedX && originZ == m_fProjectedZ && zoom == m_fProjectedZoom
			&& canvasWidth == m_fCanvasWidth && canvasHeight == m_fCanvasHeight)
			return;
		m_bGeometryDirty = false;
		m_fProjectedX = originX;
		m_fProjectedZ = originZ;
		m_fProjectedZoom = zoom;
		m_fCanvasWidth = canvasWidth;
		m_fCanvasHeight = canvasHeight;
		int offset;
		array<vector> labelPositions = {};
		array<float> labelWidths = {};
		foreach (int index, int count : m_aCounts)
		{
			LineDrawCommand line = LineDrawCommand.Cast(m_aCommands[index * 2 + 1]);
			// The native transparent outline consumes stroke width; preserve its visual weight.
			line.m_fWidth = GetGame().GetWorkspace().DPIScale(m_aWidths[index]) + 3;
			for (int i; i < count; i++)
			{
				vector point = m_aPoints[offset + i];
				line.m_Vertices[i * 2] = (point[0] - originX) * zoom;
				line.m_Vertices[i * 2 + 1] = (originZ - point[2]) * zoom;
			}
			TriMeshDrawCommand outside = TriMeshDrawCommand.Cast(m_aCommands[index * 2]);
			if (outside)
			{
				GC_MapShapeOutsideFill.Build(line.m_Vertices, canvasWidth, canvasHeight, outside);
			}
			if (m_aLabels.IsIndexValid(index) && m_aLabels[index])
			{
				vector center;
				for (int pointIndex; pointIndex < count; pointIndex++)
					center += m_aPoints[offset + pointIndex];
				center /= count;

				float labelX = GetGame().GetWorkspace().DPIUnscale((center[0] - originX) * zoom);
				float labelY = GetGame().GetWorkspace().DPIUnscale((originZ - center[2]) * zoom);
				TextWidget text = TextWidget.Cast(m_aLabels[index].FindAnyWidget("Text"));
				float textWidth, textHeight;
				text.GetTextSize(textWidth, textHeight);
				FrameSlot.SetSizeX(m_aLabels[index], textWidth + 8);
				// Nearby zones remain separately readable when their centers overlap.
				for (int pass; pass < labelPositions.Count(); pass++)
				{
					bool shifted = false;
					foreach (int previous, vector position : labelPositions)
					{
						if (labelX < position[0] + labelWidths[previous] && labelX + textWidth + 8 > position[0]
							&& Math.AbsFloat(labelY - position[1]) < 24)
						{
							labelY = position[1] + 24;
							shifted = true;
						}
					}
					if (!shifted)
						break;
				}
				FrameSlot.SetPos(m_aLabels[index], labelX, labelY);
				labelPositions.Insert(Vector(labelX, labelY, 0));
				labelWidths.Insert(textWidth + 8);
			}
			offset += count;
		}
		m_Canvas.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		if (m_bListening)
		{
			SCR_MapEntity.GetOnMapPan().Remove(OnMapPan);
			SCR_MapEntity.GetOnMapZoom().Remove(OnMapZoom);
			SCR_MapEntity.GetOnMapClose().Remove(OnMapClose);
			m_bListening = false;
		}
		if (m_Root)
			m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Parent = null;
		m_Canvas = null;
		m_aLabels.Clear();

		m_fCanvasWidth = 0;
		m_fCanvasHeight = 0;
		m_aCommands.Clear();
		m_bProjectionDirty = false;
		m_ReadyMapParent = null;
	}
}
