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
	protected static const int HOVER_TICK_MS = 16;

	[Attribute("0", UIWidgets.ComboBox, "Direction in which the content expands", enumType: GC_ESlideOutDirection)]
	protected GC_ESlideOutDirection m_eDirection;

	[Attribute("56", UIWidgets.EditBox, "Visible handle size along the slide axis")]
	protected float m_fHandleSize;

	[Attribute("112", UIWidgets.EditBox, "Handle length perpendicular to the slide axis")]
	protected float m_fHandleLength;

	[Attribute("180", UIWidgets.EditBox, "Expand and collapse duration in milliseconds")]
	protected int m_iAnimationDurationMs;

	[Attribute("5", UIWidgets.EditBox, "Distance in handle widths over which the handle fades in")]
	protected float m_fHandleRevealDistanceWidths;

	[Attribute("2", UIWidgets.EditBox, "Distance in handle widths at which the handle is fully visible")]
	protected float m_fHandleFullRevealDistanceWidths;

	[Attribute("220", UIWidgets.EditBox, "Handle fade duration in milliseconds")]
	protected int m_iHandleFadeDurationMs;

	protected float m_fExpandedX;
	protected float m_fExpandedY;
	protected float m_fTravelDistance;
	protected float m_fProgress;
	protected float m_fTargetProgress;
	protected float m_fDisplayedHandleOpacity;
	protected bool m_bHovered;
	protected bool m_bPinned;
	protected bool m_bHandleFocused;
	protected bool m_bContentInteractive = true;
	protected Widget m_wContent;
	protected Widget m_wHandle;
	protected GC_SlideOutHandleComponent m_HandleComponent;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	protected void Initialize()
	{
		if (!m_wRoot || !FrameWidget.Cast(m_wRoot.GetParent()))
		{
			Print("GC | Slide out initialization failed: root requires a parent FrameWidget", level: LogLevel.ERROR);
			return;
		}

		m_fExpandedX = FrameSlot.GetPosX(m_wRoot);
		m_fExpandedY = FrameSlot.GetPosY(m_wRoot);
		ConfigureChildren();

		if (m_eDirection == GC_ESlideOutDirection.LEFT || m_eDirection == GC_ESlideOutDirection.RIGHT)
			m_fTravelDistance = FrameSlot.GetSizeX(m_wRoot) - m_fHandleSize;
		else
			m_fTravelDistance = FrameSlot.GetSizeY(m_wRoot) - m_fHandleSize;

		if (m_fTravelDistance < 0)
			m_fTravelDistance = 0;

		m_fProgress = 0;
		m_fTargetProgress = 0;
		m_fDisplayedHandleOpacity = 0;
		m_bHovered = false;
		m_bPinned = false;
		m_bHandleFocused = false;
		ApplyProgress();

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
			inputManager.AddActionListener("MouseLeft", EActionTrigger.DOWN, OnMouseLeftDown);

		GetGame().GetCallqueue().CallLater(UpdatePointerHover, HOVER_TICK_MS, true);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Initialize);

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
			inputManager.RemoveActionListener("MouseLeft", EActionTrigger.DOWN, OnMouseLeftDown);

		GetGame().GetCallqueue().Remove(Animate);
		GetGame().GetCallqueue().Remove(UpdatePointerHover);
		m_HandleComponent = null;
		m_wHandle = null;
		m_wContent = null;
		super.HandlerDeattached(w);
	}

	void SetHandleFocused(bool focused)
	{
		m_bHandleFocused = focused;
		UpdatePointerHover();
	}

	protected void SetHovered(bool hovered)
	{
		if (m_bHovered == hovered)
			return;

		m_bHovered = hovered;
		if (hovered)
			SetExpanded(true);
		else if (!m_bPinned)
			SetExpanded(false);
	}

	protected void UpdatePointerHover()
	{
		if (!m_wHandle)
			return;

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		bool handleHovered = IsPointerInside(m_wHandle, mouseX, mouseY);
		bool contentHovered = m_fProgress > 0 && IsPointerInside(m_wContent, mouseX, mouseY);
		SetHovered(handleHovered || contentHovered || m_bHandleFocused);

		if (!m_HandleComponent)
			m_HandleComponent = GC_SlideOutHandleComponent.Cast(m_wHandle.FindHandler(GC_SlideOutHandleComponent));

		if (m_HandleComponent)
		{
			float handleOpacity = GetPointerProximity(m_wHandle, mouseX, mouseY);
			if (m_fProgress > 0 || m_fTargetProgress > 0 || m_bPinned || m_bHandleFocused)
				handleOpacity = 1;
			UpdateHandleOpacity(handleOpacity);
			m_HandleComponent.SetHovered(handleHovered || m_bHandleFocused);
		}
	}

	protected void UpdateHandleOpacity(float targetOpacity)
	{
		if (m_iHandleFadeDurationMs <= 0)
		{
			m_fDisplayedHandleOpacity = targetOpacity;
		}
		else
		{
			float opacityStep = HOVER_TICK_MS * 1.0 / m_iHandleFadeDurationMs;
			if (targetOpacity > m_fDisplayedHandleOpacity)
			{
				m_fDisplayedHandleOpacity += opacityStep;
				if (m_fDisplayedHandleOpacity > targetOpacity)
					m_fDisplayedHandleOpacity = targetOpacity;
			}
			else if (targetOpacity < m_fDisplayedHandleOpacity)
			{
				m_fDisplayedHandleOpacity -= opacityStep;
				if (m_fDisplayedHandleOpacity < targetOpacity)
					m_fDisplayedHandleOpacity = targetOpacity;
			}
		}

		m_HandleComponent.SetProximityOpacity(m_fDisplayedHandleOpacity);
	}

	protected void OnMouseLeftDown()
	{
		if (!m_wRoot.IsVisible() || !m_wHandle || !m_wHandle.IsVisible())
			return;

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		if (!IsPointerInside(m_wHandle, mouseX, mouseY))
			return;

		TogglePinned();
		if (m_HandleComponent)
			m_HandleComponent.RefreshVisuals();

	#ifdef WORKBENCH
		PrintFormat("GC | Slide out | knob pressed: pinned=%1", m_bPinned);
	#endif
	}

	protected float GetPointerProximity(Widget widget, int mouseX, int mouseY)
	{
		if (!widget || m_fHandleRevealDistanceWidths <= 0)
			return 0;

		float widgetX;
		float widgetY;
		float widgetWidth;
		float widgetHeight;
		widget.GetScreenPos(widgetX, widgetY);
		widget.GetScreenSize(widgetWidth, widgetHeight);

		float distanceX;
		if (mouseX < widgetX)
			distanceX = widgetX - mouseX;
		else if (mouseX > widgetX + widgetWidth)
			distanceX = mouseX - widgetX - widgetWidth;

		float distanceY;
		if (mouseY < widgetY)
			distanceY = widgetY - mouseY;
		else if (mouseY > widgetY + widgetHeight)
			distanceY = mouseY - widgetY - widgetHeight;

		float revealDistance = m_fHandleSize * m_fHandleRevealDistanceWidths;
		float fullRevealDistance = m_fHandleSize * m_fHandleFullRevealDistanceWidths;
		if (fullRevealDistance < 0)
			fullRevealDistance = 0;
		float distance = Math.Sqrt(distanceX * distanceX + distanceY * distanceY);
		if (distance >= revealDistance)
			return 0;
		if (distance <= fullRevealDistance)
			return 1;

		float opacity = 1 - (distance - fullRevealDistance) / (revealDistance - fullRevealDistance);
		return opacity * opacity * (3 - 2 * opacity);
	}

	protected bool IsPointerInside(Widget widget, int mouseX, int mouseY)
	{
		if (!widget)
			return false;

		float widgetX;
		float widgetY;
		float widgetWidth;
		float widgetHeight;
		widget.GetScreenPos(widgetX, widgetY);
		widget.GetScreenSize(widgetWidth, widgetHeight);
		return mouseX >= widgetX && mouseX < widgetX + widgetWidth
			&& mouseY >= widgetY && mouseY < widgetY + widgetHeight;
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
		m_wHandle = handle;
		m_HandleComponent = GC_SlideOutHandleComponent.Cast(handle.FindHandler(GC_SlideOutHandleComponent));

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
	protected ref array<float> m_aVertices = {};
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

		m_aVertices = {};
		m_aDrawCommands = {};
		RectangleCorner roundedCorners;

		switch (m_eDirection)
		{
			case GC_ESlideOutDirection.RIGHT:
				roundedCorners = RectangleCorner.TopRight | RectangleCorner.BottomRight;
				break;

			case GC_ESlideOutDirection.UP:
				roundedCorners = RectangleCorner.Top;
				break;

			case GC_ESlideOutDirection.DOWN:
				roundedCorners = RectangleCorner.Bottom;
				break;

			default:
				roundedCorners = RectangleCorner.TopLeft | RectangleCorner.BottomLeft;
				break;
		}

		m_wCanvas.TessellateRoundedRectangle(
			Vector(0, 0, 0),
			Vector(canvasSize[0], canvasSize[1], 0),
			CORNER_RADIUS,
			16,
			roundedCorners,
			m_aVertices
		);
		AddShape(m_aVertices);
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
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
		if (m_iColor == color)
			return;

		m_iColor = color;
		Redraw();
	}

}

