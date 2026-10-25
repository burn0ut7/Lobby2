#ifdef WORKBENCH
//! Explicit test inputs for the production tool; nothing runs on load.
[WorkbenchToolAttribute("AO Polygon Tests", "Select the AO polygon fixture. Buttons invoke the production generator with fixed test inputs.")]
class GC_AOPolygonTestTool : GC_AOPolygonTool
{
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Triangle: 25 m, 90 degrees")]
	protected void GenerateTriangle()
	{
		m_iSides = 3;
		m_fRadius = 25;
		m_fRotation = 90;
		GeneratePolygon();
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Heptagon: 40 m, 360 degrees")]
	protected void GenerateHeptagon()
	{
		m_iSides = 7;
		m_fRadius = 40;
		m_fRotation = 360;
		GeneratePolygon();
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Reject two sides")]
	protected void RejectTwoSides()
	{
		m_iSides = 2;
		m_fRadius = 100;
		m_fRotation = 0;
		GeneratePolygon();
	}
}
#endif
