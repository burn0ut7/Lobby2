class GC_HeaderButtonComponent : SCR_ButtonTextComponent
{
    [Attribute("0", UIWidgets.EditBox, "State index owned by this header button")]
    protected int m_iHeaderState;

    protected bool m_bHeaderSelected;
    protected bool m_bHeaderHovered;
    protected bool m_bHeaderLocked;
    protected bool m_bHeaderPermissionLocked;

    int GetHeaderState()
    {
        return m_iHeaderState;
    }

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        // The base widget component plays these automatically from its serialized attributes.
        // Clear them so sound playback remains under the header permission check below.
        SetHoverSound(string.Empty);
        SetClickedSound(string.Empty);

        // Establish the permission state before the header controller finishes its deferred setup.
        SetHeaderEnabled(SCR_Global.IsAdmin());

        // The header owns selection. Do not let the base button toggle itself.
        SetToggleable(false);
        UpdateHeaderVisuals();
    }

    void SetHeaderLocked(bool locked)
    {
        m_bHeaderLocked = locked;
        if (locked)
            m_bHeaderHovered = false;

        UpdateHeaderVisuals();
    }

    protected bool IsHeaderInteractionLocked()
    {
        return m_bHeaderLocked || m_bHeaderPermissionLocked;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (IsHeaderInteractionLocked())
            return true;

        PlaySound(SCR_SoundEvent.CLICK);
        return super.OnClick(w, x, y, button);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (IsHeaderInteractionLocked())
            return true;

        SetHeaderHovered(true);
        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnMouseEnter(w, x, y);
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        SetHeaderHovered(false);

        if (IsHeaderInteractionLocked())
            return true;

        return super.OnMouseLeave(w, enterW, x, y);
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        if (IsHeaderInteractionLocked())
            return true;

        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnFocus(w, x, y);
    }

    void SetHeaderSelected(bool selected)
    {
        m_bHeaderSelected = selected;
        UpdateHeaderVisuals();
    }

    void SetHeaderHovered(bool hovered)
    {
        m_bHeaderHovered = hovered;
        UpdateHeaderVisuals();
    }

    void SetHeaderEnabled(bool enabled)
    {
        if (!m_wRoot)
            return;

        m_bHeaderPermissionLocked = !enabled;
        if (!enabled)
            m_bHeaderHovered = false;
    }

    protected void UpdateHeaderVisuals()
    {
        if (!m_wRoot)
            return;

        Color color = Color.FromRGBA(255, 255, 255, 255);
        if (m_bHeaderSelected)
            color = Color.FromRGBA(255, 152, 50, 255);

        ImageWidget image = ImageWidget.Cast(m_wRoot.FindAnyWidget("Image0"));
        if (image)
            image.SetColor(color);

        TextWidget text = TextWidget.Cast(m_wRoot.FindAnyWidget("Text"));
        if (text)
            text.SetColor(color);

        Widget lineHighlight = m_wRoot.FindAnyWidget("LineHighlight");
        if (lineHighlight)
        {
            if (m_bHeaderSelected)
                lineHighlight.SetOpacity(0.2);
            else
                lineHighlight.SetOpacity(0);
        }

        Widget lineHighlight2 = m_wRoot.FindAnyWidget("LineHighlight2");
        if (lineHighlight2)
        {
            if (m_bHeaderSelected)
            {
                lineHighlight2.SetOpacity(1);
                lineHighlight2.SetColor(Color.FromRGBA(255, 152, 50, 255));
            }
            else if (m_bHeaderHovered)
            {
                lineHighlight2.SetOpacity(1);
                lineHighlight2.SetColor(Color.FromRGBA(255, 255, 255, 255));
            }
            else
            {
                lineHighlight2.SetOpacity(0);
                lineHighlight2.SetColor(Color.FromRGBA(255, 255, 255, 255));
            }
        }

        Widget border = m_wRoot.FindAnyWidget("Border");
        if (border)
        {
            if (m_bHeaderSelected)
                border.SetOpacity(1);
            else
                border.SetOpacity(0);
        }
    }
}
