enum GC_LobbyInventoryCategory
{
	WEAPONS,
	MEDICAL_CONSUMABLES,
	EQUIPMENT_TOOLS,
	ATTACHMENTS,
	OTHER,
	COUNT
}

class GC_LobbyInventoryDetailComponent : ScriptedWidgetComponent
{
	protected static const ResourceName CATEGORY_LAYOUT = "{6A32000000000004}UI/layouts/Inventory/Lobby_Inventory_Detail_Category.layout";
	protected static const ResourceName ROW_LAYOUT = "{6A32000000000002}UI/layouts/Inventory/Lobby_Inventory_Detail_Row.layout";
	protected static const ResourceName CELL_LAYOUT = "{6A32000000000003}UI/layouts/Inventory/Lobby_Inventory_Detail_Item.layout";
	protected static const int ITEMS_PER_ROW = 6;
	protected static const float NAME_TOOLTIP_MAX_WIDTH = 500;
	protected static const float NAME_TOOLTIP_OFFSET = 4;

	protected Widget m_wRoot;
	protected Widget m_wRows;
	protected ScrollLayoutWidget m_wScroll;
	protected Widget m_wNameTooltipLayer;
	protected Widget m_wNameTooltip;
	protected TextWidget m_wNameTooltipText;
	protected Widget m_wNameTooltipSource;
	protected ref array<SCR_ButtonBaseComponent> m_NameHoverButtons = {};
	protected ref array<Widget> m_NameHoverTargets = {};
	protected ref array<string> m_NameHoverTexts = {};
	protected SCR_ButtonBaseComponent m_DismissButton;
	protected SCR_ButtonBaseComponent m_CloseButton;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected IEntity m_Character;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wRoot = w;
		m_wRows = w.FindAnyWidget("InventoryDetailRows");
		m_wScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("InventoryDetailScroll"));
		m_wNameTooltipLayer = w.FindAnyWidget("InventoryNameTooltipLayer");
		m_wNameTooltip = w.FindAnyWidget("InventoryNameTooltip");
		m_wNameTooltipText = TextWidget.Cast(w.FindAnyWidget("InventoryNameTooltipText"));
		HideItemNameTooltip(null);

		Widget dismissButton = w.FindAnyWidget("InventoryDismissButton");
		if (dismissButton)
		{
			m_DismissButton = SCR_ButtonBaseComponent.Cast(dismissButton.FindHandler(SCR_ButtonBaseComponent));
			if (m_DismissButton)
				m_DismissButton.m_OnClicked.Insert(OnDismissClicked);
		}

		Widget closeButton = w.FindAnyWidget("CloseButton");
		if (closeButton)
		{
			m_CloseButton = SCR_ButtonBaseComponent.Cast(closeButton.FindHandler(SCR_ButtonBaseComponent));
			if (m_CloseButton)
				m_CloseButton.m_OnClicked.Insert(OnCloseClicked);
		}

		m_wRoot.SetVisible(false);
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_DismissButton)
			m_DismissButton.m_OnClicked.Remove(OnDismissClicked);

		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Remove(OnCloseClicked);

		ClearRows();
		m_DismissButton = null;
		m_CloseButton = null;
		m_PreviewManager = null;
		m_Character = null;
		m_wScroll = null;
		m_wRows = null;
		m_wNameTooltipSource = null;
		m_wNameTooltipText = null;
		m_wNameTooltip = null;
		m_wNameTooltipLayer = null;
		m_wRoot = null;

		super.HandlerDeattached(w);
	}

	void SetCharacter(IEntity character, ItemPreviewManagerEntity previewManager)
	{
		m_Character = character;
		m_PreviewManager = previewManager;

		if (IsOpen())
		{
			Refresh();
			ResetScrollPosition("character refresh");
		}
	}

	void Open()
	{
		if (!m_wRoot || !m_Character || !m_PreviewManager)
		{
			PrintFormat("GC | Lobby inventory detail cannot open: root=%1 character=%2 previewManager=%3", m_wRoot, m_Character, m_PreviewManager);
			return;
		}

		Refresh();
		m_wRoot.SetVisible(true);
		ResetScrollPosition("open");
		PrintFormat("GC | Lobby inventory detail opened for %1", m_Character);
	}

	void Close()
	{
		if (!IsOpen())
			return;

		ResetScrollPosition("close");
		m_wRoot.SetVisible(false);
		ClearRows();
		PrintFormat("GC | Lobby inventory detail closed");
	}

	bool IsOpen()
	{
		return m_wRoot && m_wRoot.IsVisible();
	}

	protected void Refresh()
	{
		ClearRows();
		if (!m_wRows || !m_Character || !m_PreviewManager)
			return;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(m_Character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
		{
			PrintFormat("GC | Lobby inventory detail character has no inventory manager: %1", m_Character);
			return;
		}

		array<IEntity> items = {};
		inventoryManager.GetItems(items, EStoragePurpose.PURPOSE_ANY);

		array<IEntity> internalEquipmentAttachments = {};
		inventoryManager.GetItems(internalEquipmentAttachments, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);

		array<IEntity> uniqueItems = {};
		int duplicateReferenceCount;
		int internalEquipmentAttachmentCount;
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			if (internalEquipmentAttachments.Find(item) != -1)
			{
				internalEquipmentAttachmentCount++;
				continue;
			}

			if (uniqueItems.Find(item) != -1)
			{
				duplicateReferenceCount++;
				continue;
			}

			uniqueItems.Insert(item);
		}

		array<IEntity> groupedItems = {};
		array<ResourceName> groupedPrefabNames = {};
		array<int> groupedItemCounts = {};
		array<int> groupedItemCategories = {};
		foreach (IEntity item : uniqueItems)
		{
			ResourceName prefabName = GetItemPrefabName(item);

			int groupIndex = -1;
			if (prefabName != string.Empty)
				groupIndex = groupedPrefabNames.Find(prefabName);

			if (groupIndex == -1)
			{
				groupedItems.Insert(item);
				groupedPrefabNames.Insert(prefabName);
				groupedItemCounts.Insert(1);
				groupedItemCategories.Insert(GetItemCategory(item));
				continue;
			}

			groupedItemCounts[groupIndex] = groupedItemCounts[groupIndex] + 1;
		}

		for (int category = 0; category < GC_LobbyInventoryCategory.COUNT; category++)
		{
			Widget row;
			int categoryItemCount;
			for (int i = 0; i < groupedItems.Count(); i++)
			{
				if (groupedItemCategories[i] != category)
					continue;

				if (categoryItemCount == 0)
					CreateCategoryHeader(category);

				if (categoryItemCount % ITEMS_PER_ROW == 0)
					row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wRows);

				if (!row)
					break;

				CreateItemCell(row, groupedItems[i], groupedItemCounts[i]);
				categoryItemCount++;
			}
		}

		PrintFormat("GC | Lobby inventory detail populated %1 categorized item groups from %2 unique entities and %3 storage entries; filtered %4 internal equipment attachments and skipped %5 duplicate references", groupedItems.Count(), uniqueItems.Count(), items.Count(), internalEquipmentAttachmentCount, duplicateReferenceCount);
	}

	protected int GetItemCategory(IEntity item)
	{
		if (!item)
			return GC_LobbyInventoryCategory.OTHER;

		if (item.FindComponent(InventoryMagazineComponent) || item.FindComponent(BaseMagazineComponent))
			return GC_LobbyInventoryCategory.WEAPONS;

		if (item.FindComponent(SCR_ExplosiveChargeInventoryItemComponent) || item.FindComponent(SCR_MineInventoryItemComponent))
			return GC_LobbyInventoryCategory.WEAPONS;

		if (item.FindComponent(BaseWeaponComponent))
			return GC_LobbyInventoryCategory.WEAPONS;

		if (item.FindComponent(SCR_ConsumableItemComponent))
			return GC_LobbyInventoryCategory.MEDICAL_CONSUMABLES;

		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (inventoryItem && inventoryItem.FindAttribute(WeaponAttachmentAttributes))
			return GC_LobbyInventoryCategory.ATTACHMENTS;

		if (item.FindComponent(SCR_GadgetComponent))
			return GC_LobbyInventoryCategory.EQUIPMENT_TOOLS;

		return GC_LobbyInventoryCategory.OTHER;
	}

	protected void CreateCategoryHeader(int category)
	{
		Widget header = GetGame().GetWorkspace().CreateWidgets(CATEGORY_LAYOUT, m_wRows);
		if (!header)
			return;

		TextWidget nameText = TextWidget.Cast(header.FindAnyWidget("CategoryName"));
		if (nameText)
			nameText.SetText(GetCategoryName(category));
	}

	protected string GetCategoryName(int category)
	{
		switch (category)
		{
			case GC_LobbyInventoryCategory.WEAPONS:
				return "WEAPONS";
			case GC_LobbyInventoryCategory.MEDICAL_CONSUMABLES:
				return "MEDICAL & CONSUMABLES";
			case GC_LobbyInventoryCategory.EQUIPMENT_TOOLS:
				return "EQUIPMENT & TOOLS";
			case GC_LobbyInventoryCategory.ATTACHMENTS:
				return "ATTACHMENTS";
		}

		return "OTHER";
	}

	protected void CreateItemCell(Widget row, IEntity item, int itemCount)
	{
		if (!row || !item)
			return;

		Widget cell = GetGame().GetWorkspace().CreateWidgets(CELL_LAYOUT, row);
		if (!cell)
			return;

		ItemPreviewWidget preview = ItemPreviewWidget.Cast(cell.FindAnyWidget("ItemPreview"));
		if (preview)
		{
			RenderTargetWidget renderTarget = preview;
			renderTarget.SetClearColor(false, 0);
			renderTarget.SetBlendMode(RenderTargetWidgetBlendMode.ALPHA_BLEND);
			m_PreviewManager.SetPreviewItem(preview, item, GetPreviewAttributes(item), true);
		}

		TextWidget nameText = TextWidget.Cast(cell.FindAnyWidget("ItemName"));
		if (!nameText)
			return;

		string itemName = GetItemDisplayName(item);
		if (itemName.IsEmpty())
		{
			PrintFormat("GC | Lobby inventory detail found unnamed visible item: prefab=%1 entity=%2", GetItemPrefabName(item), item);
			itemName = "UNNAMED ITEM";
		}

		nameText.SetText(itemName);

		Widget hoverTarget = cell.FindAnyWidget("ItemNameHoverTarget");
		SCR_ButtonBaseComponent hoverComponent;
		if (hoverTarget)
			hoverComponent = SCR_ButtonBaseComponent.Cast(hoverTarget.FindHandler(SCR_ButtonBaseComponent));

		if (hoverComponent)
		{
			hoverComponent.m_OnShowBorder.Insert(OnItemNameHoverChanged);
			m_NameHoverButtons.Insert(hoverComponent);
			m_NameHoverTargets.Insert(hoverTarget);
			m_NameHoverTexts.Insert(itemName);
		}

		Widget countBadge = cell.FindAnyWidget("ItemCountBadge");
		TextWidget countText = TextWidget.Cast(cell.FindAnyWidget("ItemCount"));
		if (!countBadge || !countText)
			return;

		bool showCount = itemCount > 1;
		countBadge.SetVisible(showCount);
		if (showCount)
			countText.SetTextFormat("x%1", itemCount);
	}

	protected string GetItemDisplayName(IEntity item)
	{
		if (!item)
			return string.Empty;

		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		UIInfo info;
		if (inventoryItem)
			info = inventoryItem.GetUIInfo();

		if (info && !info.GetName().IsEmpty())
			return info.GetName();

		return item.GetName();
	}

	protected ResourceName GetItemPrefabName(IEntity item)
	{
		if (!item)
			return string.Empty;

		EntityPrefabData prefabData = item.GetPrefabData();
		if (!prefabData)
			return string.Empty;

		return prefabData.GetPrefabName();
	}

	protected PreviewRenderAttributes GetPreviewAttributes(IEntity item)
	{
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!inventoryItem)
			return null;

		return PreviewRenderAttributes.Cast(inventoryItem.FindAttribute(PreviewRenderAttributes));
	}

	protected void ClearRows()
	{
		HideItemNameTooltip(null);
		foreach (SCR_ButtonBaseComponent hoverButton : m_NameHoverButtons)
		{
			if (hoverButton)
				hoverButton.m_OnShowBorder.Remove(OnItemNameHoverChanged);
		}

		m_NameHoverButtons.Clear();
		m_NameHoverTargets.Clear();
		m_NameHoverTexts.Clear();

		if (!m_wRows)
			return;

		Widget child = m_wRows.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			child.RemoveFromHierarchy();
			child = next;
		}
	}

	protected void OnItemNameHoverChanged(SCR_ButtonBaseComponent button, bool hovered)
	{
		int index = m_NameHoverButtons.Find(button);
		if (index == -1)
			return;

		if (hovered)
			ShowItemNameTooltip(m_NameHoverTargets[index], m_NameHoverTexts[index]);
		else
			HideItemNameTooltip(m_NameHoverTargets[index]);
	}

	void ShowItemNameTooltip(Widget source, string itemName)
	{
		if (!source || itemName.IsEmpty() || !m_wNameTooltipLayer || !m_wNameTooltip || !m_wNameTooltipText)
			return;

		m_wNameTooltipSource = source;
		m_wNameTooltipText.SetText(itemName);

		float sourceX;
		float sourceY;
		float sourceWidth;
		float sourceHeight;
		float layerX;
		float layerY;
		float layerWidth;
		float layerHeight;
		source.GetScreenPos(sourceX, sourceY);
		source.GetScreenSize(sourceWidth, sourceHeight);
		m_wNameTooltipLayer.GetScreenPos(layerX, layerY);
		m_wNameTooltipLayer.GetScreenSize(layerWidth, layerHeight);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float localX = workspace.DPIUnscale(sourceX - layerX);
		float localY = workspace.DPIUnscale(sourceY + sourceHeight - layerY) + NAME_TOOLTIP_OFFSET;
		float availableWidth = workspace.DPIUnscale(layerWidth);
		if (localX + NAME_TOOLTIP_MAX_WIDTH > availableWidth)
			localX = availableWidth - NAME_TOOLTIP_MAX_WIDTH;

		if (localX < 0)
			localX = 0;

		FrameSlot.SetPos(m_wNameTooltip, localX, localY);
		m_wNameTooltip.SetVisible(true);
	}

	void HideItemNameTooltip(Widget source)
	{
		if (source && source != m_wNameTooltipSource)
			return;

		m_wNameTooltipSource = null;
		if (m_wNameTooltip)
			m_wNameTooltip.SetVisible(false);
	}

	protected void ResetScrollPosition(string reason)
	{
		if (!m_wScroll)
		{
			PrintFormat("GC | Lobby inventory detail cannot reset scroll (%1): InventoryDetailScroll not found", reason);
			return;
		}

		float previousX;
		float previousY;
		m_wScroll.GetSliderPos(previousX, previousY);
		m_wScroll.SetSliderPos(0, 0);
		if (previousX > 0.001 || previousY > 0.001)
			PrintFormat("GC | Lobby inventory detail scroll reset (%1): (%2, %3) -> (0, 0)", reason, previousX, previousY);
	}

	protected void OnCloseClicked(SCR_ButtonBaseComponent button)
	{
		Close();
	}

	protected void OnDismissClicked(SCR_ButtonBaseComponent button)
	{
		Close();
	}
}
