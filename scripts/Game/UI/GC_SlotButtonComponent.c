class GC_SlotButtonComponent : SCR_ButtonBaseComponent
{
	protected GC_Slot m_Slot;
	protected bool m_bSlotInteractable = true;
	protected bool m_bSlotHovered;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		// Sound playback is kept behind the slot interaction check.
		SetHoverSound(string.Empty);
		SetClickedSound(string.Empty);
		SetToggleable(false);
		UpdateSlotVisuals();
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0 || !CanInteract())
			return true;

		PlaySound(SCR_SoundEvent.CLICK);
		return super.OnClick(w, x, y, button);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!CanInteract())
			return true;

		m_bSlotHovered = true;
		PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
		UpdateSlotVisuals();
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bSlotHovered = false;
		UpdateSlotVisuals();

		if (!CanInteract())
			return true;

		return super.OnMouseLeave(w, enterW, x, y);
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		if (!CanInteract())
			return true;

		PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
		return super.OnFocus(w, x, y);
	}

	//! Binds the authoritative lobby slot used for interaction checks.
	void SetSlot(GC_Slot slot)
	{
		m_Slot = slot;
		UpdateSlotVisuals();
	}

	GC_Slot GetSlot()
	{
		return m_Slot;
	}

	//! Enables or disables local interaction without changing authoritative slot state.
	void SetSlotInteractable(bool interactable)
	{
		m_bSlotInteractable = interactable;
		if (!interactable)
			m_bSlotHovered = false;

		UpdateSlotVisuals();
	}

	bool IsSlotInteractable()
	{
		return CanInteract();
	}

	protected bool CanInteract()
	{
		if (!m_bSlotInteractable)
			return false;

		if (!m_Slot)
			return true;

		return m_Slot.GetState() == GC_SlotState.FREE;
	}

	protected void UpdateSlotVisuals()
	{
		if (!m_wRoot)
			return;

		ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("SelectionAccent"));
		if (accent)
		{
			if (!CanInteract())
				accent.SetOpacity(0.35);
			else if (m_bSlotHovered)
				accent.SetOpacity(1);
			else
				accent.SetOpacity(0.85);
		}
	}
}
