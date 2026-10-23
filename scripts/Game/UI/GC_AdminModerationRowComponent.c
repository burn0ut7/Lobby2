//! Preserve native left-click selection; right-click opens the shared context menu.
class GC_AdminModerationRowComponent : SCR_ButtonBaseComponent
{
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 1)
			return super.OnClick(w, x, y, button);
		Widget parent = m_wRoot.GetParent();
		while (parent)
		{
			GC_AdminModerationComponent form = GC_AdminModerationComponent.Cast(parent.FindHandler(GC_AdminModerationComponent));
			if (form)
				return form.OpenPlayerContext(this);
			parent = parent.GetParent();
		}
		return false;
	}
}
