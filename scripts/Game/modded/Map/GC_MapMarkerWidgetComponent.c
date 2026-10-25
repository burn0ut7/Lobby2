modded class SCR_MapMarkerWidgetComponent
{
	protected bool m_bGC_OwnsAuthorOpacity;
	protected bool m_bGC_OwnsTypeIconOpacity;

	//------------------------------------------------------------------------------------------------
	protected bool GC_UsesPersistentDescription()
	{
		if (!GC_LobbyGameMode.GetGameMode() || !m_MarkerObject)
			return false;

		SCR_EMapMarkerType markerType = m_MarkerObject.GetType();
		return markerType == SCR_EMapMarkerType.PLACED_CUSTOM || markerType == SCR_EMapMarkerType.PLACED_MILITARY;
	}

	//------------------------------------------------------------------------------------------------
	override void SetTextVisible(bool state)
	{
		super.SetTextVisible(state);
		if (GC_UsesPersistentDescription())
		{
			if (!m_bGC_OwnsTypeIconOpacity)
				SetTypeIconsVisible(m_wTypeIconRoot.IsVisible());
			m_wMarkerText.SetVisible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void SetTypeIconsVisible(bool state)
	{
		if (!GC_UsesPersistentDescription())
		{
			if (m_bGC_OwnsTypeIconOpacity)
			{
				m_wTypeIconRoot.SetOpacity(1);
				m_bGC_OwnsTypeIconOpacity = false;
			}

			super.SetTypeIconsVisible(state);
			return;
		}

		// The native details row also participates in the marker's vertical layout.
		super.SetTypeIconsVisible(true);
		float opacity = 0;
		if (state)
			opacity = 1;
		m_wTypeIconRoot.SetOpacity(opacity);
		m_bGC_OwnsTypeIconOpacity = true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		bool handled = super.OnMouseLeave(w, enterW, x, y);
		if (GC_UsesPersistentDescription())
			m_wMarkerText.SetVisible(true);

		return handled;
	}

	//------------------------------------------------------------------------------------------------
	override protected void ShowAuthor(bool state)
	{
		if (!GC_UsesPersistentDescription())
		{
			if (m_bGC_OwnsAuthorOpacity)
			{
				m_wMarkerAuthor.SetOpacity(1);
				m_wAuthorPlatformIcon.SetOpacity(1);
				m_bGC_OwnsAuthorOpacity = false;
			}

			super.ShowAuthor(state);
			return;
		}

		// Reserve the native owner row without changing its platform-icon preference.
		// Opacity keeps hover from resizing the marker's content layout.
		super.ShowAuthor(true);
		float opacity = 0;
		if (state)
			opacity = 1;

		m_wMarkerAuthor.SetOpacity(opacity);
		m_wAuthorPlatformIcon.SetOpacity(opacity);
		m_bGC_OwnsAuthorOpacity = true;
	}
}