//! Button adapter which keeps pin intent scoped to the slide-out handle.
class GC_SlideOutHandleComponent : SCR_ButtonBaseComponent
{
	protected GC_SlideOutComponent m_SlideOut;
	protected GC_SlideOutHandleBackgroundComponent m_BackgroundComponent;
	protected TextWidget m_wArrow;
	protected bool m_bHovered;
	protected float m_fProximityOpacity;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		SetHoverSound(string.Empty);
		SetClickedSound(string.Empty);
		SetToggleable(false);
		m_bUseColorization = false;

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

			m_wArrow = TextWidget.Cast(handle.FindAnyWidget("SlideOutArrow"));
		}

		UpdateVisuals();
	}

	override void HandlerDeattached(Widget w)
	{
		m_BackgroundComponent = null;
		m_wArrow = null;
		m_SlideOut = null;
		super.HandlerDeattached(w);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		return super.OnMouseLeave(w, enterW, x, y);
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		if (m_SlideOut)
			m_SlideOut.SetHandleFocused(true);
		return super.OnFocus(w, x, y);
	}

	override bool OnFocusLost(Widget w, int x, int y)
	{
		if (m_SlideOut)
			m_SlideOut.SetHandleFocused(false);
		return super.OnFocusLost(w, x, y);
	}

	void SetProximityOpacity(float opacity)
	{
		m_fProximityOpacity = opacity;
		UpdateVisuals();
	}

	void SetHovered(bool hovered)
	{
		if (m_bHovered == hovered)
			return;

		m_bHovered = hovered;
		UpdateVisuals();
	}

	protected void UpdateVisuals()
	{
		bool pinned = m_SlideOut && m_SlideOut.IsPinned();
		float backgroundOpacity = 0.38;
		int backgroundRed;
		int backgroundGreen;
		int backgroundBlue;
		Color arrowColor = Color.FromRGBA(255, 255, 255, 255);

		if (pinned)
		{
			backgroundRed = 255;
			backgroundGreen = 196;
			backgroundBlue = 64;
			backgroundOpacity = 0.82;
			arrowColor = Color.FromRGBA(12, 12, 10, 255);
		}
		else if (m_bHovered)
		{
			arrowColor = Color.FromRGBA(255, 196, 64, 255);
		}

		if (m_BackgroundComponent)
			m_BackgroundComponent.SetBackgroundColor(Color.FromRGBA(
				backgroundRed,
				backgroundGreen,
				backgroundBlue,
				Math.Round(backgroundOpacity * m_fProximityOpacity * 255)
			).PackToInt());
		if (m_wArrow)
		{
			m_wArrow.SetColor(arrowColor);
			m_wArrow.SetOpacity(m_fProximityOpacity);
		}
	}

	void RefreshVisuals()
	{
		UpdateVisuals();
	}
}
