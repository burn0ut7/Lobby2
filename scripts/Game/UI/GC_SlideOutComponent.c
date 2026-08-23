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
	protected bool m_bContentInteractive = true;
	protected Widget m_wContent;

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
		m_wContent = null;
		super.HandlerDeattached(w);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		SetHandleHovered(true);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!ContainsWidget(enterW))
			SetHandleHovered(false);
		return true;
	}

	void SetHandleHovered(bool hovered)
	{
		m_bHovered = hovered;
		if (hovered)
			SetExpanded(true);
		else if (!m_bPinned)
			SetExpanded(false);
	}

	bool ContainsWidget(Widget widget)
	{
		while (widget)
		{
			if (widget == m_wRoot)
				return true;

			widget = widget.GetParent();
		}

		return false;
	}

	void TogglePinned()
	{
		m_bPinned = !m_bPinned;
		if (m_bPinned)
			SetExpanded(true);
		else if (!m_bHovered)
			SetExpanded(false);
	}

	bool IsPinned()
	{
		return m_bPinned;
	}

	protected void ConfigureChildren()
	{
		Widget content = m_wRoot.FindAnyWidget("SlideOutContent");
		Widget handle = m_wRoot.FindAnyWidget("SlideOutHandle");
		Widget handleBackground = m_wRoot.FindAnyWidget("SlideOutHandleRoundedBase");
		TextWidget arrow = TextWidget.Cast(m_wRoot.FindAnyWidget("SlideOutArrow"));
		if (!content || !handle)
		{
			PrintFormat("GC | Slide out attach failed: content=%1 handle=%2", content != null, handle != null, level: LogLevel.ERROR);
			return;
		}
		m_wContent = content;

		FrameSlot.SetAnchorMin(content, 0, 0);
		FrameSlot.SetAnchorMax(content, 1, 1);
		FrameSlot.SetAlignment(content, 0, 0);
		if (handleBackground)
		{
			GC_SlideOutHandleBackgroundComponent backgroundComponent = GC_SlideOutHandleBackgroundComponent.Cast(handleBackground.FindHandler(GC_SlideOutHandleBackgroundComponent));
			if (backgroundComponent)
				backgroundComponent.SetDirection(m_eDirection);
		}

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

		bool contentInteractive = m_fProgress > 0;
		if (m_wContent && contentInteractive != m_bContentInteractive)
		{
			m_bContentInteractive = contentInteractive;
			m_wContent.SetEnabled(contentInteractive);
		}
	}
}

//! Draws the opaque rounded base used inside the handle's translucent visual group.
class GC_SlideOutHandleBackgroundComponent : SCR_ScriptedWidgetComponent
{
	protected static const float CORNER_RADIUS = 16;

