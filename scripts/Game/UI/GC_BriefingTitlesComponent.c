class GC_BriefingTitlesComponent : SCR_ScriptedWidgetComponent
{
    protected static const ResourceName TITLE_ROW_LAYOUT = "{6A4B000000000001}UI/layouts/Briefing/Lobby_Briefing_Title_Row.layout";
    protected static const float BOTTOM_MARGIN = 14;
    protected static const float FIT_PADDING = 2;

    ref ScriptInvoker<GC_BriefingTitleRowComponent> m_OnTitleSelected = new ScriptInvoker<GC_BriefingTitleRowComponent>();

    protected ref array<GC_BriefingTitleRowComponent> m_aRows = {};
    protected int m_iSelectedBriefingOrder;
    protected SizeLayoutWidget m_wViewportSize;
    protected ScrollLayoutWidget m_wScroll;
    protected GC_BriefingScrollComponent m_ScrollComponent;
    protected Widget m_wBriefingView;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        RegisterRow("BriefingTitleRow01");
        RegisterRow("BriefingTitleRow02");
        RegisterRow("BriefingTitleRow03");
        RegisterRow("BriefingTitleRow04");

        m_wScroll = ScrollLayoutWidget.Cast(w.GetParent());
        if (m_wScroll)
        {
            m_wViewportSize = SizeLayoutWidget.Cast(m_wScroll.GetParent());
            m_ScrollComponent = GC_BriefingScrollComponent.Cast(
                m_wScroll.FindHandler(GC_BriefingScrollComponent));
        }

        m_wBriefingView = w;
        while (m_wBriefingView && m_wBriefingView.GetName() != "BriefingView")
            m_wBriefingView = m_wBriefingView.GetParent();

        ScheduleLayoutRefresh();
    }

    override void HandlerDeattached(Widget w)
    {
        GetGame().GetCallqueue().Remove(UpdateViewportHeight);
        for (int i = 0; i < m_aRows.Count(); i++)
        {
            if (m_aRows[i])
                m_aRows[i].m_OnClicked.Remove(OnRowClicked);
        }

        m_aRows.Clear();
        m_wViewportSize = null;
        m_wScroll = null;
        m_ScrollComponent = null;
        m_wBriefingView = null;
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

        RegisterRowComponent(row);
    }

    protected void RegisterRowComponent(GC_BriefingTitleRowComponent row)
    {
        if (!row || m_aRows.Contains(row))
            return;

        m_aRows.Insert(row);
        row.m_OnClicked.Insert(OnRowClicked);
    }

    protected void EnsureRowCapacity(int requiredCount)
    {
        WorkspaceWidget workspace = GetGame().GetWorkspace();
        if (!workspace || !m_wRoot)
            return;

        while (m_aRows.Count() < requiredCount)
        {
            Widget rowRoot = workspace.CreateWidgets(TITLE_ROW_LAYOUT, m_wRoot);
            if (!rowRoot)
                return;

            GC_BriefingTitleRowComponent row = GC_BriefingTitleRowComponent.Cast(
                rowRoot.FindHandler(GC_BriefingTitleRowComponent));
            if (!row)
            {
                rowRoot.RemoveFromHierarchy();
                return;
            }

            RegisterRowComponent(row);
        }
    }

    protected void ScheduleLayoutRefresh()
    {
        GetGame().GetCallqueue().Remove(UpdateViewportHeight);
        GetGame().GetCallqueue().CallLater(UpdateViewportHeight, 0);
    }

    protected void UpdateViewportHeight()
    {
        if (!m_wRoot || !m_wScroll || !m_wViewportSize || !m_wBriefingView)
            return;

        float contentWidth;
        float contentHeight;
        float viewX;
        float viewY;
        float viewWidth;
        float viewHeight;
        float scrollX;
        float scrollY;
        m_wRoot.GetScreenSize(contentWidth, contentHeight);
        m_wBriefingView.GetScreenPos(viewX, viewY);
        m_wBriefingView.GetScreenSize(viewWidth, viewHeight);
        m_wScroll.GetScreenPos(scrollX, scrollY);

        float maxViewportHeight = viewY + viewHeight - scrollY;
        WorkspaceWidget workspace = GetGame().GetWorkspace();
        if (workspace)
            maxViewportHeight -= workspace.DPIScale(BOTTOM_MARGIN);
        else
            maxViewportHeight -= BOTTOM_MARGIN;

        if (maxViewportHeight < 1)
            return;

        // A viewport that is exactly the measured content height can round down by
        // a physical pixel when converted back to layout units. ScrollLayout then
        // treats the list as overflowing and draws a full-height white scrollbar.
        // Keep a tiny fit allowance only when all rows already fit; truly
        // overflowing lists remain clamped to the available height.
        float viewportHeight;
        if (contentHeight <= maxViewportHeight)
        {
            viewportHeight = contentHeight + FIT_PADDING;
            if (workspace)
                viewportHeight = contentHeight + workspace.DPIScale(FIT_PADDING);
        }
        else
        {
            viewportHeight = maxViewportHeight;
        }

        if (viewportHeight < 1)
            viewportHeight = 1;
        if (workspace)
            viewportHeight = workspace.DPIUnscale(viewportHeight);

        m_wViewportSize.SetHeightOverride(viewportHeight);
        if (m_ScrollComponent)
            m_ScrollComponent.RefreshOverflow();
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

    //! Applies the current local audience-filtered snapshot, adding reusable rows as needed.
    GC_BriefingTitleRowComponent ConfigureBriefings(array<ref GC_Briefing> briefings)
    {
        GC_BriefingTitleRowComponent selectedRow;

        if (briefings)
            EnsureRowCapacity(briefings.Count());

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

        if (!selectedRow && m_iSelectedBriefingOrder > 0)
        {
            // The selected briefing was hidden, removed, or became unavailable
            // to this faction. Keep the document closed until the player chooses.
            m_iSelectedBriefingOrder = 0;
        }

        for (int rowIndex = 0; rowIndex < m_aRows.Count(); rowIndex++)
            m_aRows[rowIndex].SetSelected(m_aRows[rowIndex] == selectedRow);

        ScheduleLayoutRefresh();

        return selectedRow;
    }
}
