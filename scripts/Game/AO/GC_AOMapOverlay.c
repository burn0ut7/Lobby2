//! Passive map overlay. Authority supplies geometry; only projection happens here.
class GC_AOMapOverlay
{
	protected Widget m_Root;
	protected Widget m_Parent;
	protected CanvasWidget m_Canvas;
	protected ref array<int> m_aCounts = {};
	protected ref array<int> m_aColors = {};
	protected ref array<float> m_aWidths = {};
	protected ref array<vector> m_aPoints = {};
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};
	protected float m_fLastReceive;
	protected float m_fNextRequest;
	protected int m_iGeneration;
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
		}

		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fNextRequest)
		{
			m_fNextRequest = now + 500;
			controller.GC_RequestAOMap(m_iGeneration);
		}
		m_aCommands.Clear();
		if (now - m_fLastReceive <= 1500)
			Project(mapEntity);
		m_Canvas.SetDrawCommands(m_aCommands);
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
		m_aColors.Copy(colors);
		m_aWidths.Copy(widths);
		m_aPoints.Copy(points);
		m_fLastReceive = GetGame().GetWorld().GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	protected void Project(SCR_MapEntity mapEntity)
	{
		int offset;
		foreach (int index, int count : m_aCounts)
		{
			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = m_aColors[index];
			line.m_fWidth = GetGame().GetWorkspace().DPIScale(m_aWidths[index]);
			line.m_bShouldEnclose = true;
			line.m_Vertices = {};
			for (int i; i < count; i++)
			{
				vector point = m_aPoints[offset + i];
				int x, y;
				mapEntity.WorldToScreen(point[0], point[2], x, y, true);
				// WorldToScreen returns scaled coordinates local to the map frame.
				line.m_Vertices.Insert(x);
				line.m_Vertices.Insert(y);
			}
			offset += count;
			m_aCommands.Insert(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		if (m_Root)
			m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Parent = null;
		m_Canvas = null;
		m_aCounts.Clear();
		m_aColors.Clear();
		m_aWidths.Clear();
		m_aPoints.Clear();
		m_aCommands.Clear();
		m_fNextRequest = 0;
	}
}
