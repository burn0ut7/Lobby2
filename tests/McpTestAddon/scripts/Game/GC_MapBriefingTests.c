//! Optional production-handler checks; focus is not physical pointer coverage.
#ifdef WORKBENCH
modded class GC_SlideOutComponent
{
	void GC_TestHandleFocus(bool focused)
	{
		if (focused)
			GetGame().GetWorkspace().SetFocusedWidget(m_wHandle);
		else
			GetGame().GetWorkspace().SetFocusedWidget(null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDrawerState()
	{
		float x, y, width, height;
		m_wRoot.GetScreenPos(x, y);
		m_wRoot.GetScreenSize(width, height);
		return string.Format("pinned=%1 progress=%2 target=%3 opacity=%4 contentEnabled=%5 rect=%6,%7,%8,%9",
			m_bPinned, m_fProgress, m_fTargetProgress, m_fDisplayedHandleOpacity,
			m_wContent && m_wContent.IsEnabled(), x, y, width, height)
			+ string.Format(" logicalWidth=%1", FrameSlot.GetSizeX(m_wRoot));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDrawerProximity()
	{
		float x, y, width, height;
		m_wHandle.GetScreenPos(x, y);
		m_wHandle.GetScreenSize(width, height);
		float inside = GetPointerProximity(m_wHandle, x + width * 0.5, y + height * 0.5);
		float near = GetPointerProximity(m_wHandle, x - m_fHandleSize * 3, y + height * 0.5);
		float far = GetPointerProximity(m_wHandle, x - m_fHandleSize * 6, y + height * 0.5);
		return string.Format("inside=%1 near=%2 far=%3", inside, near, far);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSpectatorDrawer(bool focus)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget drawer = menu.GetRootWidget().FindAnyWidget("SpectatorPlayerSlideOut");
		if (!drawer)
			return "UNAVAILABLE spectator drawer";
		GC_SlideOutComponent slideOut = GC_SlideOutComponent.Cast(drawer.FindHandler(GC_SlideOutComponent));
		if (!slideOut)
			return "FAILED slide-out handler";
		slideOut.GC_TestHandleFocus(focus);
		return slideOut.GC_TestDrawerState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapBriefing(string action)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return "map=closed";
		Widget root = mapEntity.GetMapMenuRoot();
		if (!root)
			return "UNAVAILABLE map root";
		Widget drawer = root.FindAnyWidget("MapBriefingSlideOut");
		if (!drawer)
			return "drawer=absent";
		GC_SlideOutComponent slideOut = GC_SlideOutComponent.Cast(drawer.FindHandler(GC_SlideOutComponent));
		if (!slideOut)
			return "FAILED slide-out handler";
		if (action == "proximity")
			return slideOut.GC_TestDrawerProximity();
		if (action == "focus")
			slideOut.GC_TestHandleFocus(true);
		else if (action == "closeFocused")
		{
			slideOut.GC_TestHandleFocus(true);
			if (!slideOut.IsPinned())
				slideOut.TogglePinned();
			slideOut.TogglePinned();
			// A focus event from the same click must not reopen during collapse.
			slideOut.GC_TestHandleFocus(true);
		}
		else if (action == "leave")
			slideOut.GC_TestHandleFocus(false);
		else if (action == "pin")
			slideOut.TogglePinned();
		else if (action != "snapshot")
			return "REJECTED action";

		Widget toggle = drawer.FindAnyWidget("BriefingToggle");
		TextWidget heading = TextWidget.Cast(drawer.FindAnyWidget("BriefingSectionTitle"));
		string title;
		if (heading)
			title = heading.GetText();
		return slideOut.GC_TestDrawerState() + string.Format(" hideVisible=%1 title=%2", toggle && toggle.IsVisibleInHierarchy(), title);
	}
}
#endif
