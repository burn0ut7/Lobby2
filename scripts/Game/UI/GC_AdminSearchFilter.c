//! Editable filter using the game's combo list, row buttons and edit field.
class GC_AdminSearchFilter : SCR_ComboBoxComponent
{
	ref ScriptInvoker m_OnFilterChanged = new ScriptInvoker();
	protected EditBoxWidget m_Input;
	protected SCR_EventHandlerComponent m_InputEvents;
	protected ref array<string> m_aValues = {};
	protected ref array<string> m_aSearchTerms = {};
	protected string m_sValue;
	protected bool m_bChosen;
	protected bool m_bBuildingList;
	protected bool m_bSettingText;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Input = EditBoxWidget.Cast(w.FindAnyWidget("FilterInput"));
		m_InputEvents = SCR_EventHandlerComponent.Cast(m_Input.FindHandler(SCR_EventHandlerComponent));
		m_InputEvents.GetOnChange().Insert(OnInput);
		m_InputEvents.GetOnClick().Insert(OpenList);
		m_InputEvents.GetOnFocus().Insert(OnInputFocus);
		m_OnChanged.Insert(OnChoice);
		SCR_ButtonTextComponent field = SCR_ButtonTextComponent.GetButtonText("FilterField", w);
		field.GetRootWidget().FindAnyWidget("Text").SetVisible(false);
		field.m_OnClicked.Insert(OnField);
	}

	//------------------------------------------------------------------------------------------------
	void SetChoices(array<string> labels, array<string> values, array<string> searchTerms)
	{
		ClearAll();
		m_aValues.Copy(values);
		m_aSearchTerms.Copy(searchTerms);
		foreach (string label : labels)
			AddItem(label);
		if (m_bChosen)
		{
			m_iSelectedItem = m_aValues.Find(m_sValue);
			if (m_iSelectedItem > 0)
				SetInputText(m_aElementNames[m_iSelectedItem]);
		}
		if (m_bOpened)
		{
			CreateEntries();
			ApplySearch();
			m_Workspace.SetFocusedWidget(m_Input);
			m_Input.ActivateWriteMode();
		}
	}

	//------------------------------------------------------------------------------------------------
	string GetFilterValue()
	{
		if (m_bChosen)
			return m_sValue;
		string query = m_Input.GetText();
		query.TrimInPlace();
		string all = m_aElementNames[0];
		all.ToLower();
		string normalized = query;
		normalized.ToLower();
		if (normalized == all)
			return "";
		return query;
	}

	//------------------------------------------------------------------------------------------------
	bool HasChoice()
	{
		return m_bChosen;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnField(SCR_ButtonBaseComponent button)
	{
		if (m_bOpened)
			CloseList();
		else
			OpenList();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnInputFocus()
	{
		// Native combo creation temporarily moves focus; finish this event before opening.
		GetGame().GetCallqueue().CallLater(OpenFocusedList, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenFocusedList()
	{
		if (m_Workspace.GetFocusedWidget() == m_Input)
			OpenList();
	}

	//------------------------------------------------------------------------------------------------
	override void OpenList()
	{
		if (m_bOpened)
			return;
		super.OpenList();
		if (!m_bOpened)
			return;
		ApplySearch();
		m_Workspace.SetFocusedWidget(m_Input);
		m_Input.ActivateWriteMode();
		GetGame().GetCallqueue().CallLater(CheckFocus, 100, true);
	}

	//------------------------------------------------------------------------------------------------
	override void CreateEntries()
	{
		m_bBuildingList = true;
		super.CreateEntries();
		// The stock combo makes only its list modal, excluding the editable field.
		// Keep both accessible; native Back handling and focus-out close this list.
		if (m_wElementsRoot)
			m_Workspace.RemoveModal(m_wElementsRoot);
		m_bBuildingList = false;
	}

	//------------------------------------------------------------------------------------------------
	override void CloseList()
	{
		// Native modal focus callbacks can fire while entries are being rebuilt.
		if (m_bBuildingList)
			return;
		GetGame().GetCallqueue().Remove(OpenFocusedList);
		GetGame().GetCallqueue().Remove(CheckFocus);
		Widget focused = m_Workspace.GetFocusedWidget();
		Widget ancestor = focused;
		bool inside;
		while (ancestor)
		{
			if (ancestor == m_wRoot || ancestor == m_wElementsRoot)
				inside = true;
			ancestor = ancestor.GetParent();
		}
		super.CloseList();
		if (!inside && focused)
			m_Workspace.SetFocusedWidget(focused);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckFocus()
	{
		Widget focused = m_Workspace.GetFocusedWidget();
		while (focused)
		{
			if (focused == m_wRoot || focused == m_wElementsRoot)
				return;
			focused = focused.GetParent();
		}
		CloseList();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnInput(Widget widget)
	{
		if (m_bSettingText)
			return;
		m_bChosen = false;
		m_sValue = "";
		m_iSelectedItem = -1;
		OpenList();
		ApplySearch();
		m_OnFilterChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplySearch()
	{
		string query;
		if (!m_bChosen)
			query = m_Input.GetText();
		query.TrimInPlace();
		query.ToLower();
		foreach (int index, Widget row : m_aElementWidgets)
		{
			string terms = m_aElementNames[index] + " " + m_aSearchTerms[index];
			terms.ToLower();
			row.SetVisible(index == 0 || query.IsEmpty() || terms.Contains(query));
		}
		if (m_wScrollLayout)
			m_wScrollLayout.SetSliderPos(0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnChoice(SCR_SelectionWidgetComponent combo, int index)
	{
		if (index < 0 || index >= m_aValues.Count())
			return;
		m_bChosen = true;
		m_sValue = m_aValues[index];
		string label = m_aElementNames[index];
		if (index == 0)
			label = "";
		SetInputText(label);
		m_OnFilterChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetInputText(string text)
	{
		m_bSettingText = true;
		m_Input.SetText(text);
		m_bSettingText = false;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(OpenFocusedList);
		GetGame().GetCallqueue().Remove(CheckFocus);
		if (m_InputEvents)
		{
			m_InputEvents.GetOnChange().Remove(OnInput);
			m_InputEvents.GetOnClick().Remove(OpenList);
			m_InputEvents.GetOnFocus().Remove(OnInputFocus);
		}
		m_OnChanged.Remove(OnChoice);
		super.HandlerDeattached(w);
	}
}
