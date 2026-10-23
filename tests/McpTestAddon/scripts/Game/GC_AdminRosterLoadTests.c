#ifdef WORKBENCH
//! UI-only workload. Negative fixture IDs never enter the native player registry.
modded class GC_AdminPlayerPicker
{
	protected int m_iGC_TestRosterCount;

	//------------------------------------------------------------------------------------------------
	override protected void ReadRoster(out array<int> players, out array<string> names)
	{
		super.ReadRoster(players, names);
		if (m_iGC_TestRosterCount <= 0)
			return;
		array<string> fixtures = {"Harris", "Harper", "Chris Harris", "Hector Adams Rivera"};
		int fixture;
		while (players.Count() < m_iGC_TestRosterCount)
		{
			players.Insert(-1000 - fixture);
			if (fixture < fixtures.Count())
				names.Insert(fixtures[fixture]);
			else
				names.Insert(string.Format("Alexandra Montgomery-Sutherland [UI %1]", fixture));
			fixture++;
		}
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRosterLoad(int count)
	{
		if (count != 0 && count != 128)
			return "REJECTED expected 0 or 128 display rows";
		m_iGC_TestRosterCount = count;
		int start = System.GetTickCount();
		Rebuild();
		return string.Format("fixtureRows=%1 rebuildMs=%2; inspect native selection separately", count, System.GetTickCount(start));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRosterProbe(string query, float scrollPosition)
	{
		if (!m_Menu || !m_Menu.IsVisible() || scrollPosition < 0 || scrollPosition > 1)
			return "UNAVAILABLE open roster or scroll outside 0..1";
		int start = System.GetTickCount();
		m_Search.SetText(query);
		m_SearchEvents.GetOnChange().Invoke(m_Search);
		int elapsed = System.GetTickCount(start);
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(m_Menu.FindAnyWidget("PickerRows").GetParent());
		scroll.SetSliderPos(0, scrollPosition);
		int count;
		string firstNames;
		Widget row = m_Menu.FindAnyWidget("PickerRows").GetChildren();
		while (row)
		{
			if (count < 8)
				firstNames += TextWidget.Cast(row.FindAnyWidget("Text")).GetText() + "|";
			count++;
			row = row.GetSibling();
		}
		return string.Format("rows=%1 searchMs=%2 scrollRequested=%3 first=%4", count, elapsed, scrollPosition, firstNames);
	}

	//------------------------------------------------------------------------------------------------
	//! Layout-only stress of the actual selected-field widgets, not a renamed player.
	string GC_TestLongFieldText()
	{
		m_Field.SetText("Alexandra Montgomery-Sutherland [UI layout fixture]");
		TextWidget.Cast(m_Field.GetRootWidget().FindAnyWidget("PlayerDetail")).SetText(
			"Alexandra Montgomery-Sutherland, Christopher Richardson-Wellington");
		return "ARRANGED display strings only; native selection and identities unchanged";
	}
}

#endif
