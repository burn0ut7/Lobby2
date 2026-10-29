#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Exercises the resolved camera component with native pointer and named zoom input.
	string GC_TestSpectatorScroll(string widgetName, float input)
	{
		GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		GC_SpectatorManualCamera camera = GC_SpectatorManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!menu || !camera)
			return "UNAVAILABLE spectator";
		GC_SpectatorZoomCameraComponent zoom = GC_SpectatorZoomCameraComponent.Cast(camera.FindCameraComponent(SCR_ZoomManualCameraComponent));
		if (!zoom)
			return "UNAVAILABLE custom zoom";
		SCR_BaseManualCameraComponent cursor = camera.FindCameraComponent(GC_SpectatorCursorCameraComponent);
		SCR_BaseManualCameraComponent move = camera.FindCameraComponent(SCR_MoveToCursorManualCameraComponent);
		if (!cursor || !move)
			return "UNAVAILABLE cursor/move";
		float x = 200;
		float y = 200;
		if (widgetName != "world")
		{
			Widget widget = menu.GetRootWidget().FindAnyWidget(widgetName);
			if (!widget)
				return "UNAVAILABLE widget";
			float width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			x += width * 0.5;
			y += height * 0.5;
		}
		int oldX, oldY;
		WidgetManager.GetMousePos(oldX, oldY);
		GetGame().GetInputManager().GetMouseDeviceHandler().SetCursorPosition(x, y);
		WidgetManager.ReportMouse(x, y, menu.GetRootWidget());
		int actualX, actualY;
		WidgetManager.GetMousePos(actualX, actualY);
		vector transform[4];
		camera.GetWorldTransform(transform);
		SCR_ManualCameraParam param = new SCR_ManualCameraParam(transform, 0.1, GetGame().GetWorld(), 70, true, vector.Zero, 0);
		InputManager manager = GetGame().GetInputManager();
		manager.SetActionValue("ManualCameraZoom", input);
		manager.SetActionValue("ManualCameraMoveToCursor", input);
		float actualInput = manager.GetActionValue("ManualCameraZoom");
		cursor.EOnCameraFrame(param);
		move.EOnCameraFrame(param);
		zoom.EOnCameraFrame(param);
		manager.SetActionValue("ManualCameraZoom", 0);
		manager.SetActionValue("ManualCameraMoveToCursor", 0);
		string result = string.Format("widget=%1 blocked=%2 input=%3 fov=%4 moved=%5 cursor=%6", widgetName, menu.IsPointerOverScrollPanel(actualX, actualY), actualInput, param.fov, vector.Distance(transform[3], param.transform[3]), param.isCursorEnabled);
		GetGame().GetInputManager().GetMouseDeviceHandler().SetCursorPosition(oldX, oldY);
		WidgetManager.ReportMouse(oldX, oldY, menu.GetRootWidget());
		zoom.EOnCameraReset();
		return result;
	}
}
#endif
