#ifdef WORKBENCH
[WorkbenchToolAttribute("Map / AO Polygon", "Select one map shape or AO limit and replace its points with a regular polygon.\nRadius is centre-to-vertex distance in metres.\nUse the normal Move/Rotate and Vector tools for subsequent editing.\nGeneration replaces existing points; Undo restores them.", awesomeFontCode: 0xF5EE)]
class GC_AOPolygonTool : WorldEditorTool
{
	[Attribute("5", UIWidgets.EditBox, "Number of equally spaced vertices and sides.", params: "3 128 1", category: "Polygon")]
	protected int m_iSides;

	[Attribute("100", UIWidgets.EditBox, "Metres from the shape entity's origin to each vertex (not side length or diameter).", params: "0.1 100000", category: "Polygon")]
	protected float m_fRadius;

	[Attribute("0", UIWidgets.Slider, "Clockwise degrees around the shape's local Y axis. Zero places the first point along local +Z. Entity rotation applies in addition.", params: "0 360 1", category: "Polygon")]
	protected float m_fRotation;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Generate polygon")]
	protected void GeneratePolygon()
	{
		if (!m_API || !m_API.IsEditMode() || m_API.IsGameMode() || m_API.IsDoingEditAction())
			return;

		if (m_iSides < 3 || m_iSides > 128 || m_fRadius < 0.1 || m_fRadius > 100000 || m_fRotation < 0 || m_fRotation > 360)
		{
			Print("GC | Polygon | Use 3-128 sides, radius 0.1-100000 m and rotation 0-360 degrees.", LogLevel.WARNING);
			return;
		}

		if (m_API.GetSelectedEntitiesCount() != 1)
		{
			Print("GC | Polygon | Select exactly one map shape or AO limit.", LogLevel.WARNING);
			return;
		}

		IEntitySource source = m_API.GetSelectedEntity();
		PolylineShapeEntity shape = PolylineShapeEntity.Cast(m_API.SourceToEntity(source));
		if (!shape || (!GC_AOLimitEntity.Cast(shape) && !shape.FindComponent(GC_MapShapeComponent))
			|| source.GetSubScene() != m_API.GetCurrentSubScene()
			|| m_API.IsEntityLayerLockedHierarchy(source.GetSubScene(), source.GetLayerID()))
		{
			Print("GC | Polygon | Select a map shape or AO limit in an unlocked layer of the active subscene.", LogLevel.WARNING);
			return;
		}

		// Native object-array deletion cannot persist removal of inherited points.
		// Geometry belongs on the placed entity; the bundled prefabs contain settings only.
		BaseContainer ancestor = source.GetAncestor();
		if (ancestor && ancestor.GetObjectArray("Points").Count() > 0)
		{
			Print("GC | Polygon | Points are inherited. Generate on the shape's defining prefab, or use the empty MapShape/AOLimit prefab for instance geometry.", LogLevel.WARNING);
			return;
		}

		array<vector> points = {};
		for (int i; i < m_iSides; i++)
		{
			float angle = (m_fRotation + 360.0 * i / m_iSides) * Math.DEG2RAD;
			points.Insert(Vector(Math.Sin(angle) * m_fRadius, 0, Math.Cos(angle) * m_fRadius));
		}

		if (!m_API.BeginEntityAction("Generate polygon"))
			return;

		// As in the shipped Shape Area Tool, deselect before changing points to
		// release any Vector Tool point selection. The engine owns serialization.
		m_API.RemoveFromEntitySelection(source);
		bool changed = ReplacePoints(source, points);
		bool closed;
		if (changed)
			closed = m_API.SetVariableValue(source, null, "IsClosed", "1");
		m_API.AddToEntitySelection(source);
		m_API.EndEntityAction();

		if (!changed || !closed || source.GetObjectArray("Points").Count() != m_iSides)
			Print("GC | Polygon | Generation did not match the requested shape; inspect the points or Undo.", LogLevel.ERROR);
		else
			PrintFormat("GC | Polygon | Generated %1 sides, radius=%2 m, rotation=%3 degrees.", m_iSides, m_fRadius, m_fRotation);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ReplacePoints(IEntitySource source, array<vector> points)
	{
		int originalCount = source.GetObjectArray("Points").Count();
		foreach (int i, vector point : points)
		{
			int index = originalCount + i;
			if (!m_API.CreateObjectArrayVariableMember(source, null, "Points", "ShapePoint", index))
				return false;
			if (!m_API.SetVariableValue(source, { new ContainerIdPathEntry("Points", index) },
				"Position", string.Format("%1 %2 %3", point[0], point[1], point[2])))
				return false;
		}

		for (int i = originalCount - 1; i >= 0; i--)
		{
			if (!m_API.RemoveObjectArrayVariableMember(source, null, "Points", i))
				return false;
		}
		return true;
	}
}
#endif
