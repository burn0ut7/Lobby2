//! Runtime row used by the scenario parameter modal. The row keeps its
//! controls local; only the complete draft is sent by the parent on Apply.
class GC_BriefingParameterRowComponent : SCR_ScriptedWidgetComponent
{
	ref ScriptInvoker<GC_BriefingParameterRowComponent> m_OnChanged = new ScriptInvoker<GC_BriefingParameterRowComponent>();
	protected GC_LobbyParameterDefinition m_Definition;
	protected ref GC_LobbyParameterValue m_Value;
	protected TextWidget m_wLabel;
	protected TextWidget m_wDescription;
	protected Widget m_wEditControl;
	protected EditBoxWidget m_wEdit;
	protected Widget m_wBoolControl;
	protected TextWidget m_wBoolText;
	protected ImageWidget m_wBoolBackground;
	protected SCR_ButtonBaseComponent m_BoolButton;
	protected Widget m_wCombo;
	protected SCR_ComboBoxComponent m_Combo;
	protected SCR_ButtonBaseComponent m_ComboButton;
	protected Widget m_wSliderControl;
	protected SCR_SliderComponent m_Slider;
	protected TextWidget m_wSliderText;
	protected Widget m_wStepper;
	protected TextWidget m_wStepValue;
	protected SCR_ButtonBaseComponent m_StepDown;
	protected SCR_ButtonBaseComponent m_StepUp;
	protected SCR_EventHandlerComponent m_EditHandler;
	protected GC_LobbyIntegerParameterDefinition m_IntegerDefinition;
	protected GC_LobbyFloatParameterDefinition m_FloatDefinition;
	protected int m_iDefinitionIndex;
	protected bool m_bEditable;
	protected bool m_bUpdatingEditText;
	protected bool m_bBoolValue;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wLabel = TextWidget.Cast(w.FindAnyWidget("ParameterLabel"));
		m_wDescription = TextWidget.Cast(w.FindAnyWidget("ParameterDescription"));
		m_wEditControl = w.FindAnyWidget("ParameterValueEditControl");
		m_wEdit = EditBoxWidget.Cast(w.FindAnyWidget("ParameterValueEdit"));
		m_wBoolControl = w.FindAnyWidget("ParameterValueBoolControl");
		m_wBoolText = TextWidget.Cast(w.FindAnyWidget("ParameterValueBoolText"));
		m_wBoolBackground = ImageWidget.Cast(w.FindAnyWidget("ParameterValueBoolBackground"));
		Widget boolButton = w.FindAnyWidget("ParameterValueBoolButton");
		if (boolButton)
			m_BoolButton = SCR_ButtonBaseComponent.Cast(boolButton.FindHandler(SCR_ButtonBaseComponent));
		m_wCombo = w.FindAnyWidget("ParameterValueCombo");
		if (m_wCombo)
		{
			m_Combo = SCR_ComboBoxComponent.Cast(m_wCombo.FindHandler(SCR_ComboBoxComponent));
			Widget comboButton = m_wCombo.FindAnyWidget("ComboButton");
			if (comboButton)
				m_ComboButton = SCR_ButtonBaseComponent.Cast(comboButton.FindHandler(SCR_ButtonBaseComponent));
		}
		m_wSliderControl = w.FindAnyWidget("ParameterValueSliderControl");
		if (m_wSliderControl)
		{
			m_Slider = SCR_SliderComponent.Cast(m_wSliderControl.FindHandler(SCR_SliderComponent));
			m_wSliderText = TextWidget.Cast(m_wSliderControl.FindAnyWidget("SliderText"));
		}
		m_wStepper = w.FindAnyWidget("ParameterValueStepper");
		m_wStepValue = TextWidget.Cast(w.FindAnyWidget("ParameterValueStepValue"));
		if (m_wStepper)
		{
			Widget stepDown = m_wStepper.FindAnyWidget("ParameterValueStepDown");
			Widget stepUp = m_wStepper.FindAnyWidget("ParameterValueStepUp");
			if (stepDown)
				m_StepDown = SCR_ButtonBaseComponent.Cast(stepDown.FindHandler(SCR_ButtonBaseComponent));
			if (stepUp)
				m_StepUp = SCR_ButtonBaseComponent.Cast(stepUp.FindHandler(SCR_ButtonBaseComponent));
		}

