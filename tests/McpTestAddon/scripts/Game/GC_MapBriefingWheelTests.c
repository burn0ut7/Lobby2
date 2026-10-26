//! Synthetic coordinates exercise the production wheel handlers without desktop input.
#ifdef WORKBENCH
[BaseContainerProps()]
modded class SCR_MapCursorModule
{
	protected bool m_bGC_TestWheelPointer;
	protected int m_iGC_TestWheelX;
	protected int m_iGC_TestWheelY;

	//------------------------------------------------------------------------------------------------
	override protected void GC_GetWheelPointer(out int mouseX, out int mouseY)
	{
		if (!m_bGC_TestWheelPointer)
		{
			super.GC_GetWheelPointer(mouseX, mouseY);
			return;
		}
		mouseX = m_iGC_TestWheelX;
		mouseY = m_iGC_TestWheelY;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestWheel(int mouseX, int mouseY, int wheel)
	{
		m_iGC_TestWheelX = mouseX;
		m_iGC_TestWheelY = mouseY;
		m_bGC_TestWheelPointer = true;
		bool briefingOwns = GC_BriefingOwnsWheel();
		float before = m_MapEntity.GetTargetZoomPPU();
		if (wheel > 0)
			OnInputZoomWheelUp(120, EActionTrigger.VALUE);
		else
			OnInputZoomWheelDown(120, EActionTrigger.VALUE);
		float after = m_MapEntity.GetTargetZoomPPU();
		m_bGC_TestWheelPointer = false;
		return string.Format("owned=%1 zoomBefore=%2 zoomAfter=%3", briefingOwns, before, after);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMapBriefingWheel(string targetName, int wheel)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return "UNAVAILABLE open map";
		SCR_MapCursorModule cursor = SCR_MapCursorModule.Cast(mapEntity.GetMapModule(SCR_MapCursorModule));
		if (!cursor)
			return "UNAVAILABLE cursor";
		if (targetName == "map")
			return cursor.GC_TestWheel(20, 200, wheel);

		Widget target = mapEntity.GetMapMenuRoot().FindAnyWidget(targetName);
		if (!target)
			return "UNAVAILABLE target";
		float x, y, width, height;
		target.GetScreenPos(x, y);
		target.GetScreenSize(width, height);
		int mouseX = x + width * 0.5;
		int mouseY = y + height * 0.5;
		string result = cursor.GC_TestWheel(mouseX, mouseY, wheel);
		GC_BriefingScrollComponent handler = GC_BriefingScrollComponent.Cast(target.FindHandler(GC_BriefingScrollComponent));
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(target);
		if (handler && scroll)
		{
			float scrollX, beforeY, afterY;
			scroll.GetSliderPosPixels(scrollX, beforeY);
			bool consumed = handler.OnMouseWheel(target, mouseX, mouseY, wheel);
			scroll.GetSliderPosPixels(scrollX, afterY);
			result += string.Format(" consumed=%1 scrollBefore=%2 scrollAfter=%3", consumed, beforeY, afterY);
		}
		return result;
	}
}
#endif
