class GC_BriefingTitleRowComponent : SCR_ButtonBaseComponent
{
    protected static const ResourceName ICON_SET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
    protected static const string DEFAULT_ICON = "scenarios";
    protected static const string SCENARIO_ICON = "vegetation";

    [Attribute("1", UIWidgets.EditBox, "One-based order displayed with a leading zero")]
    protected int m_iOrder;

    [Attribute("BRIEFING TITLE", UIWidgets.EditBox, "Title displayed by this briefing row")]
    protected string m_sTitle;

    [Attribute("0", UIWidgets.CheckBox, "Show this row as the active briefing section")]
    protected bool m_bSelected;

    [Attribute("scenarios", UIWidgets.EditBox, "Image name from the briefing icon set")]
    protected string m_sIconName;

    protected bool m_bHovered;
    protected int m_iBriefingOrder;
    protected TextWidget m_wOrder;
    protected TextWidget m_wTitle;
    protected ImageWidget m_wIcon;
    protected ImageWidget m_wSelectedBackground;
    protected ImageWidget m_wLeftAccent;
    protected ImageWidget m_wRightAccent;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wOrder = TextWidget.Cast(w.FindAnyWidget("Order"));
        m_wTitle = TextWidget.Cast(w.FindAnyWidget("Title"));
        m_wIcon = ImageWidget.Cast(w.FindAnyWidget("Icon"));
        m_wSelectedBackground = ImageWidget.Cast(w.FindAnyWidget("SelectedBackground"));
        m_wLeftAccent = ImageWidget.Cast(w.FindAnyWidget("LeftAccent"));
        m_wRightAccent = ImageWidget.Cast(w.FindAnyWidget("RightAccent"));

        SetIcon(m_sIconName);

        if (m_wOrder)
            m_wOrder.SetText(m_iOrder.ToString(2));

        if (m_wTitle)
            m_wTitle.SetText(m_sTitle);

        SetHoverSound(string.Empty);
        SetClickedSound(string.Empty);
        SetToggleable(false);
        UpdateVisuals();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (button != 0)
            return true;

        PlaySound(SCR_SoundEvent.CLICK);
        return super.OnClick(w, x, y, button);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        m_bHovered = true;
        UpdateVisuals();
        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnMouseEnter(w, x, y);
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        m_bHovered = false;
        UpdateVisuals();
        return super.OnMouseLeave(w, enterW, x, y);
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnFocus(w, x, y);
    }

    void SetSelected(bool selected)
    {
        m_bSelected = selected;
        UpdateVisuals();
    }

    bool IsSelected()
    {
        return m_bSelected;
    }

    int GetDisplayOrder()
    {
        return m_iOrder;
    }

    void SetDisplayOrder(int order)
    {
        m_iOrder = order;
        if (m_wOrder)
            m_wOrder.SetText(m_iOrder.ToString(2));
    }

    void SetIcon(string iconName)
    {
        m_sIconName = iconName;
        if (m_wIcon)
            m_wIcon.LoadImageFromSet(0, ICON_SET, m_sIconName);
    }

    string GetTitle()
    {
        return m_sTitle;
    }

    int GetBriefingOrder()
    {
        return m_iBriefingOrder;
    }

    void SetBriefingData(GC_Briefing briefing)
    {
        if (!briefing)
        {
            ClearBriefingData();
            return;
        }

        m_iBriefingOrder = briefing.GetOrder();
        m_iOrder = briefing.GetOrder();
        m_sTitle = briefing.GetTitle();
        if (GC_BriefingParser.HasScenarioTag(briefing.GetContent()))
            SetIcon(SCENARIO_ICON);
        else
            SetIcon(DEFAULT_ICON);

        if (m_wOrder)
            m_wOrder.SetText(m_iOrder.ToString(2));

        if (m_wTitle)
            m_wTitle.SetText(m_sTitle);

        if (m_wRoot)
            m_wRoot.SetVisible(true);
    }

    void ClearBriefingData()
    {
        m_iBriefingOrder = 0;
        m_bSelected = false;

        if (m_wRoot)
            m_wRoot.SetVisible(false);

        UpdateVisuals();
    }

    protected void UpdateVisuals()
    {
        Color textColor = Color.FromRGBA(220, 220, 220, 255);
        Color accentColor = Color.FromRGBA(255, 255, 255, 255);

        if (m_bSelected)
        {
            textColor = Color.FromRGBA(255, 152, 50, 255);
            accentColor = textColor;
        }

        if (m_wSelectedBackground)
        {
            m_wSelectedBackground.SetColor(Color.FromRGBA(255, 152, 50, 255));
            if (m_bSelected)
                m_wSelectedBackground.SetOpacity(0.08);
            else
                m_wSelectedBackground.SetOpacity(0);
        }

        float accentOpacity = 0;
        if (m_bSelected)
            accentOpacity = 1;
        else if (m_bHovered)
            accentOpacity = 0.5;

        if (m_wLeftAccent)
        {
            m_wLeftAccent.SetColor(accentColor);
            m_wLeftAccent.SetOpacity(accentOpacity);
        }

        if (m_wRightAccent)
        {
            m_wRightAccent.SetColor(accentColor);
            m_wRightAccent.SetOpacity(accentOpacity);
        }

        if (m_wOrder)
            m_wOrder.SetColor(textColor);

        if (m_wTitle)
            m_wTitle.SetColor(textColor);

        if (m_wIcon)
            m_wIcon.SetColor(textColor);
    }
}
