#ifdef WORKBENCH
modded class GC_InventoryCataloguePanel
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPreviewRetention()
	{
		if (m_aPage.Count() < 2 || m_iPreview < m_aPage.Count())
			return "page not ready";
		array<Widget> previousCells = {};
		previousCells.Copy(m_aCells);
		array<GC_ItemCatalogueEntry> previousPage = {};
		previousPage.Copy(m_aPage);
		Refresh();
		int stable;
		foreach (int index, GC_ItemCatalogueEntry entry : previousPage)
		{
			if (m_aCells[index] == previousCells[index] && m_aCells[index].FindAnyWidget("ItemPreview").IsVisible() && m_mPreviewPrefabs.Get(m_aCells[index]) == entry.m_sPrefab)
				stable++;
		}
		int offset = m_iOffset;
		m_iOffset++;
		Refresh();
		int retained;
		for (int i; i < previousPage.Count() - 1; i++)
		{
			if (m_aCells[i] == previousCells[i + 1] && m_aCells[i].FindAnyWidget("ItemPreview").IsVisible() && m_mPreviewPrefabs.Get(m_aCells[i]) == previousPage[i + 1].m_sPrefab)
				retained++;
		}
		m_iOffset = offset;
		Refresh();
		return string.Format("pass=%1 unchanged=%2/%3 shifted=%4/%5", stable == previousPage.Count() && retained == previousPage.Count() - 1, stable, previousPage.Count(), retained, previousPage.Count() - 1);
	}
}

modded class SCR_InventoryMenuUI
{
	string GC_TestCatalogueBlankWheel(int direction)
	{
		Widget surface = m_widget.FindAnyWidget("CatalogueSurface");
		float x, y, width, height;
		surface.GetScreenPos(x, y);
		surface.GetScreenSize(width, height);
		WidgetManager.ReportMouse(x + width * 0.75, y + height * 0.8, m_widget);
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		if (hit != surface)
			return "blank surface not hit";
		// Verify native hit testing separately from the production wheel handler.
		m_GCCataloguePanel.OnMouseWheel(hit, 0, 0, direction);
		return hit.GetName() + " " + m_GCCataloguePanel.State();
	}

	string GC_TestPreviewRetention()
	{
		return m_GCCataloguePanel.GC_TestPreviewRetention();
	}
}

modded class GC_LobbyManager
{
	string GC_TestCatalogueBlankWheel(int direction)
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestCatalogueBlankWheel(direction);
	}

	string GC_TestPreviewRetention()
	{
		return SCR_InventoryMenuUI.GetInventoryMenu().GC_TestPreviewRetention();
	}

	string GC_TestCatalogueSortKeys(bool descending)
	{
		array<string> names = {" SVD Magazine Pouch", "Zulu", "Alpha", "!Special", "_Special", "   "};
		array<string> keys = {};
		map<string, string> labels = new map<string, string>();
		foreach (string name : names)
		{
			GC_ItemCatalogueEntry entry = new GC_ItemCatalogueEntry();
			entry.m_sName = name;
			string key = entry.GetSortKey(descending);
			keys.Insert(key);
			labels.Insert(key, name.Trim());
		}
		keys.Sort(descending);
		string result;
		foreach (string key : keys)
			result += "[" + labels.Get(key) + "]";
		return result;
	}
}
#endif
