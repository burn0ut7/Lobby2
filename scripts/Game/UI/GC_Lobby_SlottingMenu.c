class GC_Lobby_SlottingMenu : ChimeraMenuBase
{
	protected static const int LOADOUT_HOVER_EXIT_DELAY_MS = 100;
    protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";
    protected static const ResourceName GROUP_LAYOUT = "{6A25000000000001}UI/layouts/Groups/Lobby_Group.layout";

    protected Widget m_wRoot;
    protected GC_HeaderComponent m_Header;
    protected GC_FactionListComponent m_FactionList;
    protected GC_LobbyManager m_LobbyManager;
    protected Widget m_wGroups;
    protected ScrollLayoutWidget m_wGroupScroll;
    protected ref array<Widget> m_aGroupCards = {};
    protected FactionKey m_SelectedFactionKey;
    protected ItemPreviewWidget m_wPlayerRender;
    protected Widget m_wLoadoutInventory;
	protected GC_LobbyInventoryPreviewComponent m_InventoryPreview;
	protected GC_LobbyInventoryDetailComponent m_InventoryDetail;
    protected ItemPreviewManagerEntity m_PreviewManager;
    protected PreviewRenderAttributes m_PlayerRenderAttributes;
    protected GC_LobbyCharacterPreviewHelper m_CharacterWidgetHelper;
    protected IEntity m_PreviewEntity;
	protected GC_Slot m_HoveredSlot;
	protected GC_Slot m_PendingHoveredSlot;
	protected bool m_bWasManipulatingPreview;
	protected InputManager m_InputManager;

    override void OnMenuOpen()
    {
        if (SCR_Global.IsEditMode())
            return;

		GC_LobbyGameMode lobbyGameMode = GC_LobbyGameMode.GetGameMode();
		string phaseName = "NO_GAME_MODE";
		if (lobbyGameMode)
			phaseName = GC_LobbyManager.GetPhaseName(lobbyGameMode.GetState());
		PrintFormat("GC | Phase UI | slotting menu opened: menu=%1 state=%2", this, phaseName);

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
			m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);

        m_wRoot = GetRootWidget();

        InitializeFactionList();
        InitializeGroupWidget();
        SelectLocalPlayerFaction();
        InitializePlayerPreview();

        Widget headerRoot = m_wRoot.FindAnyWidget("MenuBar");
        if (!headerRoot)
        {
            PrintFormat("GC | Header root not found");
            return;
        }

        m_Header = GC_HeaderComponent.Cast(headerRoot.FindHandler(GC_HeaderComponent));
        if (!m_Header)
        {
            PrintFormat("GC | Header component not found");
            return;
        }

		// LOBBY is a local view, not a game-mode phase.
        m_Header.SetActiveState(1);
    }

    override void OnMenuUpdate(float tDelta)
    {
        super.OnMenuUpdate(tDelta);

		if (m_InventoryDetail && m_InventoryDetail.IsOpen())
		{
			m_PendingHoveredSlot = null;
			CancelPendingLoadoutRestore();
			return;
		}

        if (!m_CharacterWidgetHelper || !m_PreviewManager || !m_PlayerRenderAttributes)
            return;

        bool isManipulatingPreview = m_CharacterWidgetHelper.IsManipulatingPreview();
        if (m_CharacterWidgetHelper.Update(tDelta, m_PlayerRenderAttributes))
            RefreshPlayerPreview();

		if (m_bWasManipulatingPreview && !isManipulatingPreview && m_PendingHoveredSlot)
		{
			m_HoveredSlot = m_PendingHoveredSlot;
			m_PendingHoveredSlot = null;
			ShowSlotPreview(m_HoveredSlot);
		}

		m_bWasManipulatingPreview = isManipulatingPreview;
    }

	protected void RefreshAdminNamePresentation(int playerId)
	{
		foreach (Widget groupCard : m_aGroupCards)
		{
			if (!groupCard)
				continue;

			GC_GroupWidgetComponent groupWidget = GC_GroupWidgetComponent.Cast(groupCard.FindHandler(GC_GroupWidgetComponent));
			if (groupWidget)
				groupWidget.RefreshAdminNamePresentation(playerId);
		}
	}

    override void OnMenuClose()
    {
		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		string phaseName = "NO_GAME_MODE";
		if (gameMode)
			phaseName = GC_LobbyManager.GetPhaseName(gameMode.GetState());
		PrintFormat("GC | Phase UI | slotting menu closed: menu=%1 state=%2", this, phaseName);

		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		if (m_InputManager)
			m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnPauseMenuRequested);

		CancelPendingLoadoutRestore();
		CancelPendingGroupScrollReset();

        if (m_PlayerRenderAttributes)
            m_PlayerRenderAttributes.ResetDeltaRotation();

        if (m_CharacterWidgetHelper)
        {
            m_CharacterWidgetHelper.Destroy();
            m_CharacterWidgetHelper = null;
        }

        if (m_FactionList)
            m_FactionList.m_OnFactionSelected.Remove(OnFactionSelected);

		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnLobbyFactionUpdated);
			m_LobbyManager.m_OnFactionPresentationChanged.Remove(OnLobbyFactionUpdated);
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);
		}

        m_PreviewEntity = null;
		m_HoveredSlot = null;
		m_PendingHoveredSlot = null;
		m_bWasManipulatingPreview = false;
        m_PlayerRenderAttributes = null;
        m_PreviewManager = null;
        m_wPlayerRender = null;
		if (m_InventoryPreview)
			m_InventoryPreview.SetInventoryDetail(null);
		m_InventoryPreview = null;
		m_InventoryDetail = null;
        m_wRoot = null;
        m_Header = null;
        m_FactionList = null;
		m_LobbyManager = null;
        m_aGroupCards.Clear();
        m_wGroups = null;
		m_wGroupScroll = null;
		m_InputManager = null;
		m_SelectedFactionKey = string.Empty;

        super.OnMenuClose();
    }

	protected void OnPlayerRoleChanged(int playerId)
	{
		RefreshAdminNamePresentation(playerId);
	}

	protected void OnPauseMenuRequested()
	{
		if (m_InventoryDetail && m_InventoryDetail.IsOpen())
		{
			m_InventoryDetail.Close();
			return;
		}

		// Defer opening until the MenuBack input event has finished. Opening it
		// immediately would let the same Escape press close the new pause menu.
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	protected void OpenPauseMenu()
	{
		ArmaReforgerScripted.OpenPauseMenu(false, true);
	}

    protected void InitializeFactionList()
    {
        if (!m_wRoot)
            return;

        Widget factionContent = m_wRoot.FindAnyWidget("FactionContent");
        if (!factionContent)
        {
            PrintFormat("GC | Faction content not found");
            return;
        }

        Widget factionListRoot = factionContent.GetParent();
        if (!factionListRoot)
        {
            PrintFormat("GC | Faction list root not found");
            return;
        }

        m_FactionList = GC_FactionListComponent.Cast(factionListRoot.FindHandler(GC_FactionListComponent));
        if (!m_FactionList)
        {
            PrintFormat("GC | Faction list component not found");
            return;
        }

        m_FactionList.m_OnFactionSelected.Insert(OnFactionSelected);
        m_SelectedFactionKey = m_FactionList.GetSelectedFactionKey();
    }

    protected void OnFactionSelected(FactionKey factionKey)
    {
		CancelPendingLoadoutRestore();
		CancelPendingGroupScrollReset();
		ResetGroupScrollPosition();
		m_SelectedFactionKey = factionKey;
		m_HoveredSlot = null;
		m_PendingHoveredSlot = null;
		RefreshGroupWidget();
		GetGame().GetCallqueue().CallLater(ResetGroupScrollPosition, 0);
		RefreshLoadoutPreview();
    }

	protected void SelectLocalPlayerFaction()
	{
		if (!m_FactionList || !m_LobbyManager)
			return;

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		Faction playerFaction = m_LobbyManager.GetPlayerFaction(playerController.GetPlayerId());
		if (!playerFaction)
			return;

		m_FactionList.SetSelectedFaction(playerFaction.GetFactionKey());
	}

    protected void InitializeGroupWidget()
    {
        if (!m_wRoot)
            return;

        m_wGroups = m_wRoot.FindAnyWidget("Groups");
        if (!m_wGroups)
        {
            PrintFormat("GC | Groups insertion list not found");
            return;
        }

		m_wGroupScroll = ScrollLayoutWidget.Cast(m_wGroups.GetParent());
		if (!m_wGroupScroll)
			PrintFormat("GC | Group scroll layout not found");

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (!m_LobbyManager)
        {
            PrintFormat("GC | Lobby manager not found while creating groups");
            return;
        }

		m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnLobbyFactionUpdated);
		m_LobbyManager.m_OnFactionPresentationChanged.Insert(OnLobbyFactionUpdated);
		m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);
		RefreshGroupWidget();
	}

	protected void OnLobbyFactionUpdated(Faction faction)
	{
		if (m_SelectedFactionKey.IsEmpty())
			SelectLocalPlayerFaction();

		if (faction && faction.GetFactionKey() != m_SelectedFactionKey)
			return;

		if (!HasCurrentGroupTopology())
			RefreshGroupWidget();

		if (!m_HoveredSlot && !m_PendingHoveredSlot && (!m_CharacterWidgetHelper || !m_CharacterWidgetHelper.IsManipulatingPreview()))
			RefreshLoadoutPreview();
	}

	protected bool HasCurrentGroupTopology()
	{
		if (!m_LobbyManager || m_SelectedFactionKey.IsEmpty())
			return m_aGroupCards.IsEmpty();

		array<SCR_AIGroup> groups = m_LobbyManager.GetGroupsForFaction(m_SelectedFactionKey);
		if (groups.Count() != m_aGroupCards.Count())
			return false;

		for (int i = 0; i < groups.Count(); i++)
		{
			Widget groupCard = m_aGroupCards[i];
			GC_GroupWidgetComponent groupWidget;
			if (groupCard)
				groupWidget = GC_GroupWidgetComponent.Cast(groupCard.FindHandler(GC_GroupWidgetComponent));

			if (!groupWidget || groupWidget.GetGroup() != groups[i])
				return false;
		}

		return true;
	}

	protected void RefreshGroupWidget()
	{
		ClearGroupCards();

		if (!m_wGroups || !m_LobbyManager || m_SelectedFactionKey.IsEmpty())
		{
			return;
		}

		foreach (SCR_AIGroup group : m_LobbyManager.GetGroupsForFaction(m_SelectedFactionKey))
        {
            if (!group)
                continue;

            Widget groupCard = GetGame().GetWorkspace().CreateWidgets(GROUP_LAYOUT, m_wGroups);
            if (!groupCard)
                continue;

            GC_GroupWidgetComponent groupWidget = GC_GroupWidgetComponent.Cast(groupCard.FindHandler(GC_GroupWidgetComponent));
            if (!groupWidget)
            {
                groupCard.RemoveFromHierarchy();
                continue;
            }

			groupWidget.m_OnSlotHovered.Insert(OnSlotHovered);
			groupWidget.m_OnSlotHoverEnded.Insert(OnSlotHoverEnded);
            groupWidget.SetGroup(group);
            m_aGroupCards.Insert(groupCard);
        }

	}

	protected void ResetGroupScrollPosition()
	{
		if (m_wGroupScroll)
			m_wGroupScroll.SetSliderPos(0, 0);
	}

	protected void CancelPendingGroupScrollReset()
	{
		GetGame().GetCallqueue().Remove(ResetGroupScrollPosition);
	}

	protected void ClearGroupCards()
	{
		foreach (Widget groupCard : m_aGroupCards)
		{
			if (groupCard)
			{
				GC_GroupWidgetComponent groupWidget = GC_GroupWidgetComponent.Cast(groupCard.FindHandler(GC_GroupWidgetComponent));
				if (groupWidget)
				{
					groupWidget.m_OnSlotHovered.Remove(OnSlotHovered);
					groupWidget.m_OnSlotHoverEnded.Remove(OnSlotHoverEnded);
				}

				groupCard.RemoveFromHierarchy();
			}
		}

		m_aGroupCards.Clear();
    }

    protected void InitializePlayerPreview()
    {
        if (!m_wRoot)
            return;

		Widget inventoryDetailRoot = m_wRoot.FindAnyWidget("InventoryDetail");
		if (inventoryDetailRoot)
		{
			m_InventoryDetail = GC_LobbyInventoryDetailComponent.Cast(inventoryDetailRoot.FindHandler(GC_LobbyInventoryDetailComponent));
			if (!m_InventoryDetail)
				PrintFormat("GC | Lobby inventory detail component not found");
		}

        m_wLoadoutInventory = m_wRoot.FindAnyWidget("LoadoutInventory");
        if (m_wLoadoutInventory)
		{
            m_wLoadoutInventory.SetVisible(false);
			m_InventoryPreview = GC_LobbyInventoryPreviewComponent.Cast(m_wLoadoutInventory.FindHandler(GC_LobbyInventoryPreviewComponent));
			if (!m_InventoryPreview)
				PrintFormat("GC | Lobby inventory preview component not found");
			else
				m_InventoryPreview.SetInventoryDetail(m_InventoryDetail);
		}

        m_wPlayerRender = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("playerRender"));
        if (!m_wPlayerRender)
        {
            PrintFormat("GC | Player preview widget not found");
            return;
        }

        RenderTargetWidget renderTarget = m_wPlayerRender;
        if (renderTarget)
        {
            renderTarget.SetClearColor(false, 0);
            renderTarget.SetBlendMode(RenderTargetWidgetBlendMode.ALPHA_BLEND);
        }

        ChimeraWorld world = GetGame().GetWorld();
        if (!world)
            return;

        m_PreviewManager = world.GetItemPreviewManager();
        if (!m_PreviewManager)
        {
            Resource resource = Resource.Load(PREVIEW_MANAGER_PREFAB);
            if (resource.IsValid())
                GetGame().SpawnEntityPrefabLocal(resource, world);

            m_PreviewManager = world.GetItemPreviewManager();
        }

        if (!m_PreviewManager)
        {
            PrintFormat("GC | Item preview manager not found");
            return;
        }

		RefreshLoadoutPreview();
    }

	protected void OnSlotHovered(GC_Slot slot)
	{
		if (!slot)
			return;

		if (m_InventoryDetail && m_InventoryDetail.IsOpen())
			return;

		CancelPendingLoadoutRestore();
		if (m_CharacterWidgetHelper && m_CharacterWidgetHelper.IsManipulatingPreview())
		{
			m_PendingHoveredSlot = slot;
			return;
		}

		m_PendingHoveredSlot = null;
		m_HoveredSlot = slot;
		ShowSlotPreview(slot);
	}

	protected void OnSlotHoverEnded(GC_Slot slot)
	{
		if (m_InventoryDetail && m_InventoryDetail.IsOpen())
			return;

		if (slot && slot == m_PendingHoveredSlot)
			m_PendingHoveredSlot = null;

		if (!slot || slot != m_HoveredSlot)
			return;

		m_HoveredSlot = null;
		CancelPendingLoadoutRestore();
		GetGame().GetCallqueue().CallLater(RestoreSelectedLoadoutAfterHover, LOADOUT_HOVER_EXIT_DELAY_MS);
	}

	protected void RestoreSelectedLoadoutAfterHover()
	{
		if (m_InventoryDetail && m_InventoryDetail.IsOpen())
			return;

		if (!m_HoveredSlot)
			RefreshLoadoutPreview();
	}

	protected void CancelPendingLoadoutRestore()
	{
		GetGame().GetCallqueue().Remove(RestoreSelectedLoadoutAfterHover);
	}

	protected void RefreshLoadoutPreview()
	{
		if (!m_PreviewManager || !m_wPlayerRender)
			return;

		GC_Slot selectedSlot = GetLocalSelectedSlot();
		if (selectedSlot)
		{
			ShowSlotPreview(selectedSlot);
			return;
		}

		HideLoadoutPreview();
	}

	protected GC_Slot GetLocalSelectedSlot()
	{
		if (!m_LobbyManager)
			return null;

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return null;

		return m_LobbyManager.GetSlotByPlayerId(playerController.GetPlayerId());
	}

	protected void ShowSlotPreview(GC_Slot slot)
	{
		if (!slot)
			return;

		ShowPrefabPreview(slot.GetResolvedPrefab());
	}

	protected void ShowPrefabPreview(ResourceName prefab)
	{
		if (!m_PreviewManager || !m_wPlayerRender)
			return;

		if (prefab == string.Empty)
		{
			HideLoadoutPreview();
			return;
		}

		ResetPreviewState();
		m_wPlayerRender.SetVisible(true);
		if (m_wLoadoutInventory)
			m_wLoadoutInventory.SetVisible(true);

		m_PreviewManager.SetPreviewItemFromPrefab(m_wPlayerRender, prefab, null, true);
		m_PreviewEntity = m_PreviewManager.ResolvePreviewEntityForPrefab(prefab);
		LoadPreviewAttributes(m_PreviewEntity);
		if (m_InventoryPreview)
			m_InventoryPreview.SetCharacter(m_PreviewEntity);

		if (m_PreviewEntity)
			m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes, true);

		EnsurePreviewHelper();
	}

	protected void HideLoadoutPreview()
	{
		ResetPreviewState();
		if (m_InventoryPreview)
			m_InventoryPreview.ResetPreview();
		else if (m_InventoryDetail)
		{
			m_InventoryDetail.Close();
			m_InventoryDetail.SetCharacter(null, m_PreviewManager);
		}
		if (m_wPlayerRender)
			m_wPlayerRender.SetVisible(false);

		if (m_wLoadoutInventory)
			m_wLoadoutInventory.SetVisible(false);
	}

	protected void ResetPreviewState()
	{
		if (m_PlayerRenderAttributes)
			m_PlayerRenderAttributes.ResetDeltaRotation();

		m_PreviewEntity = null;
		m_PlayerRenderAttributes = null;
	}

	protected void EnsurePreviewHelper()
	{
		if (m_PlayerRenderAttributes && !m_CharacterWidgetHelper)
		{
			Widget interactionArea = m_wRoot.FindAnyWidget("LoadoutPreview");
			if (!interactionArea)
			{
				PrintFormat("GC | Loadout preview interaction area not found");
				return;
			}

			m_CharacterWidgetHelper = GC_LobbyCharacterPreviewHelper(m_wPlayerRender, interactionArea);
		}
	}

    protected void LoadPreviewAttributes(IEntity entity)
    {
        if (!entity)
            return;

        SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
        if (!storage)
            return;

        auto collection = storage.GetAttributes();
        if (collection)
            m_PlayerRenderAttributes = PreviewRenderAttributes.Cast(collection.FindAttribute(SCR_CharacterInventoryPreviewAttributes));
    }

    protected void RefreshPlayerPreview()
    {
        if (!m_PreviewManager || !m_wPlayerRender || !m_PreviewEntity)
            return;

        m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes);
    }

}
