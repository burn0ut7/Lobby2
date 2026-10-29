//! The row owns pointer input; its text children never intercept button clicks.
class GC_SpectatorEventTooltip : SCR_ScriptedWidgetTooltip
{
	//------------------------------------------------------------------------------------------------
	override protected void Clear()
	{
		GetGame().GetCallqueue().Remove(DelayedPositionInit);
		super.Clear();
		if (m_CurrentTooltip == this)
			m_CurrentTooltip = null;
	}

	//------------------------------------------------------------------------------------------------
	override void Show(WorkspaceWidget pWorkspace, Widget pToolTipWidget, float desiredPosX, float desiredPosY)
	{
		super.Show(pWorkspace, pToolTipWidget, desiredPosX, desiredPosY);
		Widget row = GetHoverWidget();
		if (!row || !m_Preset || !m_Preset.m_Content)
			return;
		TextWidget summary = TextWidget.Cast(row.FindAnyWidget("EventSummary"));
		TextWidget info = TextWidget.Cast(row.FindAnyWidget("EventInfo"));
		if (summary && info)
			m_Preset.m_Content.SetMessage(summary.GetText() + "\n" + info.GetText());
	}
}