		if (m_wEdit)
		{
			m_EditHandler = SCR_EventHandlerComponent.Cast(m_wEdit.FindHandler(SCR_EventHandlerComponent));
			if (m_EditHandler)
				m_EditHandler.GetOnChange().Insert(OnEditChanged);
		}
		if (m_BoolButton)
			m_BoolButton.m_OnClicked.Insert(OnBoolClicked);
		if (m_Combo)
			m_Combo.m_OnChanged.Insert(OnSelectionChanged);
		if (m_ComboButton)
			m_ComboButton.m_OnClicked.Insert(OnComboClicked);
		if (m_Slider)
			m_Slider.m_OnChanged.Insert(OnSliderChanged);
		if (m_StepDown)
			m_StepDown.m_OnClicked.Insert(OnStepDownClicked);
		if (m_StepUp)
			m_StepUp.m_OnClicked.Insert(OnStepUpClicked);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(SanitizeNumericEditText);

		if (m_EditHandler)
			m_EditHandler.GetOnChange().Remove(OnEditChanged);
		if (m_BoolButton)
			m_BoolButton.m_OnClicked.Remove(OnBoolClicked);
		if (m_Combo)
			m_Combo.m_OnChanged.Remove(OnSelectionChanged);
		if (m_ComboButton)
			m_ComboButton.m_OnClicked.Remove(OnComboClicked);
		if (m_Slider)
			m_Slider.m_OnChanged.Remove(OnSliderChanged);
		if (m_StepDown)
			m_StepDown.m_OnClicked.Remove(OnStepDownClicked);
		if (m_StepUp)
			m_StepUp.m_OnClicked.Remove(OnStepUpClicked);

