#ifdef WORKBENCH
//! Search presentation and bounded previews inside the native inventory menu.
class GC_InventoryCataloguePanel : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected Widget m_Vicinity;
	protected Widget m_Opened;
	protected EditBoxWidget m_Search;
	protected ref GC_ItemCatalogue m_Catalogue;
	protected ref GC_ItemCatalogueQuery m_Query = new GC_ItemCatalogueQuery();
	protected ref array<GC_ItemCatalogueEntry> m_aPage = {};
	protected ref array<Widget> m_aCells = {};
	protected int m_iOffset;
	protected int m_iTotal;
	protected int m_iPreview;
	protected int m_iIndexed = -1;
	protected float m_fRefresh;
	protected string m_sPendingText;
	protected float m_fSearchDelay;
	protected bool m_bFinished;
	protected SCR_ComboBoxComponent m_Faction;
	protected SCR_ComboBoxComponent m_Sort;
	protected SCR_SpinBoxPagingComponent m_Paging;
	protected float m_fVicinityOpacity;
	protected float m_fOpenedOpacity;
	protected bool m_bPagingActive;
	protected ref array<SCR_ButtonBaseComponent> m_aButtons = {};
	protected string m_sCategoryLabel = "All items";
	protected static const int PAGE_SIZE = 9;

	//------------------------------------------------------------------------------------------------
	bool Attach(Widget inventoryRoot)
	{
		Widget left = inventoryRoot.FindAnyWidget("LeftArea");
		m_Vicinity = inventoryRoot.FindAnyWidget("StorageLootSlot");
		m_Opened = inventoryRoot.FindAnyWidget("OpenedStorages");
		if (!left || !m_Vicinity || !m_Opened)
			return false;
		m_Root = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE1001}UI/layouts/Inventory/GC_InventoryCatalogue.layout", inventoryRoot);
		if (!m_Root)
			return false;
		if (!m_Root.FindAnyWidget("Results") || !m_Root.FindAnyWidget("Status") || !m_Root.FindAnyWidget("CatalogueSearch"))
		{
			m_Root.RemoveFromHierarchy();
			m_Root = null;
			return false;
		}
		m_fVicinityOpacity = m_Vicinity.GetOpacity();
		m_fOpenedOpacity = m_Opened.GetOpacity();
		m_Vicinity.SetOpacity(0);
		m_Opened.SetOpacity(0);
		m_Root.AddHandler(this);
		array<string> buttonNames = {"Advanced", "ClearFilter", "CategoryAll", "CategoryWeapons", "CategoryHeadwear", "CategoryClothing", "CategoryBackpacks", "CategoryVests", "CategoryEquipment", "CategoryMagazines", "CategoryAttachments"};
		foreach (string buttonName : buttonNames)
			BindButton(m_Root.FindAnyWidget(buttonName));
		m_Paging = SCR_SpinBoxPagingComponent.GetSpinBoxPagingComponent("CataloguePaging", m_Root);
		m_Paging.m_OnChanged.Insert(OnPageChanged);
		m_Search = EditBoxWidget.Cast(m_Root.FindAnyWidget("CatalogueSearch"));
		for (int i; i < PAGE_SIZE; i++)
		{
			Widget cell = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE1002}UI/layouts/Inventory/GC_InventoryCatalogueCell.layout", m_Root.FindAnyWidget("Results"));
			m_aCells.Insert(cell);
			BindButton(cell.FindAnyWidget("SelectItem"));
			RenderTargetWidget preview = RenderTargetWidget.Cast(cell.FindAnyWidget("ItemPreview"));
			preview.SetClearColor(false, 0);
			preview.SetBlendMode(RenderTargetWidgetBlendMode.ALPHA_BLEND);
		}
		m_Catalogue = new GC_ItemCatalogue();
		m_Catalogue.Start();
		m_Query.m_bPortableOnly = true;
		m_Query.m_bAlphabetical = true;
		m_Faction = SCR_ComboBoxComponent.Cast(m_Root.FindAnyWidget("Faction").FindHandler(SCR_ComboBoxComponent));
		m_Faction.AddItem("#GC-Catalogue_AllFactions");
		foreach (string faction : m_Catalogue.GetFactionKeys())
			m_Faction.AddItem(faction);
		m_Faction.AddItem("#GC-Catalogue_Unassigned");
		m_Faction.SetCurrentItem(0, false, false, false);
		m_Faction.m_OnChanged.Insert(OnOptionsChanged);
		m_Sort = SCR_ComboBoxComponent.Cast(m_Root.FindAnyWidget("Sort").FindHandler(SCR_ComboBoxComponent));
		m_Sort.AddItem("A-Z");
		m_Sort.AddItem("Z-A");
		m_Sort.SetCurrentItem(0, false, false, false);
		m_Sort.m_OnChanged.Insert(OnOptionsChanged);
		Refresh();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Detach()
	{
		foreach (SCR_ButtonBaseComponent button : m_aButtons)
			button.m_OnClicked.Remove(OnButton);
		m_aButtons.Clear();
		if (m_Paging)
		{
			m_Paging.m_OnChanged.Remove(OnPageChanged);
			m_Paging.SetButtonsActive(false);
		}
		if (m_Faction)
		{
			m_Faction.m_OnChanged.Remove(OnOptionsChanged);
			m_Faction.CloseList();
		}
		if (m_Sort)
		{
			m_Sort.m_OnChanged.Remove(OnOptionsChanged);
			m_Sort.CloseList();
		}
		if (m_Root)
		{
			m_Root.RemoveHandler(this);
			m_Root.RemoveFromHierarchy();
			if (m_Vicinity)
				m_Vicinity.SetOpacity(m_fVicinityOpacity);
			if (m_Opened)
				m_Opened.SetOpacity(m_fOpenedOpacity);
		}
		m_Root = null;
		m_aCells.Clear();
		m_aPage.Clear();
		m_Catalogue = null;
	}

	//------------------------------------------------------------------------------------------------
	void Tick(float timeSlice)
	{
		if (!m_Root)
			return;
		m_Catalogue.Tick();
		bool pagingActive = GetGame().GetWorkspace().GetFocusedWidget() != m_Search && !m_Faction.IsOpened() && !m_Sort.IsOpened();
		if (pagingActive != m_bPagingActive)
		{
			m_bPagingActive = pagingActive;
			m_Paging.SetButtonsActive(pagingActive);
		}
		string text = m_Search.GetText();
		if (text != m_sPendingText)
		{
			m_sPendingText = text;
			m_fSearchDelay = 0.15;
		}
		m_fSearchDelay -= timeSlice;
		if (m_fSearchDelay <= 0 && m_Query.m_sText != m_sPendingText)
		{
			m_Query.m_sText = m_sPendingText;
			m_iOffset = 0;
			Refresh();
		}
		m_fRefresh += timeSlice;
		if (m_fRefresh >= 0.5 && (m_iIndexed != m_Catalogue.Count() || m_bFinished != m_Catalogue.IsFinished()))
			Refresh();
		// Only visible cells, one request per frame. A new query cancels the old page queue.
		if (m_iPreview < m_aPage.Count())
		{
			ChimeraWorld world = GetGame().GetWorld();
			ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
			if (manager)
			{
				ItemPreviewWidget preview = ItemPreviewWidget.Cast(m_aCells[m_iPreview].FindAnyWidget("ItemPreview"));
				manager.SetPreviewItemFromPrefab(preview, m_aPage[m_iPreview].m_sPrefab, null, false, 1000);
				preview.SetVisible(true);
			}
			m_iPreview++;
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetSlot(string slot)
	{
		if (m_Query.m_sEquipmentSlot == slot && m_Query.m_sCategory.IsEmpty())
			return;
		m_Query.m_sCategory = string.Empty;
		m_sCategoryLabel = "All items";
		m_Query.m_sEquipmentSlot = slot;
		m_iOffset = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		array<GC_ItemCatalogueEntry> previousPage = {};
		previousPage.Copy(m_aPage);
		m_iTotal = m_Catalogue.Search(m_Query, m_iOffset, PAGE_SIZE, m_aPage);
		bool changed = previousPage.Count() != m_aPage.Count();
		if (!changed)
		{
			foreach (int position, GC_ItemCatalogueEntry entry : m_aPage)
			{
				if (entry.m_sPrefab != previousPage[position].m_sPrefab)
					changed = true;
			}
		}
		m_iIndexed = m_Catalogue.Count();
		m_bFinished = m_Catalogue.IsFinished();
		m_fRefresh = 0;
		if (changed)
			m_iPreview = 0;
		foreach (int index, Widget cell : m_aCells)
		{
			bool populated = index < m_aPage.Count();
			cell.SetVisible(populated);
			if (populated)
			{
				TextWidget.Cast(cell.FindAnyWidget("ItemName")).SetText(m_aPage[index].m_sName);
				TextWidget.Cast(cell.FindAnyWidget("ItemCategory")).SetText(m_aPage[index].GetCategoryLabel());
			}
			if (changed)
				cell.FindAnyWidget("ItemPreview").SetVisible(false);
		}
		string filter = m_Query.m_sEquipmentSlot;
		if (filter.IsEmpty())
			filter = m_sCategoryLabel;
		if (filter == "All items")
			filter = "All";
		else
		{
			filter.Replace("Loadout", "");
			filter.Replace("Area", "");
			filter.Replace("HeadCover", "Headwear");
		}
		TextWidget.Cast(m_Root.FindAnyWidget("FilterText")).SetText("#GC-Catalogue_" + filter);
		foreach (SCR_ButtonBaseComponent button : m_aButtons)
		{
			string name = button.GetRootWidget().GetName();
			if (name.StartsWith("Category"))
			{
				string selected = "Category" + m_sCategoryLabel;
				if (m_sCategoryLabel == "All items")
					selected = "CategoryAll";
				if (m_Query.m_sEquipmentSlot == "LoadoutHeadCoverArea")
					selected = "CategoryHeadwear";
				else if (m_Query.m_sEquipmentSlot == "LoadoutBackpackArea")
					selected = "CategoryBackpacks";
				else if (m_Query.m_sEquipmentSlot == "LoadoutVestArea")
					selected = "CategoryVests";
				else if (!m_Query.m_sEquipmentSlot.IsEmpty())
					selected = string.Empty;
				button.SetToggled(name == selected, false, false);
			}
		}
		string options = WidgetManager.Translate("#GC-Catalogue_Filters");
		if (!m_Query.m_sFaction.IsEmpty())
			options = m_Query.m_sFaction;
		if (m_Query.m_bDescending)
			options += " / Z-A  v";
		else
			options += " / A-Z  v";
		TextWidget.Cast(m_Root.FindAnyWidget("Advanced").FindAnyWidget("Text")).SetText(options);
		string status = string.Format("%1-%2 / %3", m_iOffset + 1, m_iOffset + m_aPage.Count(), m_iTotal);
		if (m_iTotal == 0)
			status = WidgetManager.Translate("#GC-Catalogue_NoMatches");
		if (!m_bFinished)
			status += " / " + WidgetManager.Translate("#GC-Catalogue_Indexing");
		else if (!m_Catalogue.HasCompleteCoverage())
			status += " / " + WidgetManager.Translate("#GC-Catalogue_Partial");
		TextWidget.Cast(m_Root.FindAnyWidget("Status")).SetText(status);
		m_Paging.m_OnChanged.Remove(OnPageChanged);
		m_Paging.SetPageCount(Math.Max(1, Math.Ceil(m_iTotal / (float)PAGE_SIZE)), false);
		m_Paging.SetCurrentItem(m_iOffset / PAGE_SIZE, false);
		m_Paging.m_OnChanged.Insert(OnPageChanged);
		if (changed)
			TextWidget.Cast(m_Root.FindAnyWidget("Detail")).SetText(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ProcessClick(Widget w)
	{
		if (w.GetName() == "Advanced")
		{
			Widget panel = m_Root.FindAnyWidget("AdvancedPanel");
			if (panel.IsVisible())
			{
				m_Faction.CloseList();
				m_Sort.CloseList();
			}
			panel.SetVisible(!panel.IsVisible());
			return true;
		}
		if (w.GetName().StartsWith("Category"))
		{
			string category = w.GetName();
			category.Replace("Category", "");
			SetSlot(string.Empty);
			m_Query.m_sCategory = string.Empty;
			m_sCategoryLabel = category;
			if (category == "All")
				m_sCategoryLabel = "All items";
			else if (category == "Headwear")
				m_Query.m_sEquipmentSlot = "LoadoutHeadCoverArea";
			else if (category == "Backpacks")
				m_Query.m_sEquipmentSlot = "LoadoutBackpackArea";
			else if (category == "Vests")
				m_Query.m_sEquipmentSlot = "LoadoutVestArea";
			else
				m_Query.m_sCategory = category;
			m_iOffset = 0;
			Refresh();
			return true;
		}
		if (w.GetName() == "ClearFilter")
		{
			m_Query.m_sEquipmentSlot = string.Empty;
			m_Query.m_sCategory = string.Empty;
			m_sCategoryLabel = "All items";
			m_iOffset = 0;
			Refresh();
			return true;
		}
		foreach (int index, Widget cell : m_aCells)
		{
			if (index < m_aPage.Count() && w == cell.FindAnyWidget("SelectItem"))
			{
				SCR_InventoryMenuUI.GetInventoryMenu().GC_ClickCatalogueItem(m_aPage[index]);
				return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	GC_ItemCatalogueEntry EntryAt(Widget widget)
	{
		foreach (int index, Widget cell : m_aCells)
		{
			if (index < m_aPage.Count() && widget == cell.FindAnyWidget("SelectItem"))
				return m_aPage[index];
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	void TransferResult(bool success)
	{
		string label = "#GC-Catalogue_CannotPlace";
		if (success)
			label = "#GC-Catalogue_Added";
		TextWidget.Cast(m_Root.FindAnyWidget("Detail")).SetText(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPageChanged(SCR_SpinBoxPagingComponent paging, int page)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !GC_WorkbenchItemBrowser.IsActive(controller.GetControlledEntity()))
			return;
		m_iOffset = page * PAGE_SIZE;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_Paging || m_Faction.IsOpened() || m_Sort.IsOpened())
			return false;
		if (wheel > 0)
			m_Paging.SetCurrentItem(m_Paging.GetCurrentIndex() - 1);
		else if (wheel < 0)
			m_Paging.SetCurrentItem(m_Paging.GetCurrentIndex() + 1);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void BindButton(Widget widget)
	{
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(widget.FindHandler(SCR_ButtonBaseComponent));
		if (widget.GetName().StartsWith("Category"))
			button.SetToggleable(true);
		button.m_OnClicked.Insert(OnButton);
		m_aButtons.Insert(button);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnButton(SCR_ButtonBaseComponent button)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !GC_WorkbenchItemBrowser.IsActive(controller.GetControlledEntity()))
			return;
		ProcessClick(button.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOptionsChanged(SCR_ComboBoxComponent combo, int item)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !GC_WorkbenchItemBrowser.IsActive(controller.GetControlledEntity()))
			return;
		int faction = m_Faction.GetCurrentIndex();
		m_Query.m_sFaction = string.Empty;
		if (faction > 0 && faction <= m_Catalogue.GetFactionKeys().Count())
			m_Query.m_sFaction = m_Catalogue.GetFactionKeys()[faction - 1];
		else if (faction > 0)
			m_Query.m_sFaction = "Unassigned";
		m_Query.m_bDescending = m_Sort.GetCurrentIndex() == 1;
		m_iOffset = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w.GetName().StartsWith("Category"))
		{
			string label = w.GetName();
			label.Replace("Category", "");
			TextWidget.Cast(m_Root.FindAnyWidget("Detail")).SetText("#GC-Catalogue_" + label);
		}
		foreach (int index, Widget cell : m_aCells)
		{
			if (index < m_aPage.Count() && w == cell.FindAnyWidget("SelectItem"))
				TextWidget.Cast(m_Root.FindAnyWidget("Detail")).SetText(m_aPage[index].m_sName);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	string State()
	{
		return string.Format("category=" + m_Query.m_sCategory + " faction=" + m_Query.m_sFaction + " descending=" + m_Query.m_bDescending + " slot=%1 text=%2 offset=%3 total=%4 visible=%5 previews=%6 indexed=%7 finished=%8", m_Query.m_sEquipmentSlot, m_Query.m_sText, m_iOffset, m_iTotal, m_aPage.Count(), m_iPreview, m_iIndexed, m_bFinished);
	}
}
#endif
