class GC_BriefingTitlesComponent : SCR_ScriptedWidgetComponent
{
    ref ScriptInvoker<GC_BriefingTitleRowComponent> m_OnTitleSelected = new ScriptInvoker<GC_BriefingTitleRowComponent>();

    protected ref array<GC_BriefingTitleRowComponent> m_aRows = {};
    protected int m_iSelectedBriefingOrder;
    protected bool m_bHasConfiguredBriefings;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        RegisterRow("BriefingTitleRow01");
        RegisterRow("BriefingTitleRow02");
        RegisterRow("BriefingTitleRow03");
        RegisterRow("BriefingTitleRow04");
    }

    override void HandlerDeattached(Widget w)
    {
        for (int i = 0; i < m_aRows.Count(); i++)
        {
            if (m_aRows[i])
                m_aRows[i].m_OnClicked.Remove(OnRowClicked);
        }

        m_aRows.Clear();
        super.HandlerDeattached(w);
    }

    protected void RegisterRow(string rowName)
    {
        Widget rowRoot = m_wRoot.FindAnyWidget(rowName);
        if (!rowRoot)
            return;

        GC_BriefingTitleRowComponent row =
            GC_BriefingTitleRowComponent.Cast(rowRoot.FindHandler(GC_BriefingTitleRowComponent));
        if (!row)
            return;

        m_aRows.Insert(row);
        row.m_OnClicked.Insert(OnRowClicked);
    }

    protected void OnRowClicked(SCR_ButtonBaseComponent baseButton)
    {
        GC_BriefingTitleRowComponent selectedRow = GC_BriefingTitleRowComponent.Cast(baseButton);
        if (!selectedRow)
            return;

        bool clearSelection = selectedRow.IsSelected();
        if (clearSelection)
            m_iSelectedBriefingOrder = 0;
        else
            m_iSelectedBriefingOrder = selectedRow.GetBriefingOrder();

        for (int i = 0; i < m_aRows.Count(); i++)
            m_aRows[i].SetSelected(!clearSelection && m_aRows[i] == selectedRow);

        m_OnTitleSelected.Invoke(selectedRow);
    }

    //! Applies the current local audience-filtered snapshot to the authored row capacity.
    GC_BriefingTitleRowComponent ConfigureBriefings(array<ref GC_Briefing> briefings)
    {
        GC_BriefingTitleRowComponent selectedRow;
        bool selectInitialBriefing = !m_bHasConfiguredBriefings;
        m_bHasConfiguredBriefings = true;

        for (int index = 0; index < m_aRows.Count(); index++)
        {
            GC_BriefingTitleRowComponent row = m_aRows[index];
            if (!row)
                continue;

            if (briefings && index < briefings.Count())
            {
                row.SetBriefingData(briefings[index]);
                if (m_iSelectedBriefingOrder > 0 && row.GetBriefingOrder() == m_iSelectedBriefingOrder)
                    selectedRow = row;
            }
            else
            {
                row.ClearBriefingData();
            }
        }

        if (!selectedRow && selectInitialBriefing && briefings && !briefings.IsEmpty() && !m_aRows.IsEmpty())
        {
            selectedRow = m_aRows[0];
            m_iSelectedBriefingOrder = selectedRow.GetBriefingOrder();
        }
        else if (!selectedRow && m_iSelectedBriefingOrder > 0)
        {
            // The selected briefing was hidden, removed, or became unavailable
            // to this faction. Keep the document closed until the player chooses.
            m_iSelectedBriefingOrder = 0;
        }

        for (int rowIndex = 0; rowIndex < m_aRows.Count(); rowIndex++)
            m_aRows[rowIndex].SetSelected(m_aRows[rowIndex] == selectedRow);

        return selectedRow;
    }
}
