class GC_AdminTableRowComponent : SCR_ButtonBaseComponent
{
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		bool useColorization = m_bUseColorization;
		m_bUseColorization = false;
		super.HandlerAttached(w);
		m_bUseColorization = useColorization;
		// Nested action buttons also have Background/Border children.
		m_wBackground = w.FindAnyWidget("TableBackground");
		m_wBorder = w.FindAnyWidget("TableFocus");
		m_wBorder.SetOpacity(0);
		ColorizeBackground(false);
	}

	//------------------------------------------------------------------------------------------------
	override void SetToggled(bool toggled, bool animate = true, bool invokeChange = true, bool instant = false)
	{
		super.SetToggled(toggled, animate, invokeChange, instant);
		Widget edge = GetRootWidget().FindAnyWidget("SelectionEdge");
		if (edge)
			edge.SetVisible(IsToggled());
	}
}
