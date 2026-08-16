class GC_LobbyInventoryPreviewComponent : ScriptedWidgetComponent
{
	protected static const float WEAPON_PREVIEW_ZOOM_INCREMENT = -10;
	protected static const int WEAPON_POSITION_GRENADE = 0;
	protected static const int WEAPON_POSITION_PRIMARY = 2;
	protected static const int WEAPON_POSITION_SECONDARY = 3;
	protected static const int WEAPON_POSITION_SIDEARM = 4;

	protected Widget m_wRoot;
	protected SCR_ButtonBaseComponent m_OpenButton;
	protected GC_LobbyInventoryDetailComponent m_InventoryDetail;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected IEntity m_Character;
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

		Widget openButton = w.FindAnyWidget("InventoryOpenButton");
		if (openButton)
		{
			m_OpenButton = SCR_ButtonBaseComponent.Cast(openButton.FindHandler(SCR_ButtonBaseComponent));
			if (m_OpenButton)
				m_OpenButton.m_OnClicked.Insert(OnOpenClicked);
		}

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
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_OpenButton)
			m_OpenButton.m_OnClicked.Remove(OnOpenClicked);

		ResetPreview();
		m_OpenButton = null;
		m_InventoryDetail = null;
		m_PreviewManager = null;
		m_wRoot = null;
		super.HandlerDeattached(w);
	}

	void SetInventoryDetail(GC_LobbyInventoryDetailComponent inventoryDetail)
	{
		m_InventoryDetail = inventoryDetail;
		if (m_InventoryDetail)
			m_InventoryDetail.SetCharacter(m_Character, m_PreviewManager);
	}

	void SetCharacter(IEntity character)
	{
		ResetPreview();
		if (!character)
			return;

		m_Character = character;

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

		if (m_InventoryDetail)
			m_InventoryDetail.SetCharacter(m_Character, m_PreviewManager);

		PopulateWeaponPreviews(character);
		PopulateEquipmentPreviews(character);
	}

	void ResetPreview()
	{
		m_Character = null;
		if (m_InventoryDetail)
		{
			m_InventoryDetail.Close();
			m_InventoryDetail.SetCharacter(null, m_PreviewManager);
		}

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

	protected void OnOpenClicked(SCR_ButtonBaseComponent button)
	{
		if (!m_InventoryDetail || !m_Character)
			return;

		m_InventoryDetail.Open();
	}
}
