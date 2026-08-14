class GC_LobbyInventoryPreviewComponent : ScriptedWidgetComponent
{
	protected static const float WEAPON_PREVIEW_ZOOM_INCREMENT = -10;
	protected static const int WEAPON_POSITION_GRENADE = 0;
	protected static const int WEAPON_POSITION_PRIMARY = 2;
	protected static const int WEAPON_POSITION_SECONDARY = 3;
	protected static const int WEAPON_POSITION_SIDEARM = 4;

	protected Widget m_wRoot;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected ref GC_LobbyInventoryCellPreview m_PrimaryWeaponPreview;
	protected ref GC_LobbyInventoryCellPreview m_SecondaryWeaponPreview;
	protected ref GC_LobbyInventoryCellPreview m_SidearmPreview;
	protected ref GC_LobbyInventoryCellPreview m_GrenadePreview;
	protected ref GC_LobbyInventoryCellPreview m_MapPreview;
	protected ref GC_LobbyInventoryCellPreview m_BinocularsPreview;
	protected ref GC_LobbyInventoryCellPreview m_CompassPreview;
	protected ref GC_LobbyInventoryCellPreview m_LightPreview;
	protected ref GC_LobbyInventoryCellPreview m_RadioPreview;
	protected ref GC_LobbyInventoryCellPreview m_WatchPreview;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;

		ChimeraWorld world = GetGame().GetWorld();
		if (world)
			m_PreviewManager = world.GetItemPreviewManager();

		m_PrimaryWeaponPreview = CreateCellPreview("PrimaryWeapon", "WeaponPreview", WEAPON_PREVIEW_ZOOM_INCREMENT);
		m_SecondaryWeaponPreview = CreateCellPreview("SecondaryWeapon", "WeaponPreview", WEAPON_PREVIEW_ZOOM_INCREMENT);
		m_SidearmPreview = CreateCellPreview("Sidearm", "WeaponPreview");
		m_GrenadePreview = CreateCellPreview("Grenade", "ItemPreview");
		m_MapPreview = CreateCellPreview("Map", "ItemPreview");
		m_BinocularsPreview = CreateCellPreview("Binoculars", "ItemPreview");
		m_CompassPreview = CreateCellPreview("Compass", "ItemPreview");
		m_LightPreview = CreateCellPreview("Light", "ItemPreview");
		m_RadioPreview = CreateCellPreview("Radio", "ItemPreview");
		m_WatchPreview = CreateCellPreview("Watch", "ItemPreview");

		ResetPreview();
		PrintFormat("GC | Lobby inventory preview attached");
	}

	override void HandlerDeattached(Widget w)
	{
		ResetPreview();
		m_PreviewManager = null;
		m_wRoot = null;
		super.HandlerDeattached(w);
	}

	void SetCharacter(IEntity character)
	{
		ResetPreview();
		if (!character)
			return;

		if (!m_PreviewManager)
		{
			ChimeraWorld world = GetGame().GetWorld();
			if (world)
				m_PreviewManager = world.GetItemPreviewManager();
		}

		if (!m_PreviewManager)
		{
			PrintFormat("GC | Lobby inventory has no item preview manager");
			return;
		}

		PopulateWeaponPreviews(character);
		PopulateEquipmentPreviews(character);
	}

	void ResetPreview()
	{
		SetCellItem(m_PrimaryWeaponPreview, null);
		SetCellItem(m_SecondaryWeaponPreview, null);
		SetCellItem(m_SidearmPreview, null);
		SetCellItem(m_GrenadePreview, null);
		SetCellItem(m_MapPreview, null);
		SetCellItem(m_BinocularsPreview, null);
		SetCellItem(m_CompassPreview, null);
		SetCellItem(m_LightPreview, null);
		SetCellItem(m_RadioPreview, null);
		SetCellItem(m_WatchPreview, null);
	}

	protected void PopulateWeaponPreviews(IEntity character)
	{
		CharacterWeaponManagerComponent weaponManager = CharacterWeaponManagerComponent.Cast(character.FindComponent(CharacterWeaponManagerComponent));
		if (!weaponManager)
		{
			PrintFormat("GC | Inventory preview character has no weapon manager");
			return;
		}

		array<WeaponSlotComponent> weaponSlots = {};
		weaponManager.GetWeaponsSlots(weaponSlots);
		SetCellItem(m_PrimaryWeaponPreview, GetWeaponEntityAt(weaponSlots, WEAPON_POSITION_PRIMARY));
		SetCellItem(m_SecondaryWeaponPreview, GetWeaponEntityAt(weaponSlots, WEAPON_POSITION_SECONDARY));
		SetCellItem(m_SidearmPreview, GetWeaponEntityAt(weaponSlots, WEAPON_POSITION_SIDEARM));
		SetCellItem(m_GrenadePreview, GetWeaponEntityAt(weaponSlots, WEAPON_POSITION_GRENADE));

		PrintFormat("GC | Inventory preview resolved %1 weapon slots", weaponSlots.Count());
	}

	protected IEntity GetWeaponEntityAt(array<WeaponSlotComponent> weaponSlots, int position)
	{
		if (position < 0 || position >= weaponSlots.Count())
			return null;

		WeaponSlotComponent weaponSlot = weaponSlots[position];
		if (!weaponSlot)
			return null;

		return weaponSlot.GetWeaponEntity();
	}

	protected void PopulateEquipmentPreviews(IEntity character)
	{
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
		{
			PrintFormat("GC | Inventory preview character has no inventory manager");
			return;
		}

		SetCellItem(m_MapPreview, inventoryManager.FindItemWithComponents({SCR_MapGadgetComponent}, EStoragePurpose.PURPOSE_ANY));
		SetCellItem(m_BinocularsPreview, inventoryManager.FindItemWithComponents({SCR_BinocularsComponent}, EStoragePurpose.PURPOSE_ANY));
		SetCellItem(m_CompassPreview, inventoryManager.FindItemWithComponents({SCR_CompassComponent}, EStoragePurpose.PURPOSE_ANY));
		SetCellItem(m_LightPreview, inventoryManager.FindItemWithComponents({SCR_FlashlightComponent}, EStoragePurpose.PURPOSE_ANY));
		SetCellItem(m_RadioPreview, inventoryManager.FindItemWithComponents({SCR_RadioComponent}, EStoragePurpose.PURPOSE_ANY));
		SetCellItem(m_WatchPreview, inventoryManager.FindItemWithComponents({SCR_WristwatchComponent}, EStoragePurpose.PURPOSE_ANY));
	}

	protected GC_LobbyInventoryCellPreview CreateCellPreview(string cellName, string previewWidgetName, float cameraZoomIncrement = 0)
	{
		Widget cellRoot = m_wRoot.FindAnyWidget(cellName);
		if (!cellRoot)
		{
			PrintFormat("GC | Inventory preview cell '%1' not found", cellName);
			return null;
		}

		GC_LobbyInventoryCellPreview preview = new GC_LobbyInventoryCellPreview(cellRoot, previewWidgetName, cameraZoomIncrement);
		if (!preview.IsValid())
			PrintFormat("GC | Inventory preview widget '%1/%2' not found", cellName, previewWidgetName);

		return preview;
	}

	protected void SetCellItem(GC_LobbyInventoryCellPreview cellPreview, IEntity itemEntity)
	{
		if (cellPreview)
			cellPreview.SetItemEntity(m_PreviewManager, itemEntity);
	}
}
