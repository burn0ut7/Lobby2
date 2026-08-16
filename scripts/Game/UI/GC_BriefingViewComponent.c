class GC_BriefingViewComponent : SCR_ScriptedWidgetComponent
{
    protected GC_BriefingTitlesComponent m_Titles;
    protected TextWidget m_wSectionTitle;
    protected RichTextWidget m_wSectionBody;
    protected ScrollLayoutWidget m_wSectionScroll;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wSectionTitle = TextWidget.Cast(w.FindAnyWidget("BriefingSectionTitle"));
        m_wSectionBody = RichTextWidget.Cast(w.FindAnyWidget("BriefingSectionBody"));
        m_wSectionScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("BriefingSectionScroll"));

        Widget titlesRoot = w.FindAnyWidget("BriefingTitles");
        if (titlesRoot)
            m_Titles = GC_BriefingTitlesComponent.Cast(titlesRoot.FindHandler(GC_BriefingTitlesComponent));

        if (m_Titles)
            m_Titles.m_OnTitleSelected.Insert(OnTitleSelected);

        ShowSection(1, "OVERVIEW");
    }

    override void HandlerDeattached(Widget w)
    {
        if (m_Titles)
            m_Titles.m_OnTitleSelected.Remove(OnTitleSelected);

        m_Titles = null;
        m_wSectionTitle = null;
        m_wSectionBody = null;
        m_wSectionScroll = null;

        super.HandlerDeattached(w);
    }

    protected void OnTitleSelected(GC_BriefingTitleRowComponent row)
    {
        if (!row)
            return;

        ShowSection(row.GetOrder(), row.GetTitle());
    }

    protected void ShowSection(int order, string title)
    {
        if (m_wSectionTitle)
            m_wSectionTitle.SetText(order.ToString(2) + "  " + title);

        if (m_wSectionBody)
            m_wSectionBody.SetText(GetSectionContent(order));

        if (m_wSectionScroll)
            m_wSectionScroll.SetSliderPos(0, 0);
    }

    protected string GetSectionContent(int order)
    {
        switch (order)
        {
            case 2:
                return GetSituationContent();
            case 3:
                return GetMissionContent();
            case 4:
                return GetObjectivesContent();
        }

        return GetOverviewContent();
    }

    protected string GetOverviewContent()
    {
        return "<p>Assault and secure key positions across the area of operations to establish control and degrade enemy capabilities.</p>"
            + "<p>Both sides have established strong defensive positions. Expect contact throughout the area.</p>"
            + "<p>Civilian activity is minimal. Maintain mission focus and minimize collateral damage.</p>"
            + "<br /><color rgba=\"255,152,50,255\"><b>KEY INFORMATION</b></color><br />"
            + "<color rgba=\"150,160,164,255\">AREA OF OPERATIONS</color><br />Everon, Baltic Sea<br /><br />"
            + "<color rgba=\"150,160,164,255\">PLAYERS</color><br />Two opposing forces<br /><br />"
            + "<color rgba=\"150,160,164,255\">EST. DURATION</color><br />60 minutes<br /><br />"
            + "<color rgba=\"150,160,164,255\">VICTORY CONDITION</color><br />Complete the assigned objectives and preserve friendly combat power.";
    }

    protected string GetSituationContent()
    {
        return "<p>Enemy forces control several strategically important positions and are prepared to reinforce them.</p>"
            + "<p>Friendly elements are assembling in protected staging areas. Reconnaissance is limited, so unit leaders should confirm routes before movement.</p>"
            + "<br /><color rgba=\"255,152,50,255\"><b>COMMANDER'S NOTE</b></color><br />"
            + "Maintain communications, identify targets before engagement, and report meaningful changes in enemy disposition.";
    }

    protected string GetMissionContent()
    {
        return "<p>Friendly forces will seize the designated objectives, prevent an organized enemy counterattack, and establish control of the area.</p>"
            + "<p>Leaders retain freedom of maneuver within their assigned tasks. Coordinate supporting fires and avoid isolating individual groups.</p>"
            + "<br /><color rgba=\"255,152,50,255\"><b>END STATE</b></color><br />"
            + "All required objectives are secure, enemy combat effectiveness is reduced, and friendly forces are prepared for follow-on operations.";
    }

    protected string GetObjectivesContent()
    {
        return "<color rgba=\"255,152,50,255\"><b>PRIMARY OBJECTIVES</b></color><br /><br />"
            + "<b>01</b>  Secure the assigned area of operations.<br /><br />"
            + "<b>02</b>  Disrupt enemy command and reinforcement routes.<br /><br />"
            + "<b>03</b>  Retain sufficient combat power to hold captured positions.<br /><br />"
            + "<color rgba=\"150,160,164,255\">Additional objectives may be issued by mission leadership as the situation develops.</color>";
    }
}
