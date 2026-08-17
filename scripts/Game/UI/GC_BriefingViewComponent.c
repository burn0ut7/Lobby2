class GC_BriefingViewComponent : SCR_ScriptedWidgetComponent
{
    protected static const string ACTION_DEBUG_PRINT = "debug_print";
    protected static const string ACTION_TAG_PREFIX = "<action id=\"";
    protected static const string ACTION_TAG_CLOSE = "</action>";
    protected static const string MARKER_TAG_PREFIX = "<marker name=\"";
    protected static const string MARKER_TAG_CLOSE = "</marker>";
    protected static const string LINK_TAG_PREFIX = "<link url=\"";
    protected static const string LINK_TAG_CLOSE = "</link>";
    protected static const string UNDERLINE_TAG_OPEN = "<u>";
    protected static const string UNDERLINE_TAG_CLOSE = "</u>";
    protected static const string STRIKE_TAG_OPEN = "<s>";
    protected static const string STRIKE_TAG_CLOSE = "</s>";
    protected static const string STRIKE_LONG_TAG_OPEN = "<strike>";
    protected static const string STRIKE_LONG_TAG_CLOSE = "</strike>";
    protected static const string SIZE_TAG_PREFIX = "<size scale=\"";
    protected static const string SIZE_TAG_CLOSE = "</size>";
    protected static const string BLOCK_IMAGE_TAG_PREFIX = "<img ";
    protected static const string BLOCK_IMAGE_TAG_CLOSE = "/>";
    protected static const string SCENARIO_TAG = "<scenario />";
    protected static const string TAG_ATTRIBUTE_CLOSE = "\">";
    protected static const string INVALID_INTERACTIVE_TAG_TEXT = "Invalid Lobby2 briefing interaction tag.";
    protected static const string INVALID_BLOCK_IMAGE_TAG_TEXT = "Invalid Lobby2 briefing image tag.";
    protected static const int MAX_TAG_VALUE_LENGTH = 64;
    protected static const int MAX_LINK_URL_LENGTH = 512;
    protected static const int MAX_BLOCK_IMAGES = 16;
    protected static const int MAX_IMAGE_RESOURCE_LENGTH = 256;
    protected static const int MAX_IMAGE_NAME_LENGTH = 128;
    protected static const float MIN_BLOCK_IMAGE_DIMENSION = 16;
    protected static const float MAX_BLOCK_IMAGE_DIMENSION = 2048;
    protected static const float DEFAULT_INLINE_IMAGE_DIMENSION = 24;
    protected static const float MAX_INLINE_IMAGE_DIMENSION = 96;
    protected static const float BLOCK_IMAGE_VERTICAL_PADDING = 16;
    protected static const ResourceName INLINE_TEXT_LAYOUT = "{6A4D000000000001}UI/layouts/Briefing/Lobby_Briefing_Inline_Text.layout";
    protected static const ResourceName INLINE_ACTION_LAYOUT = "{6A4D000000000002}UI/layouts/Briefing/Lobby_Briefing_Inline_Action.layout";
    protected static const ResourceName INLINE_DECORATION_LAYOUT = "{6A4D000000000003}UI/layouts/Briefing/Lobby_Briefing_Inline_Decoration.layout";
    protected static const ResourceName BLOCK_TEXT_LAYOUT = "{6A4D000000000004}UI/layouts/Briefing/Lobby_Briefing_Block_Text.layout";
    protected static const ResourceName BLOCK_IMAGE_LAYOUT = "{6A4D000000000005}UI/layouts/Briefing/Lobby_Briefing_Block_Image.layout";
    protected static const ResourceName INLINE_IMAGE_LAYOUT = "{6A4D000000000007}UI/layouts/Briefing/Lobby_Briefing_Inline_Image.layout";
    protected static const int INLINE_TAG_ACTION = 0;
    protected static const int INLINE_TAG_MARKER = 1;
    protected static const int INLINE_TAG_UNDERLINE = 2;
    protected static const int INLINE_TAG_STRIKE = 3;
    protected static const int INLINE_TAG_SIZE = 4;
    protected static const int INLINE_TAG_LINK = 5;
    protected static const int INLINE_TAG_IMAGE = 6;
    protected static const int INLINE_BASE_FONT_SIZE = 24;
    protected static const float MIN_INLINE_FONT_SCALE = 0.75;
    protected static const float MAX_INLINE_FONT_SCALE = 2.0;
    protected static const float COMPACT_WIDTH = 287;
    protected static const int SECTION_LAYOUT_SETTLE_DELAY = 50;

    protected GC_BriefingTitlesComponent m_Titles;
    protected GC_LobbyManager m_LobbyManager;
    protected SizeLayoutWidget m_wViewSize;
    protected Widget m_wIndexPanel;
    protected Widget m_wDocumentPanel;
    protected TextWidget m_wSectionTitle;
    protected RichTextWidget m_wPlainSectionBody;
    protected RichTextWidget m_wRichSectionBody;
    protected ScrollLayoutWidget m_wSectionScroll;
    protected SizeLayoutWidget m_wSectionContentSize;
    protected GC_BriefingScrollComponent m_SectionScrollComponent;
    protected Widget m_wActionMarkerFlow;
    protected RichTextWidget m_wActionMarkerBefore;
    protected Widget m_wActionInlineFlow;
    protected RichTextWidget m_wActionMarkerAfter;
    protected Widget m_wBlockImageFlow;
    protected GC_BriefingScenarioComponent m_ScenarioComponent;
	protected ref GC_BriefingRenderer m_Renderer = new GC_BriefingRenderer();
	protected ref GC_BriefingInteractiveRenderer m_InteractiveRenderer = new GC_BriefingInteractiveRenderer();
    protected float m_fExpandedWidth;
    protected float m_fMaxViewHeight;
    protected ref GC_BriefingMapFocusController m_MapFocusController = new GC_BriefingMapFocusController();

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wViewSize = SizeLayoutWidget.Cast(w);
        if (m_wViewSize)
        {
            m_fExpandedWidth = m_wViewSize.GetWidthOverride();
            m_fMaxViewHeight = m_wViewSize.GetHeightOverride();
        }

        m_wIndexPanel = w.FindAnyWidget("BriefingIndexPanel");
        m_wDocumentPanel = w.FindAnyWidget("BriefingDocumentPanel");
        m_wSectionTitle = TextWidget.Cast(w.FindAnyWidget("BriefingSectionTitle"));
        m_wPlainSectionBody = RichTextWidget.Cast(w.FindAnyWidget("BriefingSectionPlainBody"));
        m_wRichSectionBody = RichTextWidget.Cast(w.FindAnyWidget("BriefingSectionRichBody"));
        m_wSectionScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("BriefingSectionScroll"));
        m_wSectionContentSize = SizeLayoutWidget.Cast(w.FindAnyWidget("BriefingSectionContentSize"));
        if (m_wSectionScroll)
            m_SectionScrollComponent = GC_BriefingScrollComponent.Cast(m_wSectionScroll.FindHandler(GC_BriefingScrollComponent));
        m_wActionMarkerFlow = w.FindAnyWidget("BriefingActionMarkerFlow");
        m_wActionMarkerBefore = RichTextWidget.Cast(w.FindAnyWidget("BriefingActionMarkerBefore"));
        m_wActionInlineFlow = w.FindAnyWidget("BriefingActionInlineFlow");
        m_wActionMarkerAfter = RichTextWidget.Cast(w.FindAnyWidget("BriefingActionMarkerAfter"));
		m_InteractiveRenderer.Initialize(m_wActionMarkerFlow, m_wActionMarkerBefore, m_wActionInlineFlow, m_wActionMarkerAfter);
        m_wBlockImageFlow = w.FindAnyWidget("BriefingBlockImageFlow");
        if (m_wBlockImageFlow)
            m_ScenarioComponent = GC_BriefingScenarioComponent.Cast(m_wBlockImageFlow.FindHandler(GC_BriefingScenarioComponent));
		m_Renderer.Initialize(m_wBlockImageFlow, m_wSectionScroll, m_ScenarioComponent);

        Widget titlesRoot = w.FindAnyWidget("BriefingTitles");
        if (titlesRoot)
            m_Titles = GC_BriefingTitlesComponent.Cast(titlesRoot.FindHandler(GC_BriefingTitlesComponent));

        if (m_Titles)
            m_Titles.m_OnTitleSelected.Insert(OnTitleSelected);

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnBriefingsChanged.Insert(OnBriefingsChanged);
            m_LobbyManager.m_OnPlayerRegistryUpdated.Insert(OnPlayerRegistryChanged);
        }

        RefreshBriefings();
    }

    override void HandlerDeattached(Widget w)
    {
        GetGame().GetCallqueue().Remove(PrepareSectionLayout);
        GetGame().GetCallqueue().Remove(CommitSectionLayout);
        CancelMarkerFocus();
		m_InteractiveRenderer.Detach();
		m_Renderer.Detach();

        if (m_Titles)
            m_Titles.m_OnTitleSelected.Remove(OnTitleSelected);

        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnBriefingsChanged.Remove(OnBriefingsChanged);
            m_LobbyManager.m_OnPlayerRegistryUpdated.Remove(OnPlayerRegistryChanged);
        }

        m_Titles = null;
        m_LobbyManager = null;
        m_wViewSize = null;
        m_wIndexPanel = null;
        m_wDocumentPanel = null;
        m_wSectionTitle = null;
        m_wPlainSectionBody = null;
        m_wRichSectionBody = null;
        m_wSectionScroll = null;
        m_wSectionContentSize = null;
        m_SectionScrollComponent = null;
        m_wActionMarkerFlow = null;
        m_wActionMarkerBefore = null;
        m_wActionInlineFlow = null;
        m_wActionMarkerAfter = null;
        m_wBlockImageFlow = null;
        m_ScenarioComponent = null;

        super.HandlerDeattached(w);
    }

    protected void OnTitleSelected(GC_BriefingTitleRowComponent row)
    {
        if (!row || !row.IsSelected())
        {
            ShowBriefing(null);
            return;
        }

        GC_Briefing briefing;
        if (m_LobbyManager)
            briefing = m_LobbyManager.GetBriefing(row.GetBriefingOrder());

        ShowBriefing(briefing);
    }

    protected void OnBriefingsChanged(int revision)
    {
        RefreshBriefings();
    }

    protected void OnPlayerRegistryChanged(int playerId)
    {
        RefreshBriefings();
    }

    protected void RefreshBriefings()
    {
        if (!m_Titles || !m_LobbyManager)
        {
            ShowBriefing(null);
            return;
        }

        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
        {
            ShowBriefing(null);
            return;
        }

        array<ref GC_Briefing> briefings = m_LobbyManager.GetBriefingsForPlayer(playerController.GetPlayerId());
        GC_BriefingTitleRowComponent selectedRow = m_Titles.ConfigureBriefings(briefings);
        if (!selectedRow)
        {
            ShowBriefing(null);
            return;
        }

        ShowBriefing(m_LobbyManager.GetBriefing(selectedRow.GetBriefingOrder()));
    }

    protected void ShowBriefing(GC_Briefing briefing)
    {
        if (!briefing)
        {
            SetViewWidth(COMPACT_WIDTH);

            if (m_wDocumentPanel)
                m_wDocumentPanel.SetVisible(false);

            if (m_wSectionTitle)
                m_wSectionTitle.SetText(string.Empty);

            if (m_wPlainSectionBody)
            {
                m_wPlainSectionBody.SetText(string.Empty);
                m_wPlainSectionBody.SetVisible(true);
            }

            if (m_wRichSectionBody)
            {
                m_wRichSectionBody.SetText(string.Empty);
                m_wRichSectionBody.SetVisible(false);
            }

            ClearActionMarker();
            ClearBlockImageFlow();

            if (m_wSectionContentSize)
                m_wSectionContentSize.SetHeightOverride(1);
            if (m_wSectionScroll)
                m_wSectionScroll.SetSliderPos(0, 0);

            ScheduleSectionLayoutUpdate();

            return;
        }

        SetViewWidth(m_fExpandedWidth);

        if (m_wDocumentPanel)
        {
            m_wDocumentPanel.SetOpacity(0);
            m_wDocumentPanel.SetVisible(true);
        }

        if (m_wSectionTitle)
            m_wSectionTitle.SetText(briefing.GetOrder().ToString(2) + "  " + briefing.GetTitle());

        string content = briefing.GetContent();
        bool hasScenario = content.Contains(SCENARIO_TAG);
        bool isRichText = HasSupportedRichTextTag(content);
        bool hasInlineImage = HasInlineImageTag(content);
        bool hasBlockImage = HasBlockImageTag(content);
        bool hasCustomTag = isRichText && (content.Contains(ACTION_TAG_PREFIX)
            || content.Contains("<marker")
            || content.Contains("<link")
            || content.Contains(UNDERLINE_TAG_OPEN)
            || content.Contains(STRIKE_TAG_OPEN)
            || content.Contains(STRIKE_LONG_TAG_OPEN)
            || content.Contains("<size")
            || hasInlineImage);

        ClearActionMarker();
        ClearBlockImageFlow();
        bool hasScenarioFlow = hasScenario && ConfigureScenarioFlow();
        bool hasInteractiveTag = hasCustomTag && !hasBlockImage && ConfigureInteractiveTag(content);
        bool hasBlockImageFlow = hasBlockImage && !hasCustomTag && !hasScenario && ConfigureBlockImageFlow(content);
        if (m_wPlainSectionBody)
        {
            if (isRichText)
                m_wPlainSectionBody.SetText(string.Empty);
            else
                m_wPlainSectionBody.SetText(content);
            m_wPlainSectionBody.SetVisible(!isRichText);
        }

        if (m_wRichSectionBody)
        {
            m_wRichSectionBody.SetVisible(false);

            if (isRichText && !hasCustomTag && !hasBlockImage && !hasScenario)
            {
                m_wRichSectionBody.SetText(content);
                m_wRichSectionBody.SetVisible(true);
            }
            else if (hasScenario && !hasScenarioFlow)
            {
                m_wRichSectionBody.SetText("Scenario information is unavailable.");
                m_wRichSectionBody.SetVisible(true);
                Print("GC | Failed to configure dynamic scenario briefing", LogLevel.WARNING);
            }
            else if (hasBlockImage && !hasBlockImageFlow)
            {
                // Block images are Lobby2 syntax and are never forwarded to
                // the native parser when malformed or mixed with custom spans.
                m_wRichSectionBody.SetText(INVALID_BLOCK_IMAGE_TAG_TEXT);
                m_wRichSectionBody.SetVisible(true);
                Print("GC | Invalid Lobby2 briefing image tag");
            }
            else if (hasCustomTag && !hasInteractiveTag)
            {
                // Custom tags are Lobby2 syntax, not native RichText syntax.
                // Never pass a malformed custom tag through to the engine parser.
                m_wRichSectionBody.SetText(INVALID_INTERACTIVE_TAG_TEXT);
                m_wRichSectionBody.SetVisible(true);
                Print("GC | Invalid Lobby2 briefing interaction tag");
            }
            else
            {
                m_wRichSectionBody.SetText(string.Empty);
            }
        }

        if (m_wSectionScroll)
            m_wSectionScroll.SetSliderPos(0, 0);

        ScheduleSectionLayoutUpdate();
    }

    protected void ScheduleSectionLayoutUpdate()
    {
        GetGame().GetCallqueue().Remove(PrepareSectionLayout);
        GetGame().GetCallqueue().Remove(CommitSectionLayout);

        if (m_wDocumentPanel && m_wDocumentPanel.IsVisible())
            GetGame().GetCallqueue().CallLater(PrepareSectionLayout, 0);
        else
            GetGame().GetCallqueue().CallLater(CommitSectionLayout, 0);
    }

    //! RichText reports its final wrapped height only after the content-size
    //! override has participated in one layout pass. Keep that pass invisible,
    //! then perform one visible outer-panel resize in CommitSectionLayout.
    protected void PrepareSectionLayout()
    {
        if (!m_wSectionContentSize)
            return;

        ResizeBlockImages();
        m_wSectionContentSize.SetHeightOverride(MeasureSectionContentHeight());
        GetGame().GetCallqueue().CallLater(CommitSectionLayout, SECTION_LAYOUT_SETTLE_DELAY);
    }

    protected float MeasureSectionContentHeight()
    {
        float contentHeight = 1;
        if (m_wBlockImageFlow && m_wBlockImageFlow.IsVisible())
        {
			contentHeight = m_Renderer.GetContentHeight();
        }
        else if (m_wActionMarkerFlow && m_wActionMarkerFlow.IsVisible())
        {
            contentHeight = m_wActionMarkerBefore.GetContentHeight() + m_wActionMarkerAfter.GetContentHeight();

            float inlineWidth;
            float inlineHeight;
            m_wActionInlineFlow.GetScreenSize(inlineWidth, inlineHeight);
            WorkspaceWidget workspace = GetGame().GetWorkspace();
            if (workspace)
                inlineHeight = workspace.DPIUnscale(inlineHeight);

            contentHeight += inlineHeight + 4;
        }
        else if (m_wRichSectionBody && m_wRichSectionBody.IsVisible())
        {
            contentHeight = m_wRichSectionBody.GetContentHeight();
        }
        else if (m_wPlainSectionBody && m_wPlainSectionBody.IsVisible())
        {
            contentHeight = m_wPlainSectionBody.GetContentHeight();
        }

        if (contentHeight < 1)
            contentHeight = 1;

        return contentHeight;
    }

    protected void CommitSectionLayout()
    {
        if (!m_wViewSize || !m_wIndexPanel || !m_wDocumentPanel || !m_wSectionScroll || !m_wSectionContentSize)
            return;

        float contentHeight = MeasureSectionContentHeight();
        m_wSectionContentSize.SetHeightOverride(contentHeight);

        float viewWidth;
        float viewHeight;
        float indexWidth;
        float indexHeight;
        float documentWidth;
        float documentHeight;
        float scrollWidth;
        float scrollHeight;
        m_wViewSize.GetScreenSize(viewWidth, viewHeight);
        m_wIndexPanel.GetScreenSize(indexWidth, indexHeight);
        m_wDocumentPanel.GetScreenSize(documentWidth, documentHeight);
        m_wSectionScroll.GetScreenSize(scrollWidth, scrollHeight);

        WorkspaceWidget workspace = GetGame().GetWorkspace();
        float laidOutContentHeight = contentHeight;
        if (workspace)
            laidOutContentHeight = workspace.DPIScale(laidOutContentHeight);

        float outerPaddingHeight = viewHeight - documentHeight;
        if (outerPaddingHeight < 0)
            outerPaddingHeight = 0;

        float documentChromeHeight = documentHeight - scrollHeight;
        if (documentChromeHeight < 0)
            documentChromeHeight = 0;

        float desiredHeight = outerPaddingHeight + documentChromeHeight + laidOutContentHeight;
        float indexDesiredHeight = outerPaddingHeight + indexHeight;
        if (indexDesiredHeight > desiredHeight)
            desiredHeight = indexDesiredHeight;

        float maxViewHeight = m_fMaxViewHeight;
        if (workspace)
            maxViewHeight = workspace.DPIScale(maxViewHeight);

        if (desiredHeight > maxViewHeight)
            desiredHeight = maxViewHeight;
        if (desiredHeight < 1)
            desiredHeight = 1;
        if (workspace)
            desiredHeight = workspace.DPIUnscale(desiredHeight);

        SetViewHeight(desiredHeight);
        if (m_wDocumentPanel.IsVisible())
            m_wDocumentPanel.SetOpacity(1);
        if (m_SectionScrollComponent)
            m_SectionScrollComponent.RefreshOverflow();
    }

    //! Uses a conservative allowlist so ordinary text and unknown angle-bracket
    //! content fall back to the plain widget instead of reaching RichText parsing.
    protected bool HasSupportedRichTextTag(string content)
    {
        return GC_BriefingParser.HasSupportedRichTextTag(content);
    }

    protected bool ConfigureScenarioFlow()
    {
        return m_Renderer.ConfigureScenario();
    }

    protected bool ConfigureBlockImageFlow(string content)
    {
        return m_Renderer.ConfigureBlocks(content);
    }

    protected bool HasInlineImageTag(string content)
    {
        return GC_BriefingParser.HasInlineImageTag(content);
    }

    protected bool HasBlockImageTag(string content)
    {
        return GC_BriefingParser.HasBlockImageTag(content);
    }

    protected void ResizeBlockImages()
    {
        m_Renderer.Resize();
    }

    protected void ClearBlockImageFlow()
    {
        m_Renderer.Clear();
    }

    protected bool ConfigureInteractiveTag(string content)
    {
        return m_InteractiveRenderer.Configure(content, m_LobbyManager, m_wViewSize, m_MapFocusController);
    }

    protected void ClearActionMarker()
    {
        m_InteractiveRenderer.Clear();
    }

    protected void CancelMarkerFocus()
    {
        m_MapFocusController.Cancel();
    }

    protected void SetViewWidth(float width)
    {
        if (!m_wViewSize)
            return;

        m_wViewSize.SetWidthOverride(width);

        // A FrameWidgetSlot owns the instantiated view's final dimensions, so
        // keep it synchronized with the authored expanded width or compact width.
        if (FrameWidget.Cast(m_wViewSize.GetParent()))
            FrameSlot.SetSizeX(m_wViewSize, width);
    }

    protected void SetViewHeight(float height)
    {
        if (!m_wViewSize)
            return;

        m_wViewSize.SetHeightOverride(height);

        // The preview layout instantiates the briefing in a fixed, top-aligned
        // FrameWidgetSlot. Synchronize its height so only the bottom edge moves.
        if (FrameWidget.Cast(m_wViewSize.GetParent()))
            FrameSlot.SetSizeY(m_wViewSize, height);
    }
}
