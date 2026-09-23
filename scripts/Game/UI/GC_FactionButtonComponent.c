class GC_FactionButtonComponent : SCR_ButtonBaseComponent
{
    protected FactionKey m_FactionKey;
    protected bool m_bFactionSelected;
    protected bool m_bFactionHovered;
	protected bool m_bFactionInteractable = true;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        // Selection belongs to GC_FactionListComponent. The base toggle is used
        // only as the visual state; the list prevents the selected button from
        // being toggled off and keeps the other buttons in sync.
        SetHoverSound(string.Empty);
        SetClickedSound(string.Empty);
        UpdateFactionVisuals();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
		if (button == 1)
		{
			GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForFaction(m_FactionKey), m_wRoot);
			return true;
		}

        if (button != 0 || m_bFactionSelected || !m_bFactionInteractable)
            return true;

        PlaySound(SCR_SoundEvent.CLICK);
        return super.OnClick(w, x, y, button);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (m_bFactionSelected || !m_bFactionInteractable)
            return true;

        m_bFactionHovered = true;
        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnMouseEnter(w, x, y);
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        m_bFactionHovered = false;

        if (m_bFactionSelected || !m_bFactionInteractable)
            return true;

        return super.OnMouseLeave(w, enterW, x, y);
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        if (m_bFactionSelected)
            return true;

        PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
        return super.OnFocus(w, x, y);
    }

    void SetFactionKey(FactionKey factionKey)
    {
        m_FactionKey = factionKey;
    }

    FactionKey GetFactionKey()
    {
        return m_FactionKey;
    }

    void SetFactionSelected(bool selected)
    {
        m_bFactionSelected = selected;
        m_bIsToggled = selected;

        if (selected)
            m_bFactionHovered = false;

        UpdateFactionVisuals();
    }

    bool IsFactionSelected()
    {
        return m_bFactionSelected;
    }

	void SetFactionInteractable(bool interactable)
	{
		m_bFactionInteractable = interactable;
		if (!interactable)
			m_bFactionHovered = false;

		UpdateFactionVisuals();
	}

    protected void UpdateFactionVisuals()
    {
        if (!m_wRoot)
            return;

        ColorizeBackground(false);
        Widget selectionOutline = m_wRoot.FindAnyWidget("SelectionOutline");
        if (selectionOutline)
        {
            if (m_bFactionSelected)
                selectionOutline.SetOpacity(1);
            else
                selectionOutline.SetOpacity(0);
        }
		if (m_bFactionInteractable || m_bFactionSelected)
			m_wRoot.SetOpacity(1);
		else
			m_wRoot.SetOpacity(0.35);
    }
}
