//! Owns briefing-region widget wheel input. Map action listeners are guarded separately.
//! Overflow state is derived from the laid-out viewport and content sizes.
class GC_BriefingScrollComponent : SCR_ScriptedWidgetComponent
{
	[Attribute("BriefingSectionContentSize", UIWidgets.EditBox, "Widget whose laid-out height determines whether this region overflows")]
	protected string m_sContentWidgetName;

	[Attribute("BriefingOverflowIndicator", UIWidgets.EditBox, "Indicator shown while more content remains below this region")]
	protected string m_sOverflowIndicatorWidgetName;

	protected static const float SCROLL_STEP = 64;
	protected static const float OVERFLOW_EPSILON = 1;
	protected static const float END_EPSILON = 0.001;

	protected ScrollLayoutWidget m_wScroll;
	protected Widget m_wContent;
	protected Widget m_wOverflowIndicator;
	protected bool m_bOverflow;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wScroll = ScrollLayoutWidget.Cast(w);
		m_wContent = w.FindAnyWidget(m_sContentWidgetName);

		Widget root = w;
		while (root && root.GetParent())
			root = root.GetParent();
		if (root)
			m_wOverflowIndicator = root.FindAnyWidget(m_sOverflowIndicatorWidgetName);

		RefreshOverflow();
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(UpdateOverflowState);
		m_wScroll = null;
		m_wContent = null;
		m_wOverflowIndicator = null;
		m_bOverflow = false;
		super.HandlerDeattached(w);
	}

	void RefreshOverflow()
	{
		GetGame().GetCallqueue().Remove(UpdateOverflowState);
		GetGame().GetCallqueue().CallLater(UpdateOverflowState, 0);
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_wScroll)
			return false;

		// Consume widget propagation even when this region has no overflow.
		if (!m_bOverflow || wheel == 0)
		{
			m_wScroll.SetSliderPos(0, 0);
			UpdateOverflowIndicator();
			return true;
		}

		float scrollX;
		float scrollY;
		m_wScroll.GetSliderPosPixels(scrollX, scrollY);
		float scaledStep = SCROLL_STEP;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			scaledStep = workspace.DPIScale(SCROLL_STEP);

		if (wheel > 0)
			scrollY -= scaledStep;
		else
			scrollY += scaledStep;

		m_wScroll.SetSliderPosPixels(0, scrollY);
		UpdateOverflowIndicator();
		return true;
	}

	protected void UpdateOverflowState()
	{
		if (!m_wScroll || !m_wContent)
			return;

		float viewportWidth;
		float viewportHeight;
		float contentWidth;
		float contentHeight;
		m_wScroll.GetScreenSize(viewportWidth, viewportHeight);
		m_wContent.GetScreenSize(contentWidth, contentHeight);
		m_bOverflow = viewportHeight > 0 && contentHeight > viewportHeight + OVERFLOW_EPSILON;

		if (!m_bOverflow)
			m_wScroll.SetSliderPos(0, 0);

		UpdateOverflowIndicator();
	}

	protected void UpdateOverflowIndicator()
	{
		if (!m_wOverflowIndicator)
			return;

		float sliderX;
		float sliderY;
		if (m_wScroll)
			m_wScroll.GetSliderPos(sliderX, sliderY);

		m_wOverflowIndicator.SetVisible(m_bOverflow && sliderY < 1.0 - END_EPSILON);
	}
}
