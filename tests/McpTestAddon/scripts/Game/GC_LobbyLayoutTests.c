//! Explicit presentation checks on the live production menu; no automatic execution.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Constrain the current menu to an aspect ratio inside the real workspace.
	//! This tests layout bounds, not a display-resolution or DPI setting change.
	string GC_TestLobbyAspect(float aspect)
	{
		Widget root = GC_LobbyManager.Cast(this).GC_TestMenuRoot();
		if (!root || root.GetName() != "rootFrame" || (aspect != 0 && (aspect < 1.3 || aspect > 4)))
			return "REJECTED menu or aspect";
		if (aspect == 0)
		{
			FrameSlot.SetAnchorMin(root, 0, 0);
			FrameSlot.SetAnchorMax(root, 1, 1);
			FrameSlot.SetOffsets(root, 0, 0, 0, 0);
			return "restored full workspace bounds";
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float width, height;
		workspace.GetScreenSize(width, height);
		width = Math.Min(width, height * aspect);
		height = width / aspect;
		FrameSlot.SetAnchorMin(root, 0, 0);
		FrameSlot.SetAnchorMax(root, 0, 0);
		FrameSlot.SetOffsets(root, 0, 0, -workspace.DPIUnscale(width), -workspace.DPIUnscale(height));
		return string.Format("menu bounds requested=%1x%2 aspect=%3; aspect=0 restores bounds", width, height, aspect);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLobbyLayout()
	{
		Widget root = GC_LobbyManager.Cast(this).GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		array<string> names = {"MenuBar", "Title", "PlayerCount", "SlotsSize", "PlayerListPanel", "LoadoutSize", "RowSize", "playerRender", "InventoryBounds"};
		string result;
		foreach (string name : names)
		{
			Widget widget = root.FindAnyWidget(name);
			if (!widget)
				continue;
			float x, y, width, height;
			widget.GetScreenPos(x, y);
			widget.GetScreenSize(width, height);
			result += string.Format("%1 rect=%2,%3,%4,%5 visible=%6\n", name, x, y, width, height, widget.IsVisibleInHierarchy());
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLobbyFactionScroll(float position)
	{
		Widget root = GC_LobbyManager.Cast(this).GC_TestMenuRoot();
		if (!root || position < 0 || position > 1)
			return "REJECTED menu or position";
		Widget content = root.FindAnyWidget("FactionContent");
		if (!content)
			return "UNAVAILABLE faction content";
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(content.GetParent());
		if (!scroll)
			return "UNAVAILABLE faction scroll";
		scroll.SetSliderPos(0, position);
		return "faction scroll dispatched";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLobbySlotHover()
	{
		Widget root = GC_LobbyManager.Cast(this).GC_TestMenuRoot();
		if (!root)
			return "UNAVAILABLE menu";
		Widget slot = root.FindAnyWidget("SlotButton");
		if (!slot)
			return "UNAVAILABLE slot";
		GC_SlotButtonComponent button = GC_SlotButtonComponent.Cast(slot.FindHandler(GC_SlotButtonComponent));
		if (!button)
			return "UNAVAILABLE slot handler";
		button.OnMouseEnter(slot, 0, 0);
		return "production slot hover dispatched";
	}
}
#endif
