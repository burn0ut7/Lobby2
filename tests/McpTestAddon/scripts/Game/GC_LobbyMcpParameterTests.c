#ifdef WORKBENCH
modded class GC_LobbyParametersComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestState()
	{
		return string.Format("draft dirty=%1 stale=%2 pending=%3 baseRevision=%4 acceptedRevision=%5 faction=%6", m_bDirty, m_bStale, m_bSubmitting, m_iBaseRevision, m_iAcceptedRevision, m_SelectedFactionKey);
	}
}

modded class GC_BriefingParameterRowComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestEdit(string action, string value)
	{
		if (!IsInteractionAllowed())
			return "rejected: parameter is not editable";

		switch (action)
		{
			case "text":
				if (!m_wEdit || !m_wEditControl || !m_wEditControl.IsVisible())
					return "rejected: no visible edit box";
				m_wEdit.SetText(value);
				break;
			case "toggle":
				if (!m_BoolButton || !m_wBoolControl.IsVisible())
					return "rejected: no visible boolean control";
				m_BoolButton.OnClick(m_BoolButton.GetRootWidget(), 0, 0, 0);
				break;
			case "next":
				if (!m_StepUp || !m_wStepper.IsVisible())
					return "rejected: no visible stepper";
				OnStepUpClicked(m_StepUp);
				break;
			case "previous":
				if (!m_StepDown || !m_wStepper.IsVisible())
					return "rejected: no visible stepper";
				OnStepDownClicked(m_StepDown);
				break;
			case "select":
				if (!m_Combo || !m_wCombo.IsVisible() || !IsIntegerText(value))
					return "rejected: no visible selection or invalid index";
				int index = value.ToInt();
				if (index < 0 || index >= m_Combo.GetNumItems())
					return "rejected: selection index out of range";
				m_Combo.SetCurrentItem(index, true, false, false);
				break;
			case "slider":
				if (!m_Slider || !m_wSliderControl.IsVisible() || !IsFloatText(value))
					return "rejected: no visible slider or invalid value";
				m_Slider.SetValue(value.ToFloat());
				break;
			default:
				return "rejected: unknown parameter action";
		}
		return "dispatched: parameter input";
	}
}
#endif
