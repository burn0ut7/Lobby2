//! Optional timing probe observes production opening and projection without changing them.
modded class GC_MapShapeOverlay
{
	protected int m_GC_iObservedGeneration;
	protected int m_GC_iProjectionSamples;
	protected float m_GC_fOpenedAt;

	//------------------------------------------------------------------------------------------------
	override void Update(GC_LobbyPlayerController controller)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen() && mapEntity.GetMapWidget()
			&& mapEntity.GetMapWidget().GetParent() != m_Parent)
		{
			m_GC_iProjectionSamples = 0;
			m_GC_fOpenedAt = GetGame().GetWorld().GetWorldTime();
		}
		super.Update(controller);
		if (m_Canvas && m_iGeneration != m_GC_iObservedGeneration)
		{
			m_GC_iObservedGeneration = m_iGeneration;
			PrintFormat("GC | Map opening | attached generation=%1 commands=%2", m_iGeneration, m_aCommands.Count());
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapReady(MapConfiguration config)
	{
		super.OnMapReady(config);
		PrintFormat("GC | Map opening | ready generation=%1 elapsed=%2", m_iGeneration, GetGame().GetWorld().GetWorldTime() - m_GC_fOpenedAt);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Project(SCR_MapEntity mapEntity)
	{
		super.Project(mapEntity);
		if (!m_Canvas || m_GC_iProjectionSamples >= 6)
			return;
		float width, height;
		m_Canvas.GetScreenSize(width, height);
		int triangles;
		foreach (CanvasWidgetCommand command : m_aCommands)
		{
			TriMeshDrawCommand fill = TriMeshDrawCommand.Cast(command);
			if (fill)
				triangles += fill.m_Indices.Count() / 3;
		}
		PrintFormat("GC | Map opening | projected generation=%1 elapsed=%2 canvas=%3,%4 commands=%5 triangles=%6", m_iGeneration, GetGame().GetWorld().GetWorldTime() - m_GC_fOpenedAt, width, height, m_aCommands.Count(), triangles);
		m_GC_iProjectionSamples++;
	}
}
