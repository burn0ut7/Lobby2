enum GC_ESlideOutDirection
{
	LEFT = 0,
	RIGHT,
	UP,
	DOWN
}

//! Reusable hover-driven container which keeps only its handle exposed while
//! collapsed. The authored root position is the fully expanded position.
class GC_SlideOutComponent : SCR_ScriptedWidgetComponent
{
	protected static const int ANIMATION_TICK_MS = 16;

	[Attribute("0", UIWidgets.ComboBox, "Direction in which the content expands", enumType: GC_ESlideOutDirection)]
	protected GC_ESlideOutDirection m_eDirection;

	[Attribute("56", UIWidgets.EditBox, "Visible handle size along the slide axis")]
	protected float m_fHandleSize;

	[Attribute("112", UIWidgets.EditBox, "Handle length perpendicular to the slide axis")]
	protected float m_fHandleLength;

	[Attribute("180", UIWidgets.EditBox, "Expand and collapse duration in milliseconds")]
	protected int m_iAnimationDurationMs;

	protected float m_fExpandedX;
	protected float m_fExpandedY;
	protected float m_fTravelDistance;
	protected float m_fProgress;
	protected float m_fTargetProgress;
	protected bool m_bHovered;
	protected bool m_bPinned;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_fExpandedX = FrameSlot.GetPosX(w);
		m_fExpandedY = FrameSlot.GetPosY(w);
		ConfigureChildren();

		if (m_eDirection == GC_ESlideOutDirection.LEFT || m_eDirection == GC_ESlideOutDirection.RIGHT)
			m_fTravelDistance = FrameSlot.GetSizeX(w) - m_fHandleSize;
		else
			m_fTravelDistance = FrameSlot.GetSizeY(w) - m_fHandleSize;

		if (m_fTravelDistance < 0)
			m_fTravelDistance = 0;

		m_fProgress = 0;
		m_fTargetProgress = 0;
		m_bHovered = false;
		m_bPinned = false;
		ApplyProgress();
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Animate);
		super.HandlerDeattached(w);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bHovered = true;
		SetExpanded(true);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		Widget enteredWidget = enterW;
		while (enteredWidget)
		{
			if (enteredWidget == m_wRoot)
				return true;

			enteredWidget = enteredWidget.GetParent();
		}

		m_bHovered = false;
		if (!m_bPinned)
			SetExpanded(false);
		return true;
	}

	void TogglePinned()
	{
		m_bPinned = !m_bPinned;
		if (m_bPinned)
			SetExpanded(true);
		else if (!m_bHovered)
			SetExpanded(false);
	}

	protected void ConfigureChildren()
	{
		Widget content = m_wRoot.FindAnyWidget("SlideOutContent");
		Widget handle = m_wRoot.FindAnyWidget("SlideOutHandle");
		TextWidget arrow = TextWidget.Cast(m_wRoot.FindAnyWidget("SlideOutArrow"));
		if (!content || !handle)
		{
			PrintFormat("GC | Slide out attach failed: content=%1 handle=%2", content != null, handle != null, level: LogLevel.ERROR);
			return;
		}

		FrameSlot.SetAnchorMin(content, 0, 0);
		FrameSlot.SetAnchorMax(content, 1, 1);
		FrameSlot.SetAlignment(content, 0, 0);

		switch (m_eDirection)
		{
			case GC_ESlideOutDirection.RIGHT:
				FrameSlot.SetOffsets(content, 0, 0, -m_fHandleSize, 0);
				FrameSlot.SetAnchor(handle, 1, 0.5);
				FrameSlot.SetAlignment(handle, 1, 0.5);
				FrameSlot.SetPos(handle, 0, 0);
				FrameSlot.SetSize(handle, m_fHandleSize, m_fHandleLength);
				if (arrow)
					arrow.SetText(">");
				break;

			case GC_ESlideOutDirection.UP:
				FrameSlot.SetOffsets(content, 0, m_fHandleSize, 0, 0);
				FrameSlot.SetAnchor(handle, 0.5, 0);
				FrameSlot.SetAlignment(handle, 0.5, 0);
				FrameSlot.SetPos(handle, 0, 0);
				FrameSlot.SetSize(handle, m_fHandleLength, m_fHandleSize);
				if (arrow)
					arrow.SetText("^");
				break;

			case GC_ESlideOutDirection.DOWN:
				FrameSlot.SetOffsets(content, 0, 0, 0, -m_fHandleSize);
				FrameSlot.SetAnchor(handle, 0.5, 1);
				FrameSlot.SetAlignment(handle, 0.5, 1);
				FrameSlot.SetPos(handle, 0, 0);
				FrameSlot.SetSize(handle, m_fHandleLength, m_fHandleSize);
				if (arrow)
					arrow.SetText("v");
				break;

			default:
				FrameSlot.SetOffsets(content, m_fHandleSize, 0, 0, 0);
				FrameSlot.SetAnchor(handle, 0, 0.5);
				FrameSlot.SetAlignment(handle, 0, 0.5);
				FrameSlot.SetPos(handle, 0, 0);
				FrameSlot.SetSize(handle, m_fHandleSize, m_fHandleLength);
				if (arrow)
					arrow.SetText("<");
				break;
		}
	}

	protected void SetExpanded(bool expanded)
	{
		if (expanded)
			m_fTargetProgress = 1;
		else
			m_fTargetProgress = 0;

		GetGame().GetCallqueue().Remove(Animate);
		if (m_iAnimationDurationMs <= 0)
		{
			m_fProgress = m_fTargetProgress;
			ApplyProgress();
			return;
		}

		GetGame().GetCallqueue().CallLater(Animate, ANIMATION_TICK_MS, true);
	}

	protected void Animate()
	{
		float step = ANIMATION_TICK_MS * 1.0 / m_iAnimationDurationMs;
		if (m_fTargetProgress > m_fProgress)
		{
			m_fProgress += step;
			if (m_fProgress >= m_fTargetProgress)
				m_fProgress = m_fTargetProgress;
		}
		else
		{
			m_fProgress -= step;
			if (m_fProgress <= m_fTargetProgress)
				m_fProgress = m_fTargetProgress;
		}

		ApplyProgress();
		if (m_fProgress == m_fTargetProgress)
			GetGame().GetCallqueue().Remove(Animate);
	}

	protected void ApplyProgress()
	{
		float hiddenDistance = m_fTravelDistance * (1 - m_fProgress);
		float x = m_fExpandedX;
		float y = m_fExpandedY;

		switch (m_eDirection)
		{
			case GC_ESlideOutDirection.RIGHT:
				x -= hiddenDistance;
				break;

			case GC_ESlideOutDirection.UP:
				y += hiddenDistance;
				break;

			case GC_ESlideOutDirection.DOWN:
				y -= hiddenDistance;
				break;

			default:
				x += hiddenDistance;
				break;
		}

		FrameSlot.SetPos(m_wRoot, x, y);
	}
}

//! Button adapter which keeps pin intent scoped to the slide-out handle.
class GC_SlideOutHandleComponent : SCR_ScriptedWidgetComponent
{
	protected GC_SlideOutComponent m_SlideOut;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		Widget parent = w.GetParent();
		while (parent && !m_SlideOut)
		{
			m_SlideOut = GC_SlideOutComponent.Cast(parent.FindHandler(GC_SlideOutComponent));
			parent = parent.GetParent();
		}
	}

	override void HandlerDeattached(Widget w)
	{
		m_SlideOut = null;
		super.HandlerDeattached(w);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_SlideOut)
			return false;

		m_SlideOut.TogglePinned();
		return true;
	}
}
