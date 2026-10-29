#ifdef WORKBENCH
modded class GC_SlideOutComponent
{
	protected int m_iGC_EdgeX;
	protected int m_iGC_EdgeY;
	protected int m_iGC_EdgeTicks;
	protected int m_iGC_EdgeReversals;
	protected float m_fGC_EdgeMinimum;
	protected float m_fGC_EdgeLastTarget;
	protected float m_fGC_EdgeFirstRight;
	protected string m_sGC_EdgeResult;

	//------------------------------------------------------------------------------------------------
	protected void GC_TestEdgeFrame()
	{
		m_iGC_EdgeTicks++;
		bool inside;
		if (m_iGC_EdgeTicks <= 120)
			inside = ContainsPointer(m_iGC_EdgeX, m_iGC_EdgeY);
		SetHovered(inside);
		if (m_fGC_EdgeLastTarget != m_fTargetProgress)
			m_iGC_EdgeReversals++;
		m_fGC_EdgeLastTarget = m_fTargetProgress;
		if (m_iGC_EdgeTicks >= 30 && m_iGC_EdgeTicks <= 120)
			m_fGC_EdgeMinimum = Math.Min(m_fGC_EdgeMinimum, m_fProgress);
		if (m_iGC_EdgeTicks == 120)
		{
			float x, y, width, height;
			m_wContent.GetScreenPos(x, y);
			m_wContent.GetScreenSize(width, height);
			m_sGC_EdgeResult = string.Format("ticks=120 firstRight=%1 heldRight=%2 minProgress=%3 targetChanges=%4", m_fGC_EdgeFirstRight, x + width, m_fGC_EdgeMinimum, m_iGC_EdgeReversals);
		}
		if (m_iGC_EdgeTicks == 150)
		{
			m_sGC_EdgeResult += " afterLeave=" + m_fProgress.ToString();
			GetGame().GetCallqueue().Remove(GC_TestEdgeFrame);
			GetGame().GetCallqueue().CallLater(UpdatePointerHover, HOVER_TICK_MS, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(GC_TestEdgeFrame);
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestScreenEdge(string action)
	{
		if (action == "focus")
			GC_TestHandleFocus(true);
		if (action == "away")
		{
			SetHovered(false);
			GetGame().GetCallqueue().Remove(UpdatePointerHover);
			GetGame().GetCallqueue().CallLater(UpdatePointerHover, HOVER_TICK_MS, true);
		}
		if (action == "focus" || action == "away" || action == "snapshot")
			return GC_TestDrawerState();
		if (action == "result")
			return m_sGC_EdgeResult;
		if (action == "open")
		{
			float handleX, handleY, handleWidth, handleHeight;
			m_wHandle.GetScreenPos(handleX, handleY);
			m_wHandle.GetScreenSize(handleWidth, handleHeight);
			// Synthetic pointer geometry; does not move the OS cursor.
			GetGame().GetCallqueue().Remove(UpdatePointerHover);
			SetHovered(ContainsPointer(handleX + handleWidth * 0.5, handleY + handleHeight * 0.5));
			return GC_TestDrawerState();
		}
		float x, y, width, height;
		m_wContent.GetScreenPos(x, y);
		m_wContent.GetScreenSize(width, height);
		int edgeX = Math.Round(x + width);
		int centerY = Math.Round(y + height * 0.5);
		bool edgeInside = ContainsPointer(edgeX, centerY);
		if (action == "run")
		{
			GetGame().GetCallqueue().Remove(UpdatePointerHover);
			m_iGC_EdgeX = edgeX;
			m_iGC_EdgeY = centerY;
			m_iGC_EdgeTicks = 0;
			m_iGC_EdgeReversals = 0;
			m_fGC_EdgeMinimum = 1;
			m_fGC_EdgeLastTarget = 0;
			m_fGC_EdgeFirstRight = x + width;
			m_sGC_EdgeResult = "running";
			SetHovered(false);
			for (int tick; tick < 20; tick++)
				Animate();
			GetGame().GetCallqueue().CallLater(GC_TestEdgeFrame, 16, true);
			return "Started frame-separated edge probe";
		}
		if (action == "probe")
			return string.Format("right=%1 pixel=%2 inside=%3 beyond=%4", x + width, edgeX, edgeInside, ContainsPointer(edgeX + 3, centerY));

		// Deterministic hover/animation ticks against real rendered geometry.
		// This exercises production hit testing, not OS pointer movement.
		GetGame().GetCallqueue().Remove(UpdatePointerHover);
		GetGame().GetCallqueue().Remove(Animate);
		int reversals;
		float minimum = m_fProgress;
		float lastTarget = m_fTargetProgress;
		for (int i; i < 120; i++)
		{
			SetHovered(ContainsPointer(edgeX, centerY));
			if (lastTarget != m_fTargetProgress)
				reversals++;
			lastTarget = m_fTargetProgress;
			Animate();
			minimum = Math.Min(minimum, m_fProgress);
		}
		string state = GC_TestDrawerState();
		string result = string.Format("edge=%1 inside=%2 minProgress=%3 reversals=%4 ", edgeX, edgeInside, minimum, reversals);
		result += state;
		if (action == "leave")
		{
			SetHovered(ContainsPointer(-100, -100));
			for (int j; j < 20; j++)
				Animate();
			result += " left=" + GC_TestDrawerState();
		}
		GetGame().GetCallqueue().Remove(Animate);
		GetGame().GetCallqueue().CallLater(UpdatePointerHover, HOVER_TICK_MS, true);
		return result;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSlideOutEdge(string name, string action)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget drawer = menu.GetRootWidget().FindAnyWidget(name);
		if (!drawer)
			return "UNAVAILABLE drawer";
		GC_SlideOutComponent component = GC_SlideOutComponent.Cast(drawer.FindHandler(GC_SlideOutComponent));
		if (!component)
			return "UNAVAILABLE handler";
		return component.GC_TestScreenEdge(action);
	}
}
#endif
