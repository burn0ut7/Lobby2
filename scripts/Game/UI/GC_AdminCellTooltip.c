//! Native tooltip lifecycle and placement, populated from the hovered table cell.
class GC_AdminCellTooltip : SCR_ScriptedWidgetTooltip
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
		TextWidget cell = TextWidget.Cast(GetHoverWidget());
		if (!cell || !m_Preset || !m_Preset.m_Content)
		{
			ForceHidden();
			return;
		}

		float textWidth, textHeight, cellWidth, cellHeight;
		cell.GetTextSize(textWidth, textHeight);
		cell.GetScreenSize(cellWidth, cellHeight);
		if (textWidth <= pWorkspace.DPIUnscale(cellWidth) + 1
			&& textHeight <= pWorkspace.DPIUnscale(cellHeight) + 1)
		{
			ForceHidden();
			return;
		}

		m_Preset.m_Content.SetMessage(cell.GetText());
	}
}

//! Plain text preserves names and log values without interpreting rich-text markup.
[BaseContainerProps(configRoot: true)]
class GC_AdminCellTooltipContent : SCR_ScriptedWidgetTooltipContentBase
{
	//------------------------------------------------------------------------------------------------
	override bool SetMessage(string message)
	{
		if (!m_wContentRoot)
			return false;

		TextWidget text = TextWidget.Cast(m_wContentRoot.FindAnyWidget("Message"));
		if (!text)
			return false;

		text.SetText(message);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override string GetMessage()
	{
		if (!m_wContentRoot)
			return string.Empty;

		TextWidget text = TextWidget.Cast(m_wContentRoot.FindAnyWidget("Message"));
		if (!text)
			return string.Empty;

		return text.GetText();
	}
}
