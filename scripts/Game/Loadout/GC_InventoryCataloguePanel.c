#ifdef WORKBENCH
//! Search presentation and bounded previews inside the native inventory menu.
class GC_InventoryCataloguePanel : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected Widget m_Vicinity;
	protected Widget m_Opened;
	protected EditBoxWidget m_Search;
	protected Widget m_ClearSearch;
	protected SCR_ComboBoxComponent m_Sort;
	protected SCR_ComboBoxComponent m_Mod;
	protected ref array<string> m_aModIDs = {""};
	protected ref map<string, string> m_mModTitles = new map<string, string>();
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
	protected float m_fFeedbackTime;
	protected bool m_bFinished;
	protected SCR_SpinBoxPagingComponent m_Paging;
	protected float m_fVicinityOpacity;
	protected float m_fOpenedOpacity;
	protected bool m_bPagingActive;
	protected ref array<SCR_ButtonBaseComponent> m_aButtons = {};
	protected string m_sCategoryLabel = "All items";
	protected BaseInventoryStorageComponent m_WeaponStorage;
	protected IEntity m_WeaponEntity;
	protected InventoryStorageSlot m_InspectionSlot;
	protected bool m_bInspectionContext;
	protected int m_iWeaponSlot;
	protected string m_sWeaponSlotType;
	protected string m_sWeaponSignature;
	protected float m_fWeaponRefresh;
	protected static const int PAGE_SIZE = 10;

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
		array<string> buttonNames = {"CategoryAll", "CategoryWeapons", "CategoryMagazines", "CategoryGrenades", "CategoryHeadwear", "CategoryClothing", "CategoryVests", "CategoryBackpacks", "CategoryEquipment", "CategoryAttachments"};
		foreach (string buttonName : buttonNames)
			BindButton(m_Root.FindAnyWidget(buttonName));
		m_Paging = SCR_SpinBoxPagingComponent.GetSpinBoxPagingComponent("CataloguePaging", m_Root);
		m_Paging.m_OnChanged.Insert(OnPageChanged);
		m_Search = EditBoxWidget.Cast(m_Root.FindAnyWidget("CatalogueSearch"));
		m_ClearSearch = m_Root.FindAnyWidget("ClearSearch");
		BindButton(m_ClearSearch);
		m_Sort = SCR_ComboBoxComponent.Cast(m_Root.FindAnyWidget("CatalogueSort").FindHandler(SCR_ComboBoxComponent));
		m_Mod = SCR_ComboBoxComponent.Cast(m_Root.FindAnyWidget("CatalogueMod").FindHandler(SCR_ComboBoxComponent));
		m_Sort.AddItem("A–Z");
		m_Sort.AddItem("Z–A");
		m_Sort.SetCurrentItem(0, false, false, false);
		m_Mod.AddItem("All mods");
		m_Mod.SetCurrentItem(0, false, false, false);
		m_Sort.m_OnChanged.Insert(OnSearchOption);
		m_Mod.m_OnChanged.Insert(OnSearchOption);
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
		Refresh();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Detach()
	{
		GC_CatalogueTooltip.HideCurrent();
		if (m_Sort)
		{
			m_Sort.m_OnChanged.Remove(OnSearchOption);
			m_Sort.CloseList();
		}
		if (m_Mod)
		{
			m_Mod.m_OnChanged.Remove(OnSearchOption);
			m_Mod.CloseList();
		}
		foreach (SCR_ButtonBaseComponent button : m_aButtons)
			button.m_OnClicked.Remove(OnButton);
		m_aButtons.Clear();
		if (m_Paging)
		{
			m_Paging.m_OnChanged.Remove(OnPageChanged);
			m_Paging.SetButtonsActive(false);
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
		m_fWeaponRefresh += timeSlice;
		if (m_WeaponStorage && m_fWeaponRefresh >= 0.25)
		{
			m_fWeaponRefresh = 0;
			UpdateWeaponFilter();
		}
		if (m_fFeedbackTime > 0)
		{
			m_fFeedbackTime -= timeSlice;
			if (m_fFeedbackTime <= 0)
				Refresh();
		}
		bool pagingActive = GetGame().GetWorkspace().GetFocusedWidget() != m_Search && !m_Sort.IsOpened() && !m_Mod.IsOpened();
		if (pagingActive != m_bPagingActive)
		{
			m_bPagingActive = pagingActive;
			m_Paging.SetButtonsActive(pagingActive);
		}
		string text = m_Search.GetText();
		m_ClearSearch.SetVisible(!text.IsEmpty());
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
		if (m_bFinished != m_Catalogue.IsFinished() || (m_fRefresh >= 0.5 && m_iIndexed != m_Catalogue.Count()))
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
		bool wasWeapon = m_WeaponStorage != null;
		m_WeaponStorage = null;
		m_WeaponEntity = null;
		m_InspectionSlot = null;
		m_bInspectionContext = false;
		m_Query.m_bWeaponItems = false;
		m_Query.m_aMagazineWells.Clear();
		m_Query.m_aAttachmentTypes.Clear();
		if (!wasWeapon && m_Query.m_sEquipmentSlot == slot && m_Query.m_sCategory.IsEmpty())
			return;
		m_Query.m_sCategory = string.Empty;
		m_sCategoryLabel = "All items";
		m_Query.m_sEquipmentSlot = slot;
		m_iOffset = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void SetWeaponSlot(BaseInventoryStorageComponent storage, int slotID, string slotType)
	{
		SetSlot(string.Empty);
		m_WeaponStorage = storage;
		m_iWeaponSlot = slotID;
		m_sWeaponSlotType = slotType;
		m_sWeaponSignature = string.Empty;
		UpdateWeaponFilter();
	}

	//------------------------------------------------------------------------------------------------
	//! Read only the selected live weapon; catalogue candidates remain metadata, never spawned previews.
	protected void UpdateWeaponFilter()
	{
		if (!m_WeaponStorage || m_iWeaponSlot < 0 || m_iWeaponSlot >= m_WeaponStorage.GetSlotsCount())
			return;
		IEntity item = m_WeaponStorage.GetSlot(m_iWeaponSlot).GetAttachedEntity();
		BaseWeaponComponent weapon;
		if (item)
			weapon = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
		m_Query.m_aMagazineWells.Clear();
		m_Query.m_aAttachmentTypes.Clear();
		m_Query.m_bWeaponItems = weapon != null;
		m_WeaponEntity = item;
		string signature = "empty:" + m_sWeaponSlotType;
		if (weapon)
		{
			signature = item.GetPrefabData().GetPrefabName();
			array<BaseMuzzleComponent> muzzles = {};
			weapon.GetMuzzlesList(muzzles);
			foreach (BaseMuzzleComponent muzzle : muzzles)
			{
				BaseMagazineWell well = muzzle.GetMagazineWell();
				if (!well)
					continue;
				m_Query.m_aMagazineWells.Insert(well.Type());
				signature += "|mag:" + well.Type().ToString();
			}
			array<AttachmentSlotComponent> attachments = {};
			weapon.GetAttachments(attachments);
			foreach (AttachmentSlotComponent attachment : attachments)
			{
				BaseAttachmentType mount = attachment.GetAttachmentSlotType();
				if (!mount || !attachment.ShouldShowInInspection())
					continue;
				m_Query.m_aAttachmentTypes.Insert(mount.Type());
				signature += "|mount:" + mount.Type().ToString();
			}
		}
		if (signature == m_sWeaponSignature)
			return;
		m_sWeaponSignature = signature;
		m_Query.m_sEquipmentSlot = m_sWeaponSlotType;
		m_sCategoryLabel = "Weapons";
		if (weapon)
		{
			m_Query.m_sEquipmentSlot = string.Empty;
			m_sCategoryLabel = string.Empty;
		}
		m_iOffset = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	bool HasWeaponContext()
	{
		return m_Query.m_bWeaponItems;
	}

	//------------------------------------------------------------------------------------------------
	void SetInspectionSlot(InventoryStorageSlot slot, typename type)
	{
		SetSlot(string.Empty);
		m_iOffset = 0;
		m_InspectionSlot = slot;
		m_bInspectionContext = true;
		m_Query.m_bWeaponItems = true;
		m_sCategoryLabel = string.Empty;
		if (type && type.IsInherited(BaseMagazineWell))
			m_Query.m_aMagazineWells.Insert(type);
		else if (type)
			m_Query.m_aAttachmentTypes.Insert(type);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	bool HasInspectionContext()
	{
		return m_bInspectionContext;
	}

	//------------------------------------------------------------------------------------------------
	InventoryStorageSlot GetInspectionTarget()
	{
		return m_InspectionSlot;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetWeaponTarget()
	{
		if (!m_Query.m_bWeaponItems || !m_WeaponStorage || !m_WeaponEntity || m_iWeaponSlot < 0 || m_iWeaponSlot >= m_WeaponStorage.GetSlotsCount())
			return null;
		if (m_WeaponStorage.GetSlot(m_iWeaponSlot).GetAttachedEntity() != m_WeaponEntity)
			return null;
		return m_WeaponEntity;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearchOption(SCR_ComboBoxComponent combo, int index)
	{
		if (combo == m_Sort)
			m_Query.m_bDescending = index == 1;
		else if (m_aModIDs.IsIndexValid(index))
			m_Query.m_sSourceAddon = m_aModIDs[index];
		m_iOffset = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void PopulateMods()
	{
		array<string> contributing = {};
		for (int i; i < m_Catalogue.Count(); i++)
		{
			GC_ItemCatalogueEntry entry = m_Catalogue.Get(i);
			if (!entry.m_bPortable)
				continue;
			foreach (string source : entry.m_aSourceAddons)
			{
				if (contributing.Find(source) < 0)
					contributing.Insert(source);
			}
		}
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		array<string> keys = {};
		map<string, string> labels = new map<string, string>();
		map<string, string> ids = new map<string, string>();
		foreach (string guid : addons)
		{
			string id = GameProject.GetAddonID(guid);
			if (contributing.Find(id) < 0)
				continue;
			string label = GameProject.GetAddonTitle(guid);
			if (label.IsEmpty())
				label = id;
			m_mModTitles.Insert(id, label);
			string key = label;
			key.ToLower();
			key += "\t" + id;
			keys.Insert(key);
			labels.Insert(key, label);
			ids.Insert(key, id);
		}
		keys.Sort();
		foreach (string key : keys)
		{
			m_Mod.AddItem(labels.Get(key));
			m_aModIDs.Insert(ids.Get(key));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_bFinished && m_Catalogue.IsFinished())
			PopulateMods();
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
		{
			GC_CatalogueTooltip.HideCurrent();
			m_iPreview = 0;
		}
		foreach (int index, Widget cell : m_aCells)
		{
			bool populated = index < m_aPage.Count();
			cell.SetVisible(populated);
			if (populated)
			{
				// Match SCR_AddonTool.GetResourceLastAddon: the resolved resource's
				// last provider, rather than every inherited dependency.
				array<string> sources = m_aPage[index].m_aSourceAddons;
				string sourceText;
				if (!sources.IsEmpty())
				{
					string source = sources[sources.Count() - 1];
					sourceText = m_mModTitles.Get(source);
					if (sourceText.IsEmpty())
						sourceText = source;
				}
				TextWidget.Cast(cell.FindAnyWidget("SourceMods")).SetText(sourceText);
				TextWidget.Cast(cell.FindAnyWidget("ItemName")).SetText(m_aPage[index].m_sName);
				TextWidget.Cast(cell.FindAnyWidget("ItemCategory")).SetText(m_aPage[index].GetCategoryLabel());
			}
			if (changed)
				cell.FindAnyWidget("ItemPreview").SetVisible(false);
		}
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
				else if (m_Query.m_sEquipmentSlot == "LoadoutVestArea" || m_Query.m_sEquipmentSlot == "LoadoutArmoredVestSlotArea")
					selected = "CategoryVests";
				else if (!m_Query.m_sEquipmentSlot.IsEmpty())
					selected = string.Empty;
				button.SetToggled(name == selected, false, false);
			}
		}
		int pages = Math.Max(1, Math.Ceil(m_iTotal / (float)PAGE_SIZE));
		string status = string.Format("%1 / %2", m_iOffset + m_aPage.Count(), m_iTotal);
		if (m_fFeedbackTime <= 0)
			TextWidget.Cast(m_Root.FindAnyWidget("Status")).SetText(status);
		m_Paging.m_OnChanged.Remove(OnPageChanged);
		m_Paging.SetPageCount(pages, false);
		m_Paging.SetCurrentItem(m_iOffset / PAGE_SIZE, false);
		m_Paging.m_OnChanged.Insert(OnPageChanged);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ProcessClick(Widget w)
	{
		if (w == m_ClearSearch)
		{
			m_Search.SetText(string.Empty);
			m_sPendingText = string.Empty;
			m_Query.m_sText = string.Empty;
			m_fSearchDelay = 0;
			m_iOffset = 0;
			m_ClearSearch.SetVisible(false);
			GetGame().GetWorkspace().SetFocusedWidget(m_Search);
			Refresh();
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
			else
				m_Query.m_sCategory = category;
			m_iOffset = 0;
			Refresh();
			return true;
		}
		foreach (int index, Widget cell : m_aCells)
		{
			if (index < m_aPage.Count() && w == cell.FindAnyWidget("SelectItem"))
			{
				SCR_InventoryMenuUI.GetInventoryMenu().GC_ClickWeaponCatalogueItem(m_aPage[index]);
				return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	GC_ItemCatalogueEntry EntryAt(Widget widget)
	{
		while (widget && widget != m_Root)
		{
			foreach (int index, Widget cell : m_aCells)
			{
				if (index < m_aPage.Count() && widget == cell.FindAnyWidget("SelectItem"))
					return m_aPage[index];
			}
			widget = widget.GetParent();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	void TransferResult(bool success, bool deleting = false)
	{
		string label = "#GC-Catalogue_CannotPlace";
		if (success)
			label = "#GC-Catalogue_Added";
		if (deleting)
		{
			label = "#GC-Catalogue_CannotDelete";
			if (success)
				label = "#GC-Catalogue_Deleted";
		}
		m_fFeedbackTime = 2;
		TextWidget.Cast(m_Root.FindAnyWidget("Status")).SetText(label);
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
		if (!m_Paging || m_Sort.IsOpened() || m_Mod.IsOpened())
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
	string State()
	{
		return string.Format("category=" + m_Query.m_sCategory + " faction=" + m_Query.m_sFaction + " descending=" + m_Query.m_bDescending + " slot=%1 text=%2 offset=%3 total=%4 visible=%5 previews=%6 indexed=%7 finished=%8", m_Query.m_sEquipmentSlot, m_Query.m_sText, m_iOffset, m_iTotal, m_aPage.Count(), m_iPreview, m_iIndexed, m_bFinished);
	}
}
#endif
