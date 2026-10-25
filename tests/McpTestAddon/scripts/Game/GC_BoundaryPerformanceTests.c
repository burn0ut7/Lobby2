#ifdef WORKBENCH
//! Explicit bounded profiling only; production implementations remain authoritative.
class GC_BoundaryPerformanceProbe
{
	static bool s_bActive;
	static int s_iStart;
	static int s_iEnd;
	static ref array<int> s_aCalls = {0, 0, 0, 0, 0};
	static ref array<int> s_aMilliseconds = {0, 0, 0, 0, 0};
	static ref array<int> s_aMaximum = {0, 0, 0, 0, 0};

	//------------------------------------------------------------------------------------------------
	static void Record(int index, int start)
	{
		int elapsed = System.GetTickCount() - start;
		s_aCalls[index] = s_aCalls[index] + 1;
		s_aMilliseconds[index] = s_aMilliseconds[index] + elapsed;
		s_aMaximum[index] = Math.Max(s_aMaximum[index], elapsed);
	}
}

modded class GC_BoundaryWallComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		int start = System.GetTickCount();
		super.EOnFrame(owner, timeSlice);
		if (GC_BoundaryPerformanceProbe.s_bActive)
			GC_BoundaryPerformanceProbe.Record(0, start);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool UpdateNearbyMesh(vector position)
	{
		int start = System.GetTickCount();
		bool result = super.UpdateNearbyMesh(position);
		if (GC_BoundaryPerformanceProbe.s_bActive)
			GC_BoundaryPerformanceProbe.Record(1, start);
		return result;
	}
}

modded class GC_MapShapeOverlay
{
	//------------------------------------------------------------------------------------------------
	override protected void Project(SCR_MapEntity mapEntity)
	{
		int start = System.GetTickCount();
		super.Project(mapEntity);
		if (GC_BoundaryPerformanceProbe.s_bActive)
			GC_BoundaryPerformanceProbe.Record(2, start);
	}
}

modded class GC_AOLimitSystem
{
	//------------------------------------------------------------------------------------------------
	override protected void Evaluate()
	{
		int start = System.GetTickCount();
		super.Evaluate();
		if (GC_BoundaryPerformanceProbe.s_bActive)
			GC_BoundaryPerformanceProbe.Record(3, start);
	}
}

modded class GC_SetupZoneEntity
{
	//------------------------------------------------------------------------------------------------
	override protected void Evaluate()
	{
		int start = System.GetTickCount();
		super.Evaluate();
		if (GC_BoundaryPerformanceProbe.s_bActive)
			GC_BoundaryPerformanceProbe.Record(4, start);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryPerformanceStart(int seconds)
	{
		if (GC_BoundaryPerformanceProbe.s_bActive || seconds < 5 || seconds > 30)
			return "REJECTED active probe or duration outside 5..30 seconds";
		for (int i; i < 5; i++)
		{
			GC_BoundaryPerformanceProbe.s_aCalls[i] = 0;
			GC_BoundaryPerformanceProbe.s_aMilliseconds[i] = 0;
			GC_BoundaryPerformanceProbe.s_aMaximum[i] = 0;
		}
		GC_BoundaryPerformanceProbe.s_iStart = System.GetTickCount();
		GC_BoundaryPerformanceProbe.s_iEnd = 0;
		GC_BoundaryPerformanceProbe.s_bActive = true;
		GetGame().GetCallqueue().CallLater(GC_TestBoundaryPerformanceStop, seconds * 1000);
		return "STARTED wall-frame, wall-mesh, map-project, AO-evaluate, setup-evaluate; integer millisecond timing";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBoundaryPerformanceStop()
	{
		GC_BoundaryPerformanceProbe.s_bActive = false;
		GC_BoundaryPerformanceProbe.s_iEnd = System.GetTickCount();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryPerformanceReport()
	{
		string report = string.Format("active=%1 elapsedMs=%2\n", GC_BoundaryPerformanceProbe.s_bActive,
			GC_BoundaryPerformanceProbe.s_iEnd - GC_BoundaryPerformanceProbe.s_iStart);
		for (int i; i < 5; i++)
			report += string.Format("index=%1 calls=%2 totalMs=%3 maxMs=%4\n", i,
				GC_BoundaryPerformanceProbe.s_aCalls[i], GC_BoundaryPerformanceProbe.s_aMilliseconds[i], GC_BoundaryPerformanceProbe.s_aMaximum[i]);
		return report;
	}

	//------------------------------------------------------------------------------------------------
	//! Microbenchmark the actual outside-fill builder with a simple concave star.
	string GC_TestBoundaryFillBenchmark(int vertices, int repetitions)
	{
		if (vertices < 4 || vertices > 128 || repetitions < 10 || repetitions > 1000)
			return "REJECTED benchmark bounds";
		array<float> polygon = {};
		for (int i; i < vertices; i++)
		{
			float angle = Math.PI2 * i / vertices;
			float radius = 500;
			if (i % 2 == 0)
				radius = 200;
			polygon.Insert(1306 + Math.Cos(angle) * radius);
			polygon.Insert(704 + Math.Sin(angle) * radius);
		}
		TriMeshDrawCommand mesh = new TriMeshDrawCommand();
		mesh.m_Vertices = {};
		mesh.m_Indices = {};
		GC_MapShapeOutsideFill.Build(polygon, 2613, 1408, mesh);
		int start = System.GetTickCount();
		for (int sample; sample < repetitions; sample++)
			GC_MapShapeOutsideFill.Build(polygon, 2613, 1408, mesh);
		return string.Format("vertices=%1 repetitions=%2 elapsedMs=%3 triangles=%4", vertices, repetitions,
			System.GetTickCount() - start, mesh.m_Indices.Count() / 3);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		GetGame().GetCallqueue().Remove(GC_TestBoundaryPerformanceStop);
		GC_BoundaryPerformanceProbe.s_bActive = false;
		super.Shutdown();
	}
}
#endif
