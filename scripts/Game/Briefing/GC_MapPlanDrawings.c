//! The optional MapDrawing addon supplies this part of the same plan pipeline.
class GC_MapPlanDrawings
{
	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		// A loaded drawing mod needs its optional integration; do not export a
		// misleading marker-only plan while unhandled drawings may be present.
		return !"TILW_DrawingSyncComponent".ToType();
	}

	//------------------------------------------------------------------------------------------------
	void Capture(array<ref array<int>> drawings)
	{
	}

	//------------------------------------------------------------------------------------------------
	bool Validate(array<ref array<int>> drawings)
	{
		return drawings && drawings.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	void Replace(array<ref array<int>> drawings)
	{
	}
}
