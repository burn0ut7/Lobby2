modded enum ChimeraMenuPreset
{
    GC_Lobby_SlottingMenu
};

class GC_Lobby_SlottingMenu : ChimeraMenuBase
{
    protected static ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";
    protected static ResourceName FALLBACK_CHARACTER_PREFAB = "{9C2A994C94376FFE}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Ammo_random.et";

    protected Widget m_wRoot;
    protected GC_HeaderComponent m_Header;
    protected ItemPreviewWidget m_wPlayerRender;
    protected ItemPreviewManagerEntity m_PreviewManager;
    protected PreviewRenderAttributes m_PlayerRenderAttributes;
    protected GC_LobbyCharacterPreviewHelper m_CharacterWidgetHelper;
    protected IEntity m_PreviewEntity;

    override void OnMenuOpen()
    {
        PrintFormat("GC | OnMenuOpen");

        m_wRoot = GetRootWidget();

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
        m_Header.SetCanChangeState(SCR_Global.IsAdmin());
        PrintFormat("GC | Slotting menu subscribed to header state changes");
    }

    override void OnMenuUpdate(float tDelta)
    {
        super.OnMenuUpdate(tDelta);

        if (!m_CharacterWidgetHelper || !m_PreviewManager || !m_PlayerRenderAttributes)
            return;

        if (m_CharacterWidgetHelper.Update(tDelta, m_PlayerRenderAttributes))
            RefreshPlayerPreview();
    }

    override void OnMenuClose()
    {
        if (m_PlayerRenderAttributes)
            m_PlayerRenderAttributes.ResetDeltaRotation();

        if (m_CharacterWidgetHelper)
        {
            m_CharacterWidgetHelper.Destroy();
            m_CharacterWidgetHelper = null;
        }

        if (m_Header)
            m_Header.m_OnStateChanged.Remove(OnHeaderStateChanged);

        m_PreviewEntity = null;
        m_PlayerRenderAttributes = null;
        m_PreviewManager = null;
        m_wPlayerRender = null;
        m_wRoot = null;
        m_Header = null;

        super.OnMenuClose();
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

        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController)
        {
            ChimeraCharacter player = ChimeraCharacter.Cast(playerController.GetControlledEntity());
            if (player)
            {
                m_PreviewEntity = player;
                LoadPreviewAttributes(m_PreviewEntity);
            }
        }

        if (m_PreviewEntity && m_PlayerRenderAttributes)
        {
            m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes, true);
        }
        else
        {
            m_PreviewManager.SetPreviewItemFromPrefab(m_wPlayerRender, FALLBACK_CHARACTER_PREFAB, null, true);
            m_PreviewEntity = m_PreviewManager.ResolvePreviewEntityForPrefab(FALLBACK_CHARACTER_PREFAB);
            LoadPreviewAttributes(m_PreviewEntity);

            if (m_PreviewEntity && m_PlayerRenderAttributes)
                m_PreviewManager.SetPreviewItem(m_wPlayerRender, m_PreviewEntity, m_PlayerRenderAttributes, true);
        }

        if (m_PlayerRenderAttributes)
        {
            m_CharacterWidgetHelper = GC_LobbyCharacterPreviewHelper(m_wPlayerRender, GetGame().GetWorkspace());
        }
        else
        {
            PrintFormat("GC | Preview attributes not found; rotation disabled");
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
