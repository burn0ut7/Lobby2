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
    protected static const float MARKER_FOCUS_PAN_DURATION = 0.4;
    protected static const float MARKER_FOCUS_ZOOM_DURATION = 0.7;
    protected static const float MARKER_FOCUS_TARGET_SCREEN_FRACTION = 0.34;
    protected static const float MARKER_FOCUS_PANEL_MARGIN_FRACTION = 0.015;
    protected static const float MARKER_FOCUS_MIN_AVAILABLE_WIDTH_FRACTION = 0.25;
    protected static const float MARKER_DESCRIPTION_HEIGHT = 30;
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
    protected float m_fExpandedWidth;
    protected float m_fMaxViewHeight;
    protected SCR_MapEntity m_MarkerFocusMap;
    protected vector m_vMarkerFocusWorld;
    protected float m_fMarkerFocusZoom;
    protected string m_sMarkerFocusName;
    protected ref array<SCR_ButtonBaseComponent> m_aInlineButtons = {};
    protected ref array<string> m_aInlineActionIds = {};
    protected ref array<string> m_aInlineMarkerNames = {};
    protected ref array<string> m_aInlineLinkUrls = {};
    protected ref array<RichTextWidget> m_aBlockTextWidgets = {};
    protected ref array<SizeLayoutWidget> m_aBlockImageSizes = {};
    protected ref array<float> m_aBlockImageNativeWidths = {};
    protected ref array<float> m_aBlockImageNativeHeights = {};
    protected ref array<float> m_aBlockImageRequestedWidths = {};
    protected ref array<float> m_aBlockImageRequestedHeights = {};
    protected ref array<float> m_aBlockImageDisplayHeights = {};

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
        m_wBlockImageFlow = w.FindAnyWidget("BriefingBlockImageFlow");
        if (m_wBlockImageFlow)
            m_ScenarioComponent = GC_BriefingScenarioComponent.Cast(m_wBlockImageFlow.FindHandler(GC_BriefingScenarioComponent));

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
        ClearActionMarker();
        ClearBlockImageFlow();

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
        m_aInlineButtons.Clear();
        m_aInlineActionIds.Clear();
        m_aInlineMarkerNames.Clear();
        m_aInlineLinkUrls.Clear();
        m_aBlockTextWidgets.Clear();
        m_aBlockImageSizes.Clear();
        m_aBlockImageNativeWidths.Clear();
        m_aBlockImageNativeHeights.Clear();
        m_aBlockImageRequestedWidths.Clear();
        m_aBlockImageRequestedHeights.Clear();
        m_aBlockImageDisplayHeights.Clear();

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
            contentHeight = 0;
            if (m_ScenarioComponent)
                contentHeight += m_ScenarioComponent.GetContentHeight();

            foreach (RichTextWidget textBlock : m_aBlockTextWidgets)
            {
                if (textBlock)
                    contentHeight += textBlock.GetContentHeight();
            }

            foreach (float imageHeight : m_aBlockImageDisplayHeights)
                contentHeight += imageHeight + BLOCK_IMAGE_VERTICAL_PADDING;
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
        if (content.Contains("<b>") || content.Contains("<i>"))
            return true;
        if (content.Contains("<outline ") || content.Contains("<shadow "))
            return true;
        if (content.Contains("<image ") || content.Contains("<color ") || content.Contains("<font "))
            return true;
        if (content.Contains(BLOCK_IMAGE_TAG_PREFIX))
            return true;
        if (content.Contains(SCENARIO_TAG))
            return true;
        if (content.Contains("<p>") || content.Contains("<p "))
            return true;
        if (content.Contains("<h>") || content.Contains("<h "))
            return true;
        if (content.Contains("<h1>") || content.Contains("<h1 "))
            return true;
        if (content.Contains("<h2>") || content.Contains("<h2 "))
            return true;
        if (content.Contains("<br") || content.Contains("<ucs "))
            return true;
        if (content.Contains("<action name=") || content.Contains("<key "))
            return true;
        if (content.Contains(ACTION_TAG_PREFIX) || content.Contains(MARKER_TAG_PREFIX) || content.Contains(LINK_TAG_PREFIX))
            return true;
        if (content.Contains(UNDERLINE_TAG_OPEN) || content.Contains(STRIKE_TAG_OPEN))
            return true;
        if (content.Contains(STRIKE_LONG_TAG_OPEN) || content.Contains(SIZE_TAG_PREFIX))
            return true;

        return false;
    }

    protected bool ConfigureScenarioFlow()
    {
        if (!m_wBlockImageFlow || !m_ScenarioComponent || !m_ScenarioComponent.Show())
            return false;

        m_wBlockImageFlow.SetVisible(true);
        return true;
    }

    //! Lobby2 block images intentionally sit between rich-text chunks instead
    //! of participating in glyph wrapping. Native <image set="..."> remains
    //! available for small inline imageset icons.
    protected bool ConfigureBlockImageFlow(string content)
    {
        ClearBlockImageFlow();
        if (!m_wBlockImageFlow)
            return false;

        int cursor;
        int imageCount;
        while (cursor < content.Length())
        {
            int tagStart = content.IndexOfFrom(cursor, BLOCK_IMAGE_TAG_PREFIX);
            if (tagStart < 0)
            {
                AddBlockText(content.Substring(cursor, content.Length() - cursor));
                cursor = content.Length();
                continue;
            }

            AddBlockText(content.Substring(cursor, tagStart - cursor));

            int tagEnd = content.IndexOfFrom(tagStart, BLOCK_IMAGE_TAG_CLOSE);
            if (tagEnd < 0 || imageCount >= MAX_BLOCK_IMAGES)
            {
                ClearBlockImageFlow();
                return false;
            }

            int afterTag = tagEnd + BLOCK_IMAGE_TAG_CLOSE.Length();
            string tag = content.Substring(tagStart, afterTag - tagStart);
            if (!AddBlockImage(tag))
            {
                ClearBlockImageFlow();
                return false;
            }

            imageCount++;
            cursor = afterTag;
        }

        if (imageCount == 0)
            return false;

        m_wBlockImageFlow.SetVisible(true);
        return true;
    }

    protected bool HasInlineImageTag(string content)
    {
        int cursor;
        while (cursor < content.Length())
        {
            int tagStart = content.IndexOfFrom(cursor, BLOCK_IMAGE_TAG_PREFIX);
            if (tagStart < 0)
                return false;

            int tagEnd = content.IndexOfFrom(tagStart, BLOCK_IMAGE_TAG_CLOSE);
            if (tagEnd < 0)
                return false;

            int afterTag = tagEnd + BLOCK_IMAGE_TAG_CLOSE.Length();
            string tag = content.Substring(tagStart, afterTag - tagStart);
            if (tag.Contains("inline=\"true\""))
                return true;

            cursor = afterTag;
        }

        return false;
    }

    protected bool HasBlockImageTag(string content)
    {
        int cursor;
        while (cursor < content.Length())
        {
            int tagStart = content.IndexOfFrom(cursor, BLOCK_IMAGE_TAG_PREFIX);
            if (tagStart < 0)
                return false;

            int tagEnd = content.IndexOfFrom(tagStart, BLOCK_IMAGE_TAG_CLOSE);
            if (tagEnd < 0)
                return true;

            int afterTag = tagEnd + BLOCK_IMAGE_TAG_CLOSE.Length();
            string tag = content.Substring(tagStart, afterTag - tagStart);
            if (!tag.Contains("inline=\"true\""))
                return true;

            cursor = afterTag;
        }

        return false;
    }

    protected void AddBlockText(string content)
    {
        if (content.IsEmpty() || !m_wBlockImageFlow)
            return;

        Widget textRoot = GetGame().GetWorkspace().CreateWidgets(BLOCK_TEXT_LAYOUT, m_wBlockImageFlow);
        RichTextWidget textBlock = RichTextWidget.Cast(textRoot);
        if (!textBlock)
        {
            if (textRoot)
                textRoot.RemoveFromHierarchy();

            return;
        }

        textBlock.SetText(content);
        m_aBlockTextWidgets.Insert(textBlock);
    }

    protected bool AddBlockImage(string tag)
    {
        string resourceValue;
        if (!GetBlockImageAttribute(tag, "resource", resourceValue)
            || resourceValue.IsEmpty()
            || resourceValue.Length() > MAX_IMAGE_RESOURCE_LENGTH)
            return false;

        string imageName;
        bool hasImageName = tag.Contains("name=\"");
        if (hasImageName
            && (!GetBlockImageAttribute(tag, "name", imageName)
                || imageName.IsEmpty()
                || imageName.Length() > MAX_IMAGE_NAME_LENGTH))
            return false;

        float requestedWidth;
        if (!ParseBlockImageDimension(tag, "width", requestedWidth))
            return false;

        float requestedHeight;
        if (!ParseBlockImageDimension(tag, "height", requestedHeight))
            return false;

        Widget imageRoot = GetGame().GetWorkspace().CreateWidgets(BLOCK_IMAGE_LAYOUT, m_wBlockImageFlow);
        SizeLayoutWidget imageSize = SizeLayoutWidget.Cast(imageRoot);
        ImageWidget imageWidget;
        if (imageRoot)
            imageWidget = ImageWidget.Cast(imageRoot.FindAnyWidget("BriefingBlockImage"));
        if (!imageSize || !imageWidget)
        {
            if (imageRoot)
                imageRoot.RemoveFromHierarchy();

            return false;
        }

        ResourceName resource = resourceValue;
        bool loaded;
        if (resourceValue.EndsWith(".imageset"))
        {
            if (!hasImageName)
            {
                imageRoot.RemoveFromHierarchy();
                return false;
            }

            loaded = imageWidget.LoadImageFromSet(0, resource, imageName);
        }
        else
        {
            loaded = imageWidget.LoadImageTexture(0, resource);
        }

        if (!loaded)
        {
            PrintFormat("GC | Failed to load briefing block image: resource=%1 name=%2",
                resourceValue, imageName, level: LogLevel.WARNING);
            imageRoot.RemoveFromHierarchy();
            return false;
        }

        int nativeWidth;
        int nativeHeight;
        imageWidget.GetImageSize(0, nativeWidth, nativeHeight);
        if (nativeWidth <= 0 || nativeHeight <= 0)
        {
            imageRoot.RemoveFromHierarchy();
            return false;
        }

        m_aBlockImageSizes.Insert(imageSize);
        m_aBlockImageNativeWidths.Insert(nativeWidth);
        m_aBlockImageNativeHeights.Insert(nativeHeight);
        m_aBlockImageRequestedWidths.Insert(requestedWidth);
        m_aBlockImageRequestedHeights.Insert(requestedHeight);
        m_aBlockImageDisplayHeights.Insert(nativeHeight);
        return true;
    }

    protected bool GetBlockImageAttribute(string tag, string attribute, out string value)
    {
        value = string.Empty;
        string prefix = attribute + "=\"";
        int valueStart = tag.IndexOf(prefix);
        if (valueStart < 0)
            return false;

        valueStart += prefix.Length();
        int valueEnd = tag.IndexOfFrom(valueStart, "\"");
        if (valueEnd < 0)
            return false;

        value = tag.Substring(valueStart, valueEnd - valueStart);
        return true;
    }

    //! Missing width or height is valid and represented as zero. If both are
    //! present they form a maximum box rather than distorting the image.
    protected bool ParseBlockImageDimension(string tag, string attribute, out float dimension)
    {
        dimension = 0;
        if (!tag.Contains(attribute + "=\""))
            return true;

        string value;
        if (!GetBlockImageAttribute(tag, attribute, value))
            return false;

        dimension = value.ToFloat(-1);
        return dimension >= MIN_BLOCK_IMAGE_DIMENSION && dimension <= MAX_BLOCK_IMAGE_DIMENSION;
    }

    protected void ResizeBlockImages()
    {
        if (!m_wSectionScroll || m_aBlockImageSizes.IsEmpty())
            return;

        float availableWidth;
        float unusedHeight;
        m_wSectionScroll.GetScreenSize(availableWidth, unusedHeight);
        WorkspaceWidget workspace = GetGame().GetWorkspace();
        if (workspace)
            availableWidth = workspace.DPIUnscale(availableWidth);
        if (availableWidth <= 0)
            return;

        for (int index = 0; index < m_aBlockImageSizes.Count(); index++)
        {
            SizeLayoutWidget imageSize = m_aBlockImageSizes[index];
            if (!imageSize)
                continue;

            float nativeWidth = m_aBlockImageNativeWidths[index];
            float nativeHeight = m_aBlockImageNativeHeights[index];
            float requestedWidth = m_aBlockImageRequestedWidths[index];
            float requestedHeight = m_aBlockImageRequestedHeights[index];

            float scale = 1;
            if (requestedWidth > 0)
                scale = requestedWidth / nativeWidth;

            if (requestedHeight > 0)
            {
                float heightScale = requestedHeight / nativeHeight;
                if (requestedWidth <= 0 || heightScale < scale)
                    scale = heightScale;
            }

            float displayWidth = nativeWidth * scale;
            float displayHeight = nativeHeight * scale;
            if (displayWidth > availableWidth)
            {
                float widthScale = availableWidth / displayWidth;
                displayWidth *= widthScale;
                displayHeight *= widthScale;
            }

            if (displayHeight > MAX_BLOCK_IMAGE_DIMENSION)
            {
                float heightClampScale = MAX_BLOCK_IMAGE_DIMENSION / displayHeight;
                displayWidth *= heightClampScale;
                displayHeight *= heightClampScale;
            }

            imageSize.SetWidthOverride(displayWidth);
            imageSize.SetHeightOverride(displayHeight);
            m_aBlockImageDisplayHeights[index] = displayHeight;
        }
    }

    protected void ClearBlockImageFlow()
    {
        if (m_ScenarioComponent)
            m_ScenarioComponent.Clear();

        if (m_wBlockImageFlow)
        {
            m_wBlockImageFlow.SetVisible(false);
            Widget child = m_wBlockImageFlow.GetChildren();
            while (child)
            {
                Widget nextChild = child.GetSibling();
                child.RemoveFromHierarchy();
                child = nextChild;
            }
        }

        m_aBlockTextWidgets.Clear();
        m_aBlockImageSizes.Clear();
        m_aBlockImageNativeWidths.Clear();
        m_aBlockImageNativeHeights.Clear();
        m_aBlockImageRequestedWidths.Clear();
        m_aBlockImageRequestedHeights.Clear();
        m_aBlockImageDisplayHeights.Clear();
    }

    protected bool ConfigureInteractiveTag(string content)
    {
        ClearActionMarker();

        if (!m_wActionMarkerFlow || !m_wActionMarkerBefore || !m_wActionInlineFlow || !m_wActionMarkerAfter)
            return false;

        int firstTagType;
        int firstTagStart;
        if (!FindNextInlineTag(content, 0, firstTagType, firstTagStart))
            return false;

        // The custom tags' complete plain-text paragraph becomes a wrapping
        // inline flow. Content outside that paragraph remains native rich text.
        string markerPrefix = content.Substring(0, firstTagStart);
        int paragraphStart = markerPrefix.LastIndexOf("<p>");
        int previousParagraphEnd = markerPrefix.LastIndexOf("</p>");
        if (paragraphStart < 0 || paragraphStart < previousParagraphEnd)
            return false;

        int paragraphEnd = content.IndexOfFrom(firstTagStart, "</p>");
        if (paragraphEnd < 0)
            return false;

        int inlineStart = paragraphStart + 3;
        m_wActionMarkerBefore.SetText(content.Substring(0, paragraphStart));
        m_wActionMarkerAfter.SetText(content.Substring(paragraphEnd + 4, content.Length() - paragraphEnd - 4));

        string inlineContent = content.Substring(inlineStart, paragraphEnd - inlineStart);
        int cursor;
        while (cursor < inlineContent.Length())
        {
            int tagType;
            int tagStart;
            if (!FindNextInlineTag(inlineContent, cursor, tagType, tagStart))
            {
                string remainingText = inlineContent.Substring(cursor, inlineContent.Length() - cursor);
                if (remainingText.Contains("<"))
                    return false;

                AddInlineTextTokens(remainingText);
                cursor = inlineContent.Length();
                continue;
            }

            string precedingText = inlineContent.Substring(cursor, tagStart - cursor);
            if (precedingText.Contains("<"))
                return false;

            AddInlineTextTokens(precedingText);

            int afterTag;
            if (!AddInlineTag(inlineContent, tagType, tagStart, afterTag))
            {
                ClearActionMarker();
                return false;
            }

            cursor = afterTag;
        }

        m_wActionMarkerFlow.SetVisible(true);

        return true;
    }

    protected bool FindNextInlineTag(string content, int searchStart, out int tagType, out int tagStart)
    {
        tagStart = -1;
        tagType = -1;

        int candidate = content.IndexOfFrom(searchStart, ACTION_TAG_PREFIX);
        if (candidate >= 0)
        {
            tagStart = candidate;
            tagType = INLINE_TAG_ACTION;
        }

        candidate = content.IndexOfFrom(searchStart, MARKER_TAG_PREFIX);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_MARKER;
        }

        candidate = content.IndexOfFrom(searchStart, UNDERLINE_TAG_OPEN);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_UNDERLINE;
        }

        candidate = content.IndexOfFrom(searchStart, STRIKE_TAG_OPEN);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_STRIKE;
        }

        candidate = content.IndexOfFrom(searchStart, STRIKE_LONG_TAG_OPEN);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_STRIKE;
        }

        candidate = content.IndexOfFrom(searchStart, SIZE_TAG_PREFIX);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_SIZE;
        }

        candidate = content.IndexOfFrom(searchStart, LINK_TAG_PREFIX);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_LINK;
        }

        candidate = content.IndexOfFrom(searchStart, BLOCK_IMAGE_TAG_PREFIX);
        if (candidate >= 0 && (tagStart < 0 || candidate < tagStart))
        {
            tagStart = candidate;
            tagType = INLINE_TAG_IMAGE;
        }

        return tagStart >= 0;
    }

    protected bool AddInlineTag(string content, int tagType, int tagStart, out int afterTag)
    {
        if (tagType == INLINE_TAG_IMAGE)
        {
            int imageTagEnd = content.IndexOfFrom(tagStart, BLOCK_IMAGE_TAG_CLOSE);
            if (imageTagEnd < 0)
                return false;

            afterTag = imageTagEnd + BLOCK_IMAGE_TAG_CLOSE.Length();
            string imageTag = content.Substring(tagStart, afterTag - tagStart);
            if (!imageTag.Contains("inline=\"true\""))
                return false;

            return AddInlineImage(imageTag);
        }

        string tagOpen;
        string tagClose;
        string tagValue;
        int labelStart;

        switch (tagType)
        {
            case INLINE_TAG_ACTION:
                tagOpen = ACTION_TAG_PREFIX;
                tagClose = ACTION_TAG_CLOSE;
                break;

            case INLINE_TAG_MARKER:
                tagOpen = MARKER_TAG_PREFIX;
                tagClose = MARKER_TAG_CLOSE;
                break;

            case INLINE_TAG_UNDERLINE:
                tagOpen = UNDERLINE_TAG_OPEN;
                tagClose = UNDERLINE_TAG_CLOSE;
                break;

            case INLINE_TAG_STRIKE:
                tagOpen = STRIKE_TAG_OPEN;
                tagClose = STRIKE_TAG_CLOSE;
                if (content.IndexOfFrom(tagStart, STRIKE_LONG_TAG_OPEN) == tagStart)
                {
                    tagOpen = STRIKE_LONG_TAG_OPEN;
                    tagClose = STRIKE_LONG_TAG_CLOSE;
                }
                break;

            case INLINE_TAG_SIZE:
                tagOpen = SIZE_TAG_PREFIX;
                tagClose = SIZE_TAG_CLOSE;
                break;

            case INLINE_TAG_LINK:
                tagOpen = LINK_TAG_PREFIX;
                tagClose = LINK_TAG_CLOSE;
                break;
        }

        if (tagType == INLINE_TAG_ACTION || tagType == INLINE_TAG_MARKER || tagType == INLINE_TAG_SIZE || tagType == INLINE_TAG_LINK)
        {
            int valueStart = tagStart + tagOpen.Length();
            string valueTail = content.Substring(valueStart, content.Length() - valueStart);
            int attributeEnd = valueTail.IndexOf(TAG_ATTRIBUTE_CLOSE);
            if (attributeEnd < 0)
                return false;

            tagValue = valueTail.Substring(0, attributeEnd);
            int maxValueLength = MAX_TAG_VALUE_LENGTH;
            if (tagType == INLINE_TAG_LINK)
                maxValueLength = MAX_LINK_URL_LENGTH;

            if (tagValue.IsEmpty() || tagValue.Length() > maxValueLength)
                return false;

            if (tagType == INLINE_TAG_LINK && !IsValidExternalUrl(tagValue))
                return false;

            labelStart = valueStart + attributeEnd + TAG_ATTRIBUTE_CLOSE.Length();
        }
        else
        {
            labelStart = tagStart + tagOpen.Length();
        }

        int labelEnd = content.IndexOfFrom(labelStart, tagClose);
        if (labelEnd < 0)
            return false;

        string label = content.Substring(labelStart, labelEnd - labelStart);
        if (label.IsEmpty())
            label = tagValue;

        bool underline = tagType == INLINE_TAG_UNDERLINE || tagType == INLINE_TAG_LINK;
        bool strike = tagType == INLINE_TAG_STRIKE;
        float fontScale = 1.0;
        if (tagType == INLINE_TAG_SIZE)
        {
            fontScale = tagValue.ToFloat(-1);
            if (fontScale < MIN_INLINE_FONT_SCALE || fontScale > MAX_INLINE_FONT_SCALE)
                return false;
        }

        string cleanLabel;
        if (!ParseLabelFormatting(label, cleanLabel, underline, strike, fontScale))
            return false;

        afterTag = labelEnd + tagClose.Length();
        if (tagType == INLINE_TAG_ACTION)
            return AddInlineAction(cleanLabel, tagValue, string.Empty, string.Empty, underline, strike, fontScale);

        if (tagType == INLINE_TAG_MARKER)
            return AddInlineAction(cleanLabel, string.Empty, tagValue, string.Empty, underline, strike, fontScale);

        if (tagType == INLINE_TAG_LINK)
            return AddInlineAction(cleanLabel, string.Empty, string.Empty, tagValue, underline, strike, fontScale);

        return AddInlineDecoration(cleanLabel, underline, strike, fontScale);
    }

    protected bool AddInlineImage(string tag)
    {
        string resourceValue;
        if (!GetBlockImageAttribute(tag, "resource", resourceValue)
            || resourceValue.IsEmpty()
            || resourceValue.Length() > MAX_IMAGE_RESOURCE_LENGTH)
            return false;

        string imageName;
        bool hasImageName = tag.Contains("name=\"");
        if (hasImageName
            && (!GetBlockImageAttribute(tag, "name", imageName)
                || imageName.IsEmpty()
                || imageName.Length() > MAX_IMAGE_NAME_LENGTH))
            return false;

        float requestedWidth;
        if (!ParseBlockImageDimension(tag, "width", requestedWidth)
            || requestedWidth > MAX_INLINE_IMAGE_DIMENSION)
            return false;

        float requestedHeight;
        if (!ParseBlockImageDimension(tag, "height", requestedHeight)
            || requestedHeight > MAX_INLINE_IMAGE_DIMENSION)
            return false;

        Widget imageRoot = GetGame().GetWorkspace().CreateWidgets(INLINE_IMAGE_LAYOUT, m_wActionInlineFlow);
        SizeLayoutWidget imageSize = SizeLayoutWidget.Cast(imageRoot);
        ImageWidget imageWidget;
        if (imageRoot)
            imageWidget = ImageWidget.Cast(imageRoot.FindAnyWidget("BriefingInlineImage"));
        if (!imageSize || !imageWidget)
        {
            if (imageRoot)
                imageRoot.RemoveFromHierarchy();

            return false;
        }

        ResourceName resource = resourceValue;
        bool loaded;
        if (resourceValue.EndsWith(".imageset"))
        {
            if (!hasImageName)
            {
                imageRoot.RemoveFromHierarchy();
                return false;
            }

            loaded = imageWidget.LoadImageFromSet(0, resource, imageName);
        }
        else
        {
            loaded = imageWidget.LoadImageTexture(0, resource);
        }

        if (!loaded)
        {
            imageRoot.RemoveFromHierarchy();
            return false;
        }

        int nativeWidth;
        int nativeHeight;
        imageWidget.GetImageSize(0, nativeWidth, nativeHeight);
        if (nativeWidth <= 0 || nativeHeight <= 0)
        {
            imageRoot.RemoveFromHierarchy();
            return false;
        }

        float displayWidth = requestedWidth;
        float displayHeight = requestedHeight;
        if (displayWidth <= 0 && displayHeight <= 0)
            displayHeight = DEFAULT_INLINE_IMAGE_DIMENSION;

        if (displayWidth <= 0)
            displayWidth = nativeWidth * (displayHeight / nativeHeight);
        else if (displayHeight <= 0)
            displayHeight = nativeHeight * (displayWidth / nativeWidth);
        else
        {
            float widthScale = displayWidth / nativeWidth;
            float heightScale = displayHeight / nativeHeight;
            float scale = widthScale;
            if (heightScale < scale)
                scale = heightScale;

            displayWidth = nativeWidth * scale;
            displayHeight = nativeHeight * scale;
        }

        if (displayWidth > MAX_INLINE_IMAGE_DIMENSION || displayHeight > MAX_INLINE_IMAGE_DIMENSION)
        {
            float clampScale = MAX_INLINE_IMAGE_DIMENSION / displayWidth;
            if (displayHeight > displayWidth)
                clampScale = MAX_INLINE_IMAGE_DIMENSION / displayHeight;

            displayWidth *= clampScale;
            displayHeight *= clampScale;
        }

        imageSize.SetWidthOverride(displayWidth);
        imageSize.SetHeightOverride(displayHeight);
        return true;
    }

    protected bool ParseLabelFormatting(string label, out string cleanLabel, inout bool underline, inout bool strike, inout float fontScale)
    {
        cleanLabel = label;
        bool changed = true;
        while (changed)
        {
            changed = false;
            if (cleanLabel.StartsWith(UNDERLINE_TAG_OPEN) && cleanLabel.EndsWith(UNDERLINE_TAG_CLOSE))
            {
                cleanLabel = cleanLabel.Substring(UNDERLINE_TAG_OPEN.Length(), cleanLabel.Length() - UNDERLINE_TAG_OPEN.Length() - UNDERLINE_TAG_CLOSE.Length());
                underline = true;
                changed = true;
            }
            else if (cleanLabel.StartsWith(STRIKE_TAG_OPEN) && cleanLabel.EndsWith(STRIKE_TAG_CLOSE))
            {
                cleanLabel = cleanLabel.Substring(STRIKE_TAG_OPEN.Length(), cleanLabel.Length() - STRIKE_TAG_OPEN.Length() - STRIKE_TAG_CLOSE.Length());
                strike = true;
                changed = true;
            }
            else if (cleanLabel.StartsWith(STRIKE_LONG_TAG_OPEN) && cleanLabel.EndsWith(STRIKE_LONG_TAG_CLOSE))
            {
                cleanLabel = cleanLabel.Substring(STRIKE_LONG_TAG_OPEN.Length(), cleanLabel.Length() - STRIKE_LONG_TAG_OPEN.Length() - STRIKE_LONG_TAG_CLOSE.Length());
                strike = true;
                changed = true;
            }
            else if (cleanLabel.StartsWith(SIZE_TAG_PREFIX) && cleanLabel.EndsWith(SIZE_TAG_CLOSE))
            {
                int scaleStart = SIZE_TAG_PREFIX.Length();
                string sizeTail = cleanLabel.Substring(scaleStart, cleanLabel.Length() - scaleStart);
                int attributeEnd = sizeTail.IndexOf(TAG_ATTRIBUTE_CLOSE);
                if (attributeEnd < 0)
                    return false;

                float nestedScale = sizeTail.Substring(0, attributeEnd).ToFloat(-1);
                if (nestedScale < MIN_INLINE_FONT_SCALE || nestedScale > MAX_INLINE_FONT_SCALE)
                    return false;

                int sizeLabelStart = scaleStart + attributeEnd + TAG_ATTRIBUTE_CLOSE.Length();
                cleanLabel = cleanLabel.Substring(sizeLabelStart, cleanLabel.Length() - sizeLabelStart - SIZE_TAG_CLOSE.Length());
                fontScale = nestedScale;
                changed = true;
            }
        }

        if (cleanLabel.IsEmpty()
            || cleanLabel.Contains(ACTION_TAG_PREFIX)
            || cleanLabel.Contains(MARKER_TAG_PREFIX)
            || cleanLabel.Contains(LINK_TAG_PREFIX)
            || cleanLabel.Contains(UNDERLINE_TAG_OPEN)
            || cleanLabel.Contains(STRIKE_TAG_OPEN)
            || cleanLabel.Contains(STRIKE_LONG_TAG_OPEN)
            || cleanLabel.Contains("<size"))
            return false;

        return true;
    }

    protected void AddInlineTextTokens(string text)
    {
        if (text.IsEmpty() || !m_wActionInlineFlow)
            return;

        array<string> words = {};
        text.Split(" ", words, true);
        bool startsWithSpace = text.StartsWith(" ");
        bool endsWithSpace = text.EndsWith(" ");

        for (int index = 0; index < words.Count(); index++)
        {
            Widget tokenRoot = GetGame().GetWorkspace().CreateWidgets(INLINE_TEXT_LAYOUT, m_wActionInlineFlow);
            TextWidget token = TextWidget.Cast(tokenRoot);
            if (!token)
            {
                if (tokenRoot)
                    tokenRoot.RemoveFromHierarchy();

                continue;
            }

            string tokenText = words[index];
            if (index == 0 && startsWithSpace)
                tokenText = " " + tokenText;

            if (index < words.Count() - 1 || endsWithSpace)
                tokenText += " ";

            token.SetText(tokenText);
        }
    }

    protected bool AddInlineAction(string label, string actionId, string markerName, string linkUrl, bool underline, bool strike, float fontScale)
    {
        Widget actionRoot = GetGame().GetWorkspace().CreateWidgets(INLINE_ACTION_LAYOUT, m_wActionInlineFlow);
        if (!actionRoot)
            return false;

        RichTextWidget actionLabel = RichTextWidget.Cast(actionRoot.FindAnyWidget("BriefingInlineActionLabel"));
        Widget actionButton = actionRoot.FindAnyWidget("BriefingInlineActionButton");
        if (!actionLabel || !actionButton)
            return false;

        actionLabel.SetText(label);
        actionLabel.SetExactFontSize(INLINE_BASE_FONT_SIZE * fontScale);
        SetInlineDecorationVisibility(actionRoot, underline, strike);

        SCR_ButtonBaseComponent actionButtonComponent = SCR_ButtonBaseComponent.Cast(actionButton.FindHandler(SCR_ButtonBaseComponent));
        if (!actionButtonComponent)
            return false;

        m_aInlineButtons.Insert(actionButtonComponent);
        m_aInlineActionIds.Insert(actionId);
        m_aInlineMarkerNames.Insert(markerName);
        m_aInlineLinkUrls.Insert(linkUrl);
        actionButtonComponent.m_OnClicked.Insert(OnInlineActionClicked);
        return true;
    }

    protected bool AddInlineDecoration(string label, bool underline, bool strike, float fontScale)
    {
        Widget decorationRoot = GetGame().GetWorkspace().CreateWidgets(INLINE_DECORATION_LAYOUT, m_wActionInlineFlow);
        if (!decorationRoot)
            return false;

        RichTextWidget decorationLabel = RichTextWidget.Cast(decorationRoot.FindAnyWidget("BriefingInlineDecorationLabel"));
        if (!decorationLabel)
            return false;

        decorationLabel.SetText(label);
        decorationLabel.SetExactFontSize(INLINE_BASE_FONT_SIZE * fontScale);
        SetInlineDecorationVisibility(decorationRoot, underline, strike);
        return true;
    }

    protected void SetInlineDecorationVisibility(Widget root, bool underline, bool strike)
    {
        Widget underlineLine = root.FindAnyWidget("BriefingInlineUnderline");
        if (underlineLine)
            underlineLine.SetVisible(underline);

        Widget strikeLine = root.FindAnyWidget("BriefingInlineStrike");
        if (strikeLine)
            strikeLine.SetVisible(strike);
    }

    protected void ClearActionMarker()
    {
        if (m_wActionMarkerFlow)
            m_wActionMarkerFlow.SetVisible(false);

        if (m_wActionMarkerBefore)
            m_wActionMarkerBefore.SetText(string.Empty);

        if (m_wActionMarkerAfter)
            m_wActionMarkerAfter.SetText(string.Empty);

        foreach (SCR_ButtonBaseComponent actionButton : m_aInlineButtons)
        {
            if (actionButton)
                actionButton.m_OnClicked.Remove(OnInlineActionClicked);
        }

        m_aInlineButtons.Clear();
        m_aInlineActionIds.Clear();
        m_aInlineMarkerNames.Clear();
        m_aInlineLinkUrls.Clear();

        if (m_wActionInlineFlow)
        {
            Widget child = m_wActionInlineFlow.GetChildren();
            while (child)
            {
                Widget nextChild = child.GetSibling();
                child.RemoveFromHierarchy();
                child = nextChild;
            }
        }
    }

    protected void OnInlineActionClicked(SCR_ButtonBaseComponent button)
    {
        int buttonIndex = m_aInlineButtons.Find(button);
        if (buttonIndex < 0)
            return;

        string linkUrl = m_aInlineLinkUrls[buttonIndex];
        if (!linkUrl.IsEmpty())
        {
            OpenExternalLink(linkUrl);
            return;
        }

        string markerName = m_aInlineMarkerNames[buttonIndex];
        if (!markerName.IsEmpty())
        {
            FocusBriefingMarker(markerName);
            return;
        }

        ExecuteBriefingAction(m_aInlineActionIds[buttonIndex]);
    }

    protected bool IsValidExternalUrl(string url)
    {
        if (url.IsEmpty() || url.Length() > MAX_LINK_URL_LENGTH)
            return false;

        if (!url.StartsWith("https://") && !url.StartsWith("http://"))
            return false;

        if (url.Contains("\"") || url.Contains("<") || url.Contains(">") || url.Contains(" "))
            return false;

        if (url.Contains("\\") || url.Contains("\t") || url.Contains("\r") || url.Contains("\n"))
            return false;

        int hostStart = url.IndexOf("://") + 3;
        if (hostStart < 3 || hostStart >= url.Length())
            return false;

        string hostAndPath = url.Substring(hostStart, url.Length() - hostStart);
        return !hostAndPath.StartsWith("/") && !hostAndPath.StartsWith("?") && !hostAndPath.StartsWith("#");
    }

    protected void OpenExternalLink(string url)
    {
        if (!IsValidExternalUrl(url))
        {
            PrintFormat("GC | Blocked invalid briefing link: %1", url, level: LogLevel.WARNING);
            return;
        }

        PlatformService platformService = GetGame().GetPlatformService();
        if (!platformService)
        {
            PrintFormat("GC | Unable to open briefing link because the platform service is unavailable: %1", url, level: LogLevel.WARNING);
            return;
        }

        platformService.OpenBrowser(url);
    }

    protected void FocusBriefingMarker(string markerName)
    {
        if (!m_LobbyManager)
            m_LobbyManager = GC_LobbyManager.GetManager();

        PlayerController playerController = GetGame().GetPlayerController();
        if (!m_LobbyManager || !playerController)
        {
            PrintFormat("GC | Briefing marker focus failed: marker '%1' cannot be resolved because briefing player data is unavailable", markerName, level: LogLevel.WARNING);
            return;
        }

        GC_BriefingMarker focusMarker;
        array<ref GC_BriefingMarker> visibleMarkers = m_LobbyManager.GetBriefingMarkersForPlayer(playerController.GetPlayerId());
        foreach (GC_BriefingMarker marker : visibleMarkers)
        {
            if (marker && marker.GetName() == markerName)
            {
                focusMarker = marker;
                break;
            }
        }

        if (!focusMarker)
        {
            PrintFormat("GC | Briefing marker focus failed: marker '%1' was not found or is not visible to the current player", markerName, level: LogLevel.WARNING);
            return;
        }

        SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
        if (!mapEntity || !mapEntity.IsOpen())
        {
            PrintFormat("GC | Briefing marker focus failed: map is not open for marker '%1'", markerName, level: LogLevel.WARNING);
            return;
        }

        Widget mapRoot = mapEntity.GetMapMenuRoot();
        Widget mapFrame;
        if (mapRoot)
            mapFrame = mapRoot.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
        if (!mapFrame)
        {
            PrintFormat("GC | Briefing marker focus failed: map frame is unavailable for marker '%1'", markerName, level: LogLevel.WARNING);
            return;
        }

        float mapX;
        float mapY;
        float mapWidth;
        float mapHeight;
        mapFrame.GetScreenPos(mapX, mapY);
        mapFrame.GetScreenSize(mapWidth, mapHeight);
        if (mapWidth <= 0 || mapHeight <= 0)
        {
            PrintFormat("GC | Briefing marker focus failed: map viewport has no size for marker '%1'", markerName, level: LogLevel.WARNING);
            return;
        }

        float availableLeft = mapX;
        float availableRight = mapX + mapWidth;
        float availableTop = mapY;
        float availableBottom = mapY + mapHeight;
        float mapCenterX = mapX + mapWidth * 0.5;
        float mapCenterY = mapY + mapHeight * 0.5;

        if (m_wViewSize)
        {
            float panelX;
            float panelY;
            float panelWidth;
            float panelHeight;
            m_wViewSize.GetScreenPos(panelX, panelY);
            m_wViewSize.GetScreenSize(panelWidth, panelHeight);

            bool overlapsVertically = panelY < mapY + mapHeight && panelY + panelHeight > mapY;
            bool overlapsHorizontally = panelX < mapX + mapWidth && panelX + panelWidth > mapX;
            if (overlapsVertically && overlapsHorizontally)
            {
                float margin = mapWidth * MARKER_FOCUS_PANEL_MARGIN_FRACTION;
                if (panelX + panelWidth * 0.5 <= mapCenterX)
                    availableLeft = panelX + panelWidth + margin;
                else
                    availableRight = panelX - margin;

                if (availableRight - availableLeft < mapWidth * MARKER_FOCUS_MIN_AVAILABLE_WIDTH_FRACTION)
                {
                    availableLeft = mapX;
                    availableRight = mapX + mapWidth;
                }
            }
        }

        float availableWidth = availableRight - availableLeft;
        float availableShortSide = availableWidth;
        if (mapHeight < availableShortSide)
            availableShortSide = mapHeight;

        float currentZoom = mapEntity.GetCurrentZoom();
        if (currentZoom <= 0)
        {
            PrintFormat("GC | Briefing marker focus failed: map zoom is invalid for marker '%1'", markerName, level: LogLevel.WARNING);
            return;
        }

        float targetZoom = currentZoom;
        if (focusMarker.GetSize() > 0)
        {
            float currentMarkerPixels = GetProjectedMarkerSize(mapEntity, focusMarker);
            if (currentMarkerPixels <= 0)
            {
                PrintFormat("GC | Briefing marker focus failed: projected size is invalid for marker '%1'", markerName, level: LogLevel.WARNING);
                return;
            }

            float angleRadians = focusMarker.GetRotation() * Math.DEG2RAD;
            float rotationCos = Math.Cos(angleRadians);
            if (rotationCos < 0)
                rotationCos = -rotationCos;
            float rotationSin = Math.Sin(angleRadians);
            if (rotationSin < 0)
                rotationSin = -rotationSin;

            // A rotated square occupies |cos| + |sin| times its unrotated size.
            float rotationFootprint = rotationCos + rotationSin;
            if (rotationFootprint < 1)
                rotationFootprint = 1;

            float currentVisualPixels = currentMarkerPixels * rotationFootprint;
            float targetVisualPixels = availableShortSide * MARKER_FOCUS_TARGET_SCREEN_FRACTION;
            targetZoom = currentZoom * targetVisualPixels / currentVisualPixels;

            if (targetZoom < mapEntity.GetMinZoom())
                targetZoom = mapEntity.GetMinZoom();
            else if (targetZoom > mapEntity.GetMaxZoom())
                targetZoom = mapEntity.GetMaxZoom();
        }

        float focusScreenX = (availableLeft + availableRight) * 0.5;
        WorkspaceWidget workspace = GetGame().GetWorkspace();
        float descriptionOffset;
        if (workspace && !focusMarker.GetDescription().IsEmpty())
            descriptionOffset = workspace.DPIScale(MARKER_DESCRIPTION_HEIGHT * 0.5);
        float focusScreenY = (availableTop + availableBottom) * 0.5 - descriptionOffset;
        float horizontalScreenOffset = focusScreenX - mapCenterX;
        float verticalScreenOffset = focusScreenY - mapCenterY;
        vector markerPosition = focusMarker.GetWorldPosition();

        float currentFocusWorldX = markerPosition[0] - horizontalScreenOffset / currentZoom;
        float currentFocusWorldZ = markerPosition[2] + verticalScreenOffset / currentZoom;
        int currentPanX;
        int currentPanY;
        mapEntity.WorldToScreen(currentFocusWorldX, currentFocusWorldZ, currentPanX, currentPanY, false);

        CancelMarkerFocus();
        m_MarkerFocusMap = mapEntity;
        m_fMarkerFocusZoom = targetZoom;
        m_vMarkerFocusWorld = {
            markerPosition[0] - horizontalScreenOffset / targetZoom,
            markerPosition[1],
            markerPosition[2] + verticalScreenOffset / targetZoom
        };
        m_sMarkerFocusName = markerName;
        SCR_MapEntity.GetOnMapPanEnd().Insert(OnMarkerFocusPanComplete);
        mapEntity.PanSmooth(currentPanX, currentPanY, MARKER_FOCUS_PAN_DURATION);
    }

    //! Measures the marker's authored size as a world footprint through the map
    //! projection. World-scale markers render at this footprint; fixed-pixel
    //! markers use it only to choose a useful contextual focus zoom.
    protected float GetProjectedMarkerSize(SCR_MapEntity mapEntity, GC_BriefingMarker marker)
    {
        vector worldPosition = marker.GetWorldPosition();
        float halfWorldSize = marker.GetSize() * 0.5;

        int leftX;
        int leftY;
        int rightX;
        int rightY;
        int bottomX;
        int bottomY;
        int topX;
        int topY;
        mapEntity.WorldToScreen(worldPosition[0] - halfWorldSize, worldPosition[2], leftX, leftY, false);
        mapEntity.WorldToScreen(worldPosition[0] + halfWorldSize, worldPosition[2], rightX, rightY, false);
        mapEntity.WorldToScreen(worldPosition[0], worldPosition[2] - halfWorldSize, bottomX, bottomY, false);
        mapEntity.WorldToScreen(worldPosition[0], worldPosition[2] + halfWorldSize, topX, topY, false);

        float projectedWidth = rightX - leftX;
        if (projectedWidth < 0)
            projectedWidth = -projectedWidth;
        float projectedHeight = topY - bottomY;
        if (projectedHeight < 0)
            projectedHeight = -projectedHeight;

        return (projectedWidth + projectedHeight) * 0.5;
    }

    protected void OnMarkerFocusPanComplete(float panX, float panY)
    {
        SCR_MapEntity mapEntity = m_MarkerFocusMap;
        vector focusWorld = m_vMarkerFocusWorld;
        float focusZoom = m_fMarkerFocusZoom;
        string markerName = m_sMarkerFocusName;
        CancelMarkerFocus();

        if (!mapEntity || !mapEntity.IsOpen())
        {
            PrintFormat("GC | Briefing marker focus stopped after panning because the map closed: marker='%1'", markerName, level: LogLevel.WARNING);
            return;
        }

        mapEntity.ZoomPanSmooth(focusZoom, focusWorld[0], focusWorld[2], MARKER_FOCUS_ZOOM_DURATION);
    }

    protected void CancelMarkerFocus()
    {
        SCR_MapEntity.GetOnMapPanEnd().Remove(OnMarkerFocusPanComplete);
        m_MarkerFocusMap = null;
        m_vMarkerFocusWorld = vector.Zero;
        m_fMarkerFocusZoom = 0;
        m_sMarkerFocusName = string.Empty;
    }

    protected void ExecuteBriefingAction(string actionId)
    {
        switch (actionId)
        {
            case ACTION_DEBUG_PRINT:
                Print("GC | Briefing action clicked");
                break;

            default:
                PrintFormat("GC | Unsupported briefing action: %1", actionId);
                break;
        }
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
