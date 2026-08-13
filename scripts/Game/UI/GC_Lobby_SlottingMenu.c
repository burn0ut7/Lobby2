modded enum ChimeraMenuPreset
{
    GC_Lobby_SlottingMenu
};

class GC_Lobby_SlottingMenu : ChimeraMenuBase
{
	protected static const int LOADOUT_HOVER_EXIT_DELAY_MS = 100;
    protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";
    protected static const ResourceName FALLBACK_CHARACTER_PREFAB = "{9C2A994C94376FFE}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Ammo_random.et";
    protected static const ResourceName GROUP_LAYOUT = "{6A25000000000001}UI/layouts/Groups/Lobby_Group.layout";

    protected Widget m_wRoot;
    protected GC_HeaderComponent m_Header;
    protected GC_FactionListComponent m_FactionList;
    protected GC_LobbyManager m_LobbyManager;
    protected Widget m_wGroups;
    protected ref array<Widget> m_aGroupCards = {};
    protected FactionKey m_SelectedFactionKey;
    protected ItemPreviewWidget m_wPlayerRender;
    protected ItemPreviewManagerEntity m_PreviewManager;
    protected PreviewRenderAttributes m_PlayerRenderAttributes;
    protected GC_LobbyCharacterPreviewHelper m_CharacterWidgetHelper;
    protected IEntity m_PreviewEntity;
	protected GC_Slot m_HoveredSlot;
	protected GC_Slot m_PendingHoveredSlot;
	protected bool m_bWasManipulatingPreview;

    override void OnMenuOpen()
    {
        if (SCR_Global.IsEditMode())
            return;

        PrintFormat("GC | OnMenuOpen");

        m_wRoot = GetRootWidget();

        InitializeFactionList();
        InitializeGroupWidget();
        InitializePlayerPreview();

        Widget headerRoot = m_wRoot.FindAnyWidget("Frame");
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

        m_Header.m_OnStateChanged.Insert(OnHeaderStateChanged);
        m_Header.SetActiveState(0);
        PrintFormat("GC | Slotting menu subscribed to header state changes");
    }

    override void OnMenuUpdate(float tDelta)
    {
        super.OnMenuUpdate(tDelta);

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

    override void OnMenuClose()
    {
		CancelPendingLoadoutRestore();

        if (m_PlayerRenderAttributes)
            m_PlayerRenderAttributes.ResetDeltaRotation();

        if (m_CharacterWidgetHelper)
        {
            m_CharacterWidgetHelper.Destroy();
            m_CharacterWidgetHelper = null;
        }

        if (m_Header)
            m_Header.m_OnStateChanged.Remove(OnHeaderStateChanged);

        if (m_FactionList)
            m_FactionList.m_OnFactionSelected.Remove(OnFactionSelected);

		if (m_LobbyManager)
			m_LobbyManager.m_OnFactionUpdated.Remove(OnLobbyFactionUpdated);

        m_PreviewEntity = null;
		m_HoveredSlot = null;
		m_PendingHoveredSlot = null;
		m_bWasManipulatingPreview = false;
        m_PlayerRenderAttributes = null;
        m_PreviewManager = null;
        m_wPlayerRender = null;
        m_wRoot = null;
        m_Header = null;
        m_FactionList = null;
		m_LobbyManager = null;
        m_aGroupCards.Clear();
        m_wGroups = null;
        m_SelectedFactionKey = string.Empty;

        super.OnMenuClose();
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
        PrintFormat("GC | Slotting menu subscribed to faction selection");
    }

    protected void OnFactionSelected(FactionKey factionKey)
    {
		CancelPendingLoadoutRestore();
		m_SelectedFactionKey = factionKey;
		m_HoveredSlot = null;
		m_PendingHoveredSlot = null;
		RefreshGroupWidget();
		RefreshLoadoutPreview();
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

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (!m_LobbyManager)
        {
            PrintFormat("GC | Lobby manager not found while creating groups");
            return;
        }

		m_LobbyManager.m_OnFactionUpdated.Insert(OnLobbyFactionUpdated);
		RefreshGroupWidget();
	}

	protected void OnLobbyFactionUpdated(Faction faction)
	{
		if (!faction || faction.GetFactionKey() != m_SelectedFactionKey)
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
			PrintFormat("GC | No faction selected; lobby group list is empty");
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

		PrintFormat("GC | Created %1 lobby group cards for faction '%2'", m_aGroupCards.Count(), m_SelectedFactionKey);
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

		ShowDefaultPlayerPreview();
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

		ShowPrefabPreview(slot.GetPrefab());
	}

	protected void ShowDefaultPlayerPreview()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
		{
			ChimeraCharacter player = ChimeraCharacter.Cast(playerController.GetControlledEntity());
			if (player)
			{
				ShowEntityPreview(player);
				return;
			}
		}

		ShowPrefabPreview(FALLBACK_CHARACTER_PREFAB);
	}

	protected void ShowPrefabPreview(ResourceName prefab)
	{
		if (!m_PreviewManager || !m_wPlayerRender || prefab == string.Empty)
			return;

		ResetPreviewState();
		m_PreviewManager.SetPreviewItemFromPrefab(m_wPlayerRender, prefab, null, true);
		m_PreviewEntity = m_PreviewManager.ResolvePreviewEntityForPrefab(prefab);
		LoadPreviewAttributes(m_PreviewEntity);

		if (m_PreviewEntity)
			m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes, true);

		EnsurePreviewHelper();
	}

	protected void ShowEntityPreview(IEntity entity)
	{
		if (!m_PreviewManager || !m_wPlayerRender || !entity)
			return;

		ResetPreviewState();
		m_PreviewEntity = entity;
		LoadPreviewAttributes(m_PreviewEntity);
		m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes, true);
		EnsurePreviewHelper();
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
			m_CharacterWidgetHelper = GC_LobbyCharacterPreviewHelper(m_wPlayerRender, GetGame().GetWorkspace());
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

    protected void OnHeaderStateChanged(GC_HeaderComponent header)
    {
        int state = header.GetActiveState();
        PrintFormat("GC | Slotting header state callback: %1", state);

        switch (state)
        {
            case 0:
                MissionPreview();
                break;
            case 1:
                LobbyState();
                break;
            case 2:
                BriefingState();
                break;
            case 3:
                GameState();
                break;
            case 4:
                DebriefState();
                break;
            default:
                ClearHeaderState();
                break;
        }
    }

    protected void MissionPreview()
    {
        PrintFormat("GC | Mission Preview state selected");
        // Put the actual mission-preview transition here.
    }

    protected void LobbyState()
    {
        PrintFormat("GC | Lobby state selected");
    }

    protected void BriefingState()
    {
        PrintFormat("GC | Briefing state selected");
    }

    protected void GameState()
    {
        PrintFormat("GC | Game state selected");
    }

    protected void DebriefState()
    {
        PrintFormat("GC | Debrief state selected");
    }

    protected void ClearHeaderState()
    {
        PrintFormat("GC | Header state cleared");
    }
}
