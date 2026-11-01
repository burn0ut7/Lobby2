#ifdef WORKBENCH
//! Editable save destination using the shared native searchable dropdown.
class GC_LoadoutScopeSelector : GC_AdminSearchFilter
{
	//------------------------------------------------------------------------------------------------
	string GetText()
	{
		if (m_aValues.IsEmpty())
			return m_Input.GetText();
		return GetFilterValue();
	}

	//------------------------------------------------------------------------------------------------
	void SetText(string value)
	{
		m_sValue = value;
		m_iSelectedItem = m_aValues.Find(value);
		m_bChosen = m_iSelectedItem >= 0;
		SetInputText(value);
	}

	//------------------------------------------------------------------------------------------------
	void SetPlaceholderText(string value)
	{
		m_Input.SetPlaceholderText(value);
	}

	//------------------------------------------------------------------------------------------------
	void Populate(array<string> values, string allLabel)
	{
		string value = GetText();
		array<string> labels = {WidgetManager.Translate(allLabel)};
		array<string> choices = {""};
		foreach (string option : values)
		{
			labels.Insert(option);
			choices.Insert(option);
		}
		SetChoices(labels, choices, choices);
		SetText(value);
		SetPlaceholderText(WidgetManager.Translate(allLabel));
	}

	//------------------------------------------------------------------------------------------------
	override void OpenList()
	{
		super.OpenList();
		ChimeraMenuBase menu = ChimeraMenuBase.GetOwnerMenu(m_wRoot);
		if (menu && m_wElementsRoot)
			m_wElementsRoot.SetZOrder(menu.GetRootWidget().GetZOrder() + 1);
	}
}
#endif