		m_Definition = null;
		m_Value = null;
		m_IntegerDefinition = null;
		m_FloatDefinition = null;
		m_iDefinitionIndex = -1;
		m_wLabel = null;
		m_wDescription = null;
		m_wEditControl = null;
		m_wEdit = null;
		m_wBoolControl = null;
		m_wBoolText = null;
		m_wBoolBackground = null;
		m_BoolButton = null;
		m_wCombo = null;
		m_Combo = null;
		m_ComboButton = null;
		m_wSliderControl = null;
		m_Slider = null;
		m_wSliderText = null;
		m_wStepper = null;
		m_wStepValue = null;
		m_StepDown = null;
		m_StepUp = null;
		m_EditHandler = null;
		super.HandlerDeattached(w);
	}

	void Configure(GC_LobbyParameterDefinition definition, GC_LobbyParameterValue value, bool editable, int definitionIndex)
	{
		m_Definition = definition;
		m_Value = new GC_LobbyParameterValue();
		m_Value.CopyFrom(value);
		m_IntegerDefinition = GC_LobbyIntegerParameterDefinition.Cast(definition);
		m_FloatDefinition = GC_LobbyFloatParameterDefinition.Cast(definition);
		m_iDefinitionIndex = definitionIndex;
		m_bEditable = editable;
		m_bUpdatingEditText = false;

		if (m_wLabel)
			m_wLabel.SetText(definition.GetLabel());
		if (m_wDescription)
			m_wDescription.SetText(definition.GetDescription());

		SetControlVisible(m_wEditControl, false);
		SetControlVisible(m_wBoolControl, false);
		SetControlVisible(m_wCombo, false);
		SetControlVisible(m_wSliderControl, false);
		SetControlVisible(m_wStepper, false);

		GC_LobbyBoolParameterDefinition boolDefinition = GC_LobbyBoolParameterDefinition.Cast(definition);
		if (boolDefinition && m_wBoolControl && m_BoolButton)
		{
			m_bBoolValue = value.GetBoolValue();
			m_wBoolControl.SetVisible(true);
			m_wBoolControl.SetEnabled(editable);
			m_BoolButton.SetEnabled(editable);
			UpdateBoolVisuals();
			return;
		}

		GC_LobbySelectionParameterDefinition selectionDefinition = GC_LobbySelectionParameterDefinition.Cast(definition);
		if (selectionDefinition && m_Combo)
		{
			m_wCombo.SetVisible(true);
			m_Combo.ClearAll();
			int selectedIndex = -1;
			int optionIndex;
			foreach (GC_LobbyParameterOption option : selectionDefinition.GetOptions())
			{
				m_Combo.AddItem(option.GetLabel(), false, option);
				if (option.GetKey() == value.GetSelectionKey())
					selectedIndex = optionIndex;
				optionIndex++;
			}
			if (selectedIndex < 0 && m_Combo.GetNumItems() > 0)
				selectedIndex = 0;
			if (selectedIndex >= 0)
				m_Combo.SetCurrentItem(selectedIndex, false, false, false);
			m_Combo.SetEnabled(editable);
			if (m_ComboButton)
				m_ComboButton.SetEnabled(editable);
			return;
		}

		if (m_IntegerDefinition)
		{
			switch (m_IntegerDefinition.GetPresentation())
			{
				case GC_ELobbyIntegerPresentation.SPIN_BOX:
					if (m_wStepper)
					{
						m_wStepper.SetVisible(true);
						UpdateStepperText(value.GetIntValue());
						SetStepperEnabled(editable);
					}
					return;
				case GC_ELobbyIntegerPresentation.SLIDER:
					if (m_Slider)
					{
						m_wSliderControl.SetVisible(true);
						m_Slider.SetSliderSettings(m_IntegerDefinition.GetMinimum(), m_IntegerDefinition.GetMaximum(), m_IntegerDefinition.GetStep(), "%1");
						m_Slider.SetValue(value.GetIntValue());
						m_wSliderControl.SetEnabled(editable);
					}
					return;
			}
		}

		if (m_FloatDefinition && m_FloatDefinition.GetPresentation() == GC_ELobbyFloatPresentation.SLIDER && m_Slider)
		{
			m_wSliderControl.SetVisible(true);
			m_Slider.SetSliderSettings(m_FloatDefinition.GetMinimum(), m_FloatDefinition.GetMaximum(), m_FloatDefinition.GetStep(), "%1");
			m_Slider.SetValue(value.GetFloatValue());
			m_wSliderControl.SetEnabled(editable);
			UpdateSliderText(value.GetFloatValue());
			return;
		}

		if (m_wEditControl && m_wEdit)
		{
			m_wEditControl.SetVisible(true);
			string editText = value.GetNormalizedValue();
			if (m_FloatDefinition)
				editText = value.GetFloatValue().ToString(-1, m_FloatDefinition.GetPrecision());
			m_wEdit.SetText(editText);
			m_wEdit.SetEnabled(editable);
		}
	}

	void SetEditable(bool editable)
	{
		m_bEditable = editable;
		if (m_wEdit)
			m_wEdit.SetEnabled(editable);
		if (m_wEditControl)
			m_wEditControl.SetEnabled(editable);
		if (m_wBoolControl)
			m_wBoolControl.SetEnabled(editable);
		if (m_BoolButton)
			m_BoolButton.SetEnabled(editable);
		if (m_Combo)
			m_Combo.SetEnabled(editable);
		if (m_ComboButton)
			m_ComboButton.SetEnabled(editable);
		if (m_wSliderControl)
			m_wSliderControl.SetEnabled(editable);
		SetStepperEnabled(editable);
	}

	protected void SetControlVisible(Widget widget, bool visible)
	{
		if (widget)
			widget.SetVisible(visible);
	}

	protected void SetStepperEnabled(bool enabled)
	{
		if (m_StepDown)
			m_StepDown.SetEnabled(enabled);
		if (m_StepUp)
			m_StepUp.SetEnabled(enabled);
	}

	protected void UpdateStepperText(int value)
	{
		if (m_wStepValue)
			m_wStepValue.SetText(value.ToString());
	}

	protected void UpdateSliderText(float value)
	{
		if (!m_wSliderText)
			return;

		if (m_FloatDefinition)
			m_wSliderText.SetText(value.ToString(-1, m_FloatDefinition.GetPrecision()));
		else
			m_wSliderText.SetText(value.ToString());
	}

	protected void UpdateBoolVisuals()
	{
		if (m_wBoolText)
		{
			if (m_bBoolValue)
			{
				m_wBoolText.SetText("ENABLED");
				m_wBoolText.SetColor(Color.FromInt(GC_LobbyUIColors.ACCENT));
			}
			else
			{
				m_wBoolText.SetText("DISABLED");
				m_wBoolText.SetColor(Color.FromInt(GC_LobbyUIColors.TEXT_MUTED));
			}
		}

		if (m_wBoolBackground)
		{
			if (m_bBoolValue)
				m_wBoolBackground.SetColor(Color.FromInt(GC_LobbyUIColors.PANEL_STRONG));
			else
				m_wBoolBackground.SetColor(Color.FromInt(GC_LobbyUIColors.PANEL));
		}
	}

	protected void OnEditChanged(Widget widget)
	{
		if (!IsInteractionAllowed())
			return;

		if (m_wEdit && !m_bUpdatingEditText && (m_IntegerDefinition || m_FloatDefinition))
		{
			// EditBoxWidget dispatches OnChange synchronously from SetText. Filter
			// on the next frame so replacing invalid input cannot recurse through
			// this event handler.
			GetGame().GetCallqueue().Remove(SanitizeNumericEditText);
			GetGame().GetCallqueue().CallLater(SanitizeNumericEditText, 0);
		}

		m_OnChanged.Invoke(this);
	}

	protected void SanitizeNumericEditText()
	{
		if (!m_wEdit || (!m_IntegerDefinition && !m_FloatDefinition))
			return;

		string text = m_wEdit.GetText();
		string filtered;
		bool hasDecimal;
		for (int index = 0; index < text.Length(); index++)
		{
			string character = text[index];
			int ascii = character.ToAscii();
			if (ascii >= 48 && ascii <= 57)
				filtered += character;
			else if ((ascii == 43 || ascii == 45) && filtered.IsEmpty())
				filtered += character;
			else if (ascii == 46 && m_FloatDefinition && !hasDecimal)
			{
				filtered += character;
				hasDecimal = true;
			}
		}

		if (filtered == text)
			return;

		m_bUpdatingEditText = true;
		m_wEdit.SetText(filtered);
		m_bUpdatingEditText = false;
	}

	protected void OnComboClicked(SCR_ButtonBaseComponent button)
	{
		if (IsInteractionAllowed() && m_Combo)
			m_Combo.OpenList();
	}

	protected void OnBoolClicked(SCR_ButtonBaseComponent button)
	{
		if (!IsInteractionAllowed() || button != m_BoolButton)
			return;

		m_bBoolValue = !m_bBoolValue;
		UpdateBoolVisuals();
		m_OnChanged.Invoke(this);
	}

	protected void OnSelectionChanged(SCR_ComboBoxComponent combo, int index)
	{
		if (!IsInteractionAllowed())
			return;

		m_OnChanged.Invoke(this);
	}

	protected void OnSliderChanged(SCR_SliderComponent slider, float value)
	{
		if (!IsInteractionAllowed())
			return;

		UpdateSliderText(value);
		m_OnChanged.Invoke(this);
	}

	protected void OnStepDownClicked(SCR_ButtonBaseComponent button)
	{
		if (!IsInteractionAllowed() || !m_IntegerDefinition)
			return;

		int value = m_wStepValue.GetText().ToInt() - m_IntegerDefinition.GetStep();
		value = Math.Clamp(value, m_IntegerDefinition.GetMinimum(), m_IntegerDefinition.GetMaximum());
		UpdateStepperText(value);
		m_OnChanged.Invoke(this);
	}

	protected void OnStepUpClicked(SCR_ButtonBaseComponent button)
	{
		if (!IsInteractionAllowed() || !m_IntegerDefinition)
			return;

		int value = m_wStepValue.GetText().ToInt() + m_IntegerDefinition.GetStep();
		value = Math.Clamp(value, m_IntegerDefinition.GetMinimum(), m_IntegerDefinition.GetMaximum());
		UpdateStepperText(value);
		m_OnChanged.Invoke(this);
	}

	//! m_bEditable is presentation state only. Authority is queried again for
	//! every interaction so a logout cannot use a still-enabled control while
	//! the reliable role notification is converging locally.
	protected bool IsInteractionAllowed()
	{
		if (!m_bEditable)
			return false;

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController || !SCR_Global.IsAdmin(playerController.GetPlayerId()))
			return false;

		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return false;

		SCR_EGameModeState phase = lobbyManager.GetLobbyPhase();
		return phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING;
	}

	protected bool IsIntegerText(string text)
	{
		if (text.IsEmpty())
			return false;

		string remaining = text;
		remaining.Replace("0", "");
		remaining.Replace("1", "");
		remaining.Replace("2", "");
		remaining.Replace("3", "");
		remaining.Replace("4", "");
		remaining.Replace("5", "");
		remaining.Replace("6", "");
		remaining.Replace("7", "");
		remaining.Replace("8", "");
		remaining.Replace("9", "");
		return remaining.IsEmpty()
			|| (remaining == "-" && text.StartsWith("-") && text.Length() > 1)
			|| (remaining == "+" && text.StartsWith("+") && text.Length() > 1);
	}

	protected bool IsFloatText(string text)
	{
		if (text.IsEmpty())
			return false;

		string remaining = text;
		remaining.Replace("0", "");
		remaining.Replace("1", "");
		remaining.Replace("2", "");
		remaining.Replace("3", "");
		remaining.Replace("4", "");
		remaining.Replace("5", "");
		remaining.Replace("6", "");
		remaining.Replace("7", "");
		remaining.Replace("8", "");
		remaining.Replace("9", "");
		return remaining.IsEmpty()
			|| (remaining == "." && text != ".")
			|| (remaining == "-" && text.StartsWith("-") && text.Length() > 1)
			|| (remaining == "+" && text.StartsWith("+") && text.Length() > 1)
			|| (remaining == "-." && text.StartsWith("-") && text.Length() > 2)
			|| (remaining == "+." && text.StartsWith("+") && text.Length() > 2);
	}

	GC_LobbyParameterValue BuildValue()
	{
		if (!m_Definition || !m_Value)
			return null;

		GC_LobbyParameterValue result = new GC_LobbyParameterValue();
		GC_LobbyBoolParameterDefinition boolDefinition = GC_LobbyBoolParameterDefinition.Cast(m_Definition);
		if (boolDefinition && m_BoolButton)
		{
			result.SetBool(m_iDefinitionIndex, m_bBoolValue);
			return result;
		}

		if (m_IntegerDefinition)
		{
			switch (m_IntegerDefinition.GetPresentation())
			{
				case GC_ELobbyIntegerPresentation.EDIT_BOX:
					if (!m_wEdit || !IsIntegerText(m_wEdit.GetText()))
						return null;
					result.SetInteger(m_iDefinitionIndex, m_wEdit.GetText().ToInt());
					return result;
				case GC_ELobbyIntegerPresentation.SPIN_BOX:
					if (!m_wStepValue)
						return null;
					result.SetInteger(m_iDefinitionIndex, m_wStepValue.GetText().ToInt());
					return result;
				case GC_ELobbyIntegerPresentation.SLIDER:
					if (!m_Slider)
						return null;
					int integerValue = Math.Round(m_Slider.GetValue()).ToString().ToInt();
					result.SetInteger(m_iDefinitionIndex, integerValue);
					return result;
			}
			return null;
		}

		if (m_FloatDefinition)
		{
			if (m_FloatDefinition.GetPresentation() == GC_ELobbyFloatPresentation.EDIT_BOX)
			{
				if (!m_wEdit || !IsFloatText(m_wEdit.GetText()))
					return null;
				result.SetFloat(m_iDefinitionIndex, m_wEdit.GetText().ToFloat());
			}
			else
			{
				if (!m_Slider)
					return null;
				result.SetFloat(m_iDefinitionIndex, m_Slider.GetValue());
			}
			return result;
		}

		GC_LobbyStringParameterDefinition stringDefinition = GC_LobbyStringParameterDefinition.Cast(m_Definition);
		if (stringDefinition && m_wEdit)
		{
			result.SetString(m_iDefinitionIndex, m_wEdit.GetText());
			return result;
		}

		GC_LobbySelectionParameterDefinition selectionDefinition = GC_LobbySelectionParameterDefinition.Cast(m_Definition);
		if (selectionDefinition && m_Combo)
		{
			int selectedIndex = m_Combo.GetCurrentIndex();
			array<ref GC_LobbyParameterOption> options = selectionDefinition.GetOptions();
			if (selectedIndex < 0 || selectedIndex >= options.Count())
				return null;
			result.SetSelection(m_iDefinitionIndex, options[selectedIndex].GetKey());
			return result;
		}

		return null;
	}

	int GetDefinitionIndex() { return m_iDefinitionIndex; }
	bool IsEditable() { return m_bEditable; }

	Widget GetWidgetRoot() { return m_wRoot; }
}
