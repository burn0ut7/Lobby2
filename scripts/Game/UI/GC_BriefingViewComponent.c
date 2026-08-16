class GC_BriefingViewComponent : SCR_ScriptedWidgetComponent
{
    protected static const float COMPACT_WIDTH = 244;
    protected static const float EXPANDED_WIDTH = 720;

    protected GC_BriefingTitlesComponent m_Titles;
    protected GC_LobbyManager m_LobbyManager;
    protected SizeLayoutWidget m_wViewSize;
    protected Widget m_wColumnDivider;
    protected Widget m_wDocumentPanel;
    protected TextWidget m_wSectionTitle;
    protected TextWidget m_wPlainSectionBody;
    protected RichTextWidget m_wRichSectionBody;
    protected ScrollLayoutWidget m_wSectionScroll;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wViewSize = SizeLayoutWidget.Cast(w);
        m_wColumnDivider = w.FindAnyWidget("BriefingColumnDivider");
        m_wDocumentPanel = w.FindAnyWidget("BriefingDocumentPanel");
        m_wSectionTitle = TextWidget.Cast(w.FindAnyWidget("BriefingSectionTitle"));
        m_wPlainSectionBody = TextWidget.Cast(w.FindAnyWidget("BriefingSectionPlainBody"));
        m_wRichSectionBody = RichTextWidget.Cast(w.FindAnyWidget("BriefingSectionRichBody"));
        m_wSectionScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("BriefingSectionScroll"));

        Widget titlesRoot = w.FindAnyWidget("BriefingTitles");
        if (titlesRoot)
            m_Titles = GC_BriefingTitlesComponent.Cast(titlesRoot.FindHandler(GC_BriefingTitlesComponent));

        if (m_Titles)
            m_Titles.m_OnTitleSelected.Insert(OnTitleSelected);

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnBriefingsChanged.Insert(OnBriefingsChanged);
            m_LobbyManager.m_OnPlayerRegistryUpdated.Insert(OnPlayerRegistryChanged);
        }

        RefreshBriefings();
    }

    override void HandlerDeattached(Widget w)
    {
        if (m_Titles)
            m_Titles.m_OnTitleSelected.Remove(OnTitleSelected);

        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnBriefingsChanged.Remove(OnBriefingsChanged);
            m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnPlayerRegistryChanged);
        }

        m_Titles = null;
        m_LobbyManager = null;
        m_wViewSize = null;
        m_wColumnDivider = null;
        m_wDocumentPanel = null;
        m_wSectionTitle = null;
        m_wPlainSectionBody = null;
        m_wRichSectionBody = null;
        m_wSectionScroll = null;

        super.HandlerDeattached(w);
    }

    protected void OnTitleSelected(GC_BriefingTitleRowComponent row)
    {
        if (!row || !row.IsSelected())
        {
            ShowBriefing(null);
            return;
        }

        GC_Briefing briefing;
        if (m_LobbyManager)
            briefing = m_LobbyManager.GetBriefing(row.GetBriefingId());

        ShowBriefing(briefing);
    }

    protected void OnBriefingsChanged(int revision)
    {
        RefreshBriefings();
    }

    protected void OnPlayerRegistryChanged(int playerId)
    {
        RefreshBriefings();
    }

    protected void RefreshBriefings()
    {
        if (!m_Titles || !m_LobbyManager)
        {
            ShowBriefing(null);
            return;
        }

        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
        {
            ShowBriefing(null);
            return;
        }

        array<ref GC_Briefing> briefings = m_LobbyManager.GetBriefingsForPlayer(playerController.GetPlayerId());
        GC_BriefingTitleRowComponent selectedRow = m_Titles.ConfigureBriefings(briefings);
        if (!selectedRow)
        {
            ShowBriefing(null);
            return;
        }

        ShowBriefing(m_LobbyManager.GetBriefing(selectedRow.GetBriefingId()));
    }

    protected void ShowBriefing(GC_Briefing briefing)
    {
        if (!briefing)
        {
            SetViewWidth(COMPACT_WIDTH);

            if (m_wColumnDivider)
                m_wColumnDivider.SetVisible(false);

            if (m_wDocumentPanel)
                m_wDocumentPanel.SetVisible(false);

            if (m_wSectionTitle)
                m_wSectionTitle.SetText(string.Empty);

            if (m_wPlainSectionBody)
            {
                m_wPlainSectionBody.SetText(string.Empty);
                m_wPlainSectionBody.SetVisible(true);
            }

            if (m_wRichSectionBody)
            {
                m_wRichSectionBody.SetText(string.Empty);
                m_wRichSectionBody.SetVisible(false);
            }

            return;
        }

        SetViewWidth(EXPANDED_WIDTH);

        if (m_wColumnDivider)
            m_wColumnDivider.SetVisible(true);

        if (m_wDocumentPanel)
            m_wDocumentPanel.SetVisible(true);

        if (m_wSectionTitle)
            m_wSectionTitle.SetText(briefing.GetOrder().ToString(2) + "  " + briefing.GetTitle());

        bool isRichText = briefing.GetTextFormat() == GC_BriefingTextFormat.RICH;
        if (m_wPlainSectionBody)
        {
            m_wPlainSectionBody.SetText(briefing.GetContent());
            m_wPlainSectionBody.SetVisible(!isRichText);
        }

        if (m_wRichSectionBody)
        {
            m_wRichSectionBody.SetText(briefing.GetContent());
            m_wRichSectionBody.SetVisible(isRichText);
        }

        if (m_wSectionScroll)
            m_wSectionScroll.SetSliderPos(0, 0);
    }

    protected void SetViewWidth(float width)
    {
        if (!m_wViewSize)
            return;

        m_wViewSize.SetWidthOverride(width);

        // A FrameWidgetSlot owns the instantiated view's final dimensions and
        // otherwise keeps the authored 720-pixel width despite the size override.
        if (FrameWidget.Cast(m_wViewSize.GetParent()))
            FrameSlot.SetSizeX(m_wViewSize, width);
    }
}
