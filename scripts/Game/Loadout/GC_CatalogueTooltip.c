//! Native hover placement; display contributing addon titles without repeating the item name.
class GC_CatalogueTooltip : SCR_ScriptedWidgetTooltip
{
	//------------------------------------------------------------------------------------------------
	static void HideCurrent()
	{
		GC_CatalogueTooltip current = GC_CatalogueTooltip.Cast(m_CurrentTooltip);
		if (current)
			current.ForceHidden();
	}

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
		Widget hover = GetHoverWidget();
		if (!hover || !m_Preset || !m_Preset.m_Content)
			return;
		TextWidget source = TextWidget.Cast(hover.GetParent().FindAnyWidget("SourceMods"));
		if (!source || source.GetText().IsEmpty())
		{
			ForceHidden();
			return;
		}
		m_Preset.m_Content.SetMessage(source.GetText());
	}
}
