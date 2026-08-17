//! Owns briefing-region wheel input so it cannot propagate to the map beneath it.
//! Overflow state is derived from the laid-out viewport and content sizes.
class GC_BriefingScrollComponent : SCR_ScriptedWidgetComponent
{
	[Attribute("BriefingSectionContentSize", UIWidgets.EditBox, "Widget whose laid-out height determines whether this region overflows")]
	protected string m_sContentWidgetName;

	[Attribute("BriefingOverflowIndicator", UIWidgets.EditBox, "Indicator shown while more content remains below this region")]
	protected string m_sOverflowIndicatorWidgetName;

	[Attribute("", UIWidgets.EditBox, "Optional scrollbar track shown only while this region overflows")]
	protected string m_sScrollbarTrackWidgetName;

	[Attribute("", UIWidgets.EditBox, "Optional scrollbar thumb positioned from the current scroll amount")]
	protected string m_sScrollbarThumbWidgetName;

	protected static const float SCROLL_STEP = 64;
	protected static const float OVERFLOW_EPSILON = 1;
	protected static const float END_EPSILON = 0.001;
	protected static const float SCROLLBAR_WIDTH = 5;
	protected static const float SCROLLBAR_RIGHT_INSET = 3;
	protected static const float MIN_THUMB_HEIGHT = 36;

	protected ScrollLayoutWidget m_wScroll;
	protected Widget m_wContent;
	protected Widget m_wOverflowIndicator;
	protected Widget m_wScrollbarTrack;
	protected Widget m_wScrollbarThumb;
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
		{
			m_wOverflowIndicator = root.FindAnyWidget(m_sOverflowIndicatorWidgetName);
			if (!m_sScrollbarTrackWidgetName.IsEmpty())
				m_wScrollbarTrack = root.FindAnyWidget(m_sScrollbarTrackWidgetName);
			if (!m_sScrollbarThumbWidgetName.IsEmpty())
				m_wScrollbarThumb = root.FindAnyWidget(m_sScrollbarThumbWidgetName);
		}

		RefreshOverflow();
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(UpdateOverflowState);
		m_wScroll = null;
		m_wContent = null;
		m_wOverflowIndicator = null;
		m_wScrollbarTrack = null;
		m_wScrollbarThumb = null;
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

		// Always consume wheel input over a briefing region. Even without overflow,
		// it must not reach and zoom the map underneath the panel.
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
		UpdateScrollbar();
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
		UpdateScrollbar(viewportWidth, viewportHeight, contentHeight);
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

	protected void UpdateScrollbar(float viewportWidth = -1, float viewportHeight = -1, float contentHeight = -1)
	{
		if (!m_wScrollbarTrack || !m_wScrollbarThumb || !m_wScroll)
			return;

		m_wScrollbarTrack.SetVisible(m_bOverflow);
		m_wScrollbarThumb.SetVisible(m_bOverflow);
		if (!m_bOverflow)
			return;

		if (viewportHeight < 0)
		{
			m_wScroll.GetScreenSize(viewportWidth, viewportHeight);
			float contentWidth;
			m_wContent.GetScreenSize(contentWidth, contentHeight);
		}

		float viewportX;
		float viewportY;
		float layerX;
		float layerY;
		m_wScroll.GetScreenPos(viewportX, viewportY);
		Widget layer = m_wScrollbarTrack.GetParent();
		if (layer)
			layer.GetScreenPos(layerX, layerY);

		float trackX = viewportX - layerX + viewportWidth - SCROLLBAR_WIDTH - SCROLLBAR_RIGHT_INSET;
		float trackY = viewportY - layerY;
		float thumbHeight = Math.Max(MIN_THUMB_HEIGHT, viewportHeight * viewportHeight / contentHeight);
		thumbHeight = Math.Min(thumbHeight, viewportHeight);

		float sliderX;
		float sliderY;
		m_wScroll.GetSliderPos(sliderX, sliderY);
		float thumbY = trackY + sliderY * (viewportHeight - thumbHeight);

		FrameSlot.SetPos(m_wScrollbarTrack, trackX, trackY);
		FrameSlot.SetSize(m_wScrollbarTrack, SCROLLBAR_WIDTH, viewportHeight);
		FrameSlot.SetPos(m_wScrollbarThumb, trackX, thumbY);
		FrameSlot.SetSize(m_wScrollbarThumb, SCROLLBAR_WIDTH, thumbHeight);
	}
}
