class GC_BriefingTitlesComponent : SCR_ScriptedWidgetComponent
{
    ref ScriptInvoker<GC_BriefingTitleRowComponent> m_OnTitleSelected = new ScriptInvoker<GC_BriefingTitleRowComponent>();

    protected ref array<GC_BriefingTitleRowComponent> m_aRows = {};

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

        for (int i = 0; i < m_aRows.Count(); i++)
            m_aRows[i].SetSelected(m_aRows[i] == selectedRow);

        m_OnTitleSelected.Invoke(selectedRow);
    }
}