	protected CanvasWidget m_wCanvas;
	protected ref array<float> m_aBodyVertices = {};
	protected ref array<float> m_aMiddleVertices = {};
	protected ref array<float> m_aFirstCornerVertices = {};
	protected ref array<float> m_aSecondCornerVertices = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};
	protected int m_iColor = 0xff000000;
	protected GC_ESlideOutDirection m_eDirection;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wCanvas = CanvasWidget.Cast(w);
		if (m_wCanvas)
			GetGame().GetCallqueue().CallLater(Redraw, 0);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Redraw);
		m_wCanvas = null;
		super.HandlerDeattached(w);
	}

	protected void Redraw()
	{
		if (!m_wCanvas)
			return;

		vector canvasSize = m_wCanvas.GetSizeInUnits();
		if (canvasSize[0] <= 0 || canvasSize[1] <= 0)
		{
			GetGame().GetCallqueue().CallLater(Redraw, 0);
			return;
		}

		m_aBodyVertices = {};
		m_aMiddleVertices = {};
		m_aFirstCornerVertices = {};
		m_aSecondCornerVertices = {};
		m_aDrawCommands = {};

		switch (m_eDirection)
		{
			case GC_ESlideOutDirection.RIGHT:
				TessellateRectangle(Vector(0, 0, 0), Vector(canvasSize[0] - CORNER_RADIUS, canvasSize[1], 0), m_aBodyVertices);
				TessellateRectangle(Vector(0, CORNER_RADIUS, 0), Vector(canvasSize[0], canvasSize[1] - CORNER_RADIUS, 0), m_aMiddleVertices);
				m_wCanvas.TessellateCircle(Vector(canvasSize[0] - CORNER_RADIUS, CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aFirstCornerVertices);
				m_wCanvas.TessellateCircle(Vector(canvasSize[0] - CORNER_RADIUS, canvasSize[1] - CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aSecondCornerVertices);
				break;

			case GC_ESlideOutDirection.UP:
				TessellateRectangle(Vector(0, CORNER_RADIUS, 0), Vector(canvasSize[0], canvasSize[1], 0), m_aBodyVertices);
				TessellateRectangle(Vector(CORNER_RADIUS, 0, 0), Vector(canvasSize[0] - CORNER_RADIUS, canvasSize[1], 0), m_aMiddleVertices);
				m_wCanvas.TessellateCircle(Vector(CORNER_RADIUS, CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aFirstCornerVertices);
				m_wCanvas.TessellateCircle(Vector(canvasSize[0] - CORNER_RADIUS, CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aSecondCornerVertices);
				break;

			case GC_ESlideOutDirection.DOWN:
				TessellateRectangle(Vector(0, 0, 0), Vector(canvasSize[0], canvasSize[1] - CORNER_RADIUS, 0), m_aBodyVertices);
				TessellateRectangle(Vector(CORNER_RADIUS, 0, 0), Vector(canvasSize[0] - CORNER_RADIUS, canvasSize[1], 0), m_aMiddleVertices);
				m_wCanvas.TessellateCircle(Vector(CORNER_RADIUS, canvasSize[1] - CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aFirstCornerVertices);
				m_wCanvas.TessellateCircle(Vector(canvasSize[0] - CORNER_RADIUS, canvasSize[1] - CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aSecondCornerVertices);
				break;

			default:
				TessellateRectangle(Vector(CORNER_RADIUS, 0, 0), Vector(canvasSize[0], canvasSize[1], 0), m_aBodyVertices);
				TessellateRectangle(Vector(0, CORNER_RADIUS, 0), Vector(canvasSize[0], canvasSize[1] - CORNER_RADIUS, 0), m_aMiddleVertices);
				m_wCanvas.TessellateCircle(Vector(CORNER_RADIUS, CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aFirstCornerVertices);
				m_wCanvas.TessellateCircle(Vector(CORNER_RADIUS, canvasSize[1] - CORNER_RADIUS, 0), CORNER_RADIUS, 16, m_aSecondCornerVertices);
				break;
		}

		AddShape(m_aBodyVertices);
		AddShape(m_aMiddleVertices);
		AddShape(m_aFirstCornerVertices);
		AddShape(m_aSecondCornerVertices);
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected void TessellateRectangle(vector mins, vector maxs, out notnull array<float> vertices)
	{
		m_wCanvas.TessellateRoundedRectangle(mins, maxs, 0, 1, RectangleCorner.All, vertices);
	}

	protected void AddShape(array<float> vertices)
	{
		PolygonDrawCommand shape = new PolygonDrawCommand();
		shape.m_iColor = m_iColor;
		shape.m_Vertices = vertices;
		m_aDrawCommands.Insert(shape);
	}

	void SetDirection(GC_ESlideOutDirection direction)
	{
		m_eDirection = direction;
		Redraw();
	}

	void SetBackgroundColor(int color)
	{
		m_iColor = color;
		Redraw();
	}
}

//! Button adapter which keeps pin intent scoped to the slide-out handle.
class GC_SlideOutHandleComponent : SCR_ScriptedWidgetComponent
{
	protected GC_SlideOutComponent m_SlideOut;
	protected GC_SlideOutHandleBackgroundComponent m_BackgroundComponent;
	protected Widget m_wHandleBackground;
	protected TextWidget m_wArrow;
	protected bool m_bHovered;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		Widget parent = w.GetParent();
		while (parent && !m_SlideOut)
		{
			m_SlideOut = GC_SlideOutComponent.Cast(parent.FindHandler(GC_SlideOutComponent));
			parent = parent.GetParent();
		}

		Widget handle = w;
		if (handle)
		{
			Widget roundedBase = handle.FindAnyWidget("SlideOutHandleRoundedBase");
			if (roundedBase)
				m_BackgroundComponent = GC_SlideOutHandleBackgroundComponent.Cast(roundedBase.FindHandler(GC_SlideOutHandleBackgroundComponent));

			m_wHandleBackground = handle.FindAnyWidget("SlideOutHandleBackground");
			m_wArrow = TextWidget.Cast(handle.FindAnyWidget("SlideOutArrow"));
		}

		UpdateVisuals();
	}

	override void HandlerDeattached(Widget w)
	{
		m_BackgroundComponent = null;
		m_wHandleBackground = null;
		m_wArrow = null;
		m_SlideOut = null;
		super.HandlerDeattached(w);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bHovered = true;
		if (m_SlideOut)
			m_SlideOut.SetHandleHovered(true);
		UpdateVisuals();
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bHovered = false;
		if (m_SlideOut && !m_SlideOut.ContainsWidget(enterW))
			m_SlideOut.SetHandleHovered(false);
		UpdateVisuals();
		return true;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_SlideOut)
			return false;

		m_SlideOut.TogglePinned();
		UpdateVisuals();
		return true;
	}

	protected void UpdateVisuals()
	{
		bool pinned = m_SlideOut && m_SlideOut.IsPinned();
		int backgroundColor = 0xff000000;
		float backgroundOpacity = 0.42;
		Color arrowColor = Color.FromRGBA(255, 255, 255, 255);

		if (pinned)
		{
			backgroundColor = 0xffffc440;
			backgroundOpacity = 0.9;
			arrowColor = Color.FromRGBA(12, 12, 10, 255);
		}
		else if (m_bHovered)
		{
			backgroundOpacity = 0.7;
			arrowColor = Color.FromRGBA(255, 196, 64, 255);
		}

		if (m_BackgroundComponent)
			m_BackgroundComponent.SetBackgroundColor(backgroundColor);
		if (m_wHandleBackground)
			m_wHandleBackground.SetOpacity(backgroundOpacity);
		if (m_wArrow)
			m_wArrow.SetColor(arrowColor);
	}
}
