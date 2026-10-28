//! Preserve native left-click selection; right-click opens the shared context menu.
class GC_AdminModerationRowComponent : SCR_ScriptedWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 1)
			return false;
		Widget parent = m_wRoot.GetParent();
		while (parent)
		{
			GC_AdminModerationComponent form = GC_AdminModerationComponent.Cast(parent.FindHandler(GC_AdminModerationComponent));
			if (form)
				return form.OpenPlayerContext(SCR_ButtonBaseComponent.Cast(m_wRoot.FindHandler(SCR_ButtonBaseComponent)));
			parent = parent.GetParent();
		}
		return false;
	}
}
