[BaseContainerProps()]
class GC_BriefingMapCursorModule : SCR_MapCursorModule
{
	override protected void OnInputZoomWheelUp(float value, EActionTrigger reason)
	{
		if (IsPointerOverScrollableUI())
			return;

		super.OnInputZoomWheelUp(value, reason);
	}

	override protected void OnInputZoomWheelDown(float value, EActionTrigger reason)
	{
		if (IsPointerOverScrollableUI())
			return;

		super.OnInputZoomWheelDown(value, reason);
	}

	//! The map wheel actions bypass widget event propagation. Test the widgets
	//! beneath the pointer at action time so the block cannot become stale.
	protected bool IsPointerOverScrollableUI()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		array<Widget> widgets = {};
		WidgetManager.TraceWidgets(mouseX, mouseY, workspace, widgets);
		foreach (Widget widget : widgets)
		{
			if (ScrollLayoutWidget.Cast(widget))
				return true;
		}

		return false;
	}
}
