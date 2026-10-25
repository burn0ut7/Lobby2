//! Passive map overlay. Authority supplies geometry; only projection happens here.
class GC_AOMapOverlay
{
	protected Widget m_Root;
	protected Widget m_Parent;
	protected CanvasWidget m_Canvas;
	protected ref array<int> m_aCounts = {};
	protected ref array<float> m_aWidths = {};
	protected ref array<vector> m_aPoints = {};
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};
	protected float m_fLastReceive;
	protected float m_fNextRequest;
	protected int m_iGeneration;
	protected bool m_bListening;
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
		}

		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fNextRequest)
		{
			m_fNextRequest = now + 500;
			controller.GC_RequestAOMap(m_iGeneration);
		}
		// This timer owns discovery, snapshots and expiry, never moving-map projection.
		if (now - m_fLastReceive > 1500 && !m_aCommands.IsEmpty())
		{
			m_aCommands.Clear();
			m_Canvas.SetDrawCommands(m_aCommands);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Receive(int generation, array<int> counts, array<int> colors, array<float> widths, array<vector> points)
	{
		if (!m_Canvas || generation != m_iGeneration || counts.Count() != colors.Count() || counts.Count() != widths.Count())
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
		m_aWidths.Copy(widths);
		m_aPoints.Copy(points);
		m_fLastReceive = GetGame().GetWorld().GetWorldTime();
		m_aCommands.Resize(counts.Count());
		foreach (int index, int count : counts)
		{
			LineDrawCommand line = LineDrawCommand.Cast(m_aCommands[index]);
			if (!line)
			{
				line = new LineDrawCommand();
				line.m_bShouldEnclose = true;
				line.m_Vertices = {};
				m_aCommands[index] = line;
			}
			line.m_iColor = colors[index];
			if (line.m_Vertices.Count() != count * 2)
				line.m_Vertices.Resize(count * 2);
		}
		Project(SCR_MapEntity.GetMapInstance());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapPan(float x, float y, bool adjusted)
	{
		Project(SCR_MapEntity.GetMapInstance());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapZoom(float zoom)
	{
		Project(SCR_MapEntity.GetMapInstance());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapClose(MapConfiguration config)
	{
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void Project(SCR_MapEntity mapEntity)
	{
		if (!m_Canvas || !mapEntity || !mapEntity.IsOpen() || !GC_LobbyGameMode.GetGameMode())
			return;

		int offset;
		foreach (int index, CanvasWidgetCommand command : m_aCommands)
		{
			int count = m_aCounts[index];
			LineDrawCommand line = LineDrawCommand.Cast(command);
			line.m_fWidth = GetGame().GetWorkspace().DPIScale(m_aWidths[index]);
			for (int i; i < count; i++)
			{
				vector point = m_aPoints[offset + i];
				int x, y;
				mapEntity.WorldToScreen(point[0], point[2], x, y, true);
				// WorldToScreen returns scaled coordinates local to the map frame.
				line.m_Vertices[i * 2] = x;
				line.m_Vertices[i * 2 + 1] = y;
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
		m_aCounts.Clear();
		m_aWidths.Clear();
		m_aPoints.Clear();
		m_aCommands.Clear();
		m_fNextRequest = 0;
	}
}
