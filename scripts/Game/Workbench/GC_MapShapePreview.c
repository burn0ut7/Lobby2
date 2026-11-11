#ifdef WORKBENCH
//! Selected map-style polygon. Temporary visuals are never added to the editor source.
class GC_MapShapePreview
{
	protected ref array<ref Shape> m_aShapes = {};
	protected bool m_bReportedInvalid;
	protected ref array<vector> m_aPoints = {};
	protected int m_iLineColor;
	protected int m_iFillColor;
	protected float m_fWidth;
	protected bool m_bInverted;
	protected bool m_bClosed;
	protected bool m_bBuilt;
	protected int m_iLastPointChange;
	protected ref array<vector> m_aPendingPoints = {};
	protected vector m_vMinimum;
	protected vector m_vMaximum;

	//------------------------------------------------------------------------------------------------
	static bool IsSelected(GenericEntity owner)
	{
		if (!owner || !SCR_Global.IsEditMode())
			return false;
		WorldEditorAPI api = owner._WB_GetEditorAPI();
		return api && api.IsEntitySelected(api.EntityToSource(owner));
	}

	//------------------------------------------------------------------------------------------------
	void Update(PolylineShapeEntity owner, int lineColor, float width, int fillColor, bool inverted)
	{
		if (!IsSelected(owner))
		{
			Clear();
			m_bBuilt = false;
			m_aPendingPoints.Clear();
			return;
		}

		// Native edit actions include dragging the entity and its control points.
		// Drop the overlay once and keep all geometry work outside that transaction.
		WorldEditorAPI api = owner._WB_GetEditorAPI();
		if (api.IsDoingEditAction())
		{
			Clear();
			m_bBuilt = false;
			m_iLastPointChange = System.GetTickCount();
			return;
		}

		array<vector> points = {};
		owner.GetPointsPositions(points);
		// The map is an X/Z projection. Keep the preview flat and visible through terrain.
		float height = owner.GetOrigin()[1] + 0.15;
		for (int i = 0; i < points.Count(); i++)
		{
			vector point = owner.CoordToParent(points[i]);
			point[1] = height;
			points[i] = point;
		}
		// Defer triangulation and preview work until points stay still for 250 ms.
		// Active edit actions above remain suspended even when the mover pauses.
		if (!GC_MapPolygon.SamePoints(points, m_aPendingPoints))
		{
			Clear();
			m_bBuilt = false;
			m_aPendingPoints.Copy(points);
			m_iLastPointChange = System.GetTickCount();
			return;
		}
		if (System.GetTickCount(m_iLastPointChange) < 250)
			return;
		bool closed = owner.IsClosed();
		// Refresh the finite outside-fill canvas after edits settle.
		vector minimum = m_vMinimum;
		vector maximum = m_vMaximum;
		if (!m_bBuilt)
			owner.GetWorld().GetBoundBox(minimum, maximum);
		bool changed = !m_bBuilt || points.Count() != m_aPoints.Count() || closed != m_bClosed
			|| lineColor != m_iLineColor || fillColor != m_iFillColor || width != m_fWidth || inverted != m_bInverted;
		if (!changed)
		{
			foreach (int index, vector point : points)
			{
				if (point != m_aPoints[index])
				{
					changed = true;
					break;
				}
			}
		}
		if (!changed)
			return;

		Clear();
		m_aPoints.Copy(points);
		m_iLineColor = lineColor;
		m_iFillColor = fillColor;
		m_fWidth = width;
		m_bInverted = inverted;
		m_bClosed = closed;
		m_vMinimum = minimum;
		m_vMaximum = maximum;
		m_bBuilt = true;
		if (points.Count() >= 2)
		{
			bool valid = closed && GC_MapPolygon.IsValid(points);
			if (closed && !valid && !m_bReportedInvalid)
				Print("GC | Map preview | Invalid polygon: use 3-128 points without crossed or overlapping edges.", LogLevel.WARNING);
			m_bReportedInvalid = closed && !valid;
			if (valid && (fillColor & 0xFF000000) != 0)
				BuildFill(points, minimum, maximum, height, fillColor, inverted);
			if ((lineColor & 0xFF000000) != 0)
				BuildOutline(points, Math.Clamp(width, 1, 20) * 0.1, lineColor, closed);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildFill(array<vector> points, vector minimum, vector maximum, float height, int color, bool inverted)
	{
		// Outside fill needs a finite canvas. Use the world extent, expanded for any
		// authored points beyond it; interior fill uses the same concave-safe partition.
		foreach (vector point : points)
		{
			minimum[0] = Math.Min(minimum[0], point[0]);
			minimum[2] = Math.Min(minimum[2], point[2]);
			maximum[0] = Math.Max(maximum[0], point[0]);
			maximum[2] = Math.Max(maximum[2], point[2]);
		}
		array<float> polygon = {};
		foreach (vector point : points)
		{
			polygon.Insert(point[0] - minimum[0]);
			polygon.Insert(point[2] - minimum[2]);
		}
		TriMeshDrawCommand mesh = new TriMeshDrawCommand();
		mesh.m_Vertices = {};
		mesh.m_Indices = {};
		GC_MapShapeOutsideFill.Build(polygon, maximum[0] - minimum[0], maximum[2] - minimum[2], mesh, inverted);
		if (mesh.m_Indices.IsEmpty())
			return;

		vector origin = Vector(minimum[0], height, minimum[2]);

		// Fixed-size batches bound native buffer sizes without limiting polygon edges.
		vector triangles[1536];
		int used;
		foreach (int index : mesh.m_Indices)
		{
			triangles[used] = Vector(mesh.m_Vertices[index * 2], 0, mesh.m_Vertices[index * 2 + 1]);
			used++;
			if (used == 1536)
			{
				AddShapeBatch(triangles, used, color, origin);
				used = 0;
			}
		}
		if (used > 0)
			AddShapeBatch(triangles, used, color, origin);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddShapeBatch(vector triangles[], int count, int color, vector origin)
	{
		// Debug shapes never enter the world's entity/spatial/mesh resource systems.
		Shape shape = Shape.CreateTris(color, ShapeFlags.NOZBUFFER | ShapeFlags.NOZWRITE
			| ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOOUTLINE, triangles, count / 3);
		if (!shape)
			return;
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = origin;
		shape.SetMatrix(transform);
		m_aShapes.Insert(shape);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildOutline(array<vector> points, float width, int color, bool closed)
	{
		vector origin = points[0];
		vector triangles[1536];
		int used;
		int edges = points.Count() - 1;
		if (closed)
			edges++;
		for (int edge = 0; edge < edges; edge++)
		{
			vector a = points[edge] - origin;
			vector b = points[(edge + 1) % points.Count()] - origin;
			vector delta = b - a;
			float length = delta.Length();
			if (length < 0.001)
				continue;
			vector side = Vector(-delta[2], 0, delta[0]) * (width * 0.5 / length);
			triangles[used] = a + side;
			triangles[used + 1] = b + side;
			triangles[used + 2] = b - side;
			triangles[used + 3] = a + side;
			triangles[used + 4] = b - side;
			triangles[used + 5] = a - side;
			used += 6;
			if (used == 1536)
			{
				AddShapeBatch(triangles, used, color, origin);
				used = 0;
			}
		}
		if (used > 0)
			AddShapeBatch(triangles, used, color, origin);
	}

	//------------------------------------------------------------------------------------------------
	protected void Clear()
	{
		m_aShapes.Clear();
	}
}
#endif
