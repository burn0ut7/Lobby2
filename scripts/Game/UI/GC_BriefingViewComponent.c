class GC_BriefingViewComponent : SCR_ScriptedWidgetComponent
{
    protected static const string ACTION_DEBUG_PRINT = "debug_print";
    protected static const string ACTION_TAG_PREFIX = "<action id=\"";
    protected static const string ACTION_TAG_CLOSE = "</action>";
    protected static const string MARKER_TAG_PREFIX = "<marker name=\"";
    protected static const string MARKER_TAG_CLOSE = "</marker>";
    protected static const string UNDERLINE_TAG_OPEN = "<u>";
    protected static const string UNDERLINE_TAG_CLOSE = "</u>";
    protected static const string STRIKE_TAG_OPEN = "<s>";
    protected static const string STRIKE_TAG_CLOSE = "</s>";
    protected static const string STRIKE_LONG_TAG_OPEN = "<strike>";
    protected static const string STRIKE_LONG_TAG_CLOSE = "</strike>";
    protected static const string SIZE_TAG_PREFIX = "<size scale=\"";
    protected static const string SIZE_TAG_CLOSE = "</size>";
    protected static const string TAG_ATTRIBUTE_CLOSE = "\">";
    protected static const string INVALID_INTERACTIVE_TAG_TEXT = "Invalid Lobby2 briefing interaction tag.";
    protected static const int MAX_TAG_VALUE_LENGTH = 64;
    protected static const ResourceName INLINE_TEXT_LAYOUT = "{6A4D000000000001}UI/layouts/Briefing/Lobby_Briefing_Inline_Text.layout";
    protected static const ResourceName INLINE_ACTION_LAYOUT = "{6A4D000000000002}UI/layouts/Briefing/Lobby_Briefing_Inline_Action.layout";
    protected static const ResourceName INLINE_DECORATION_LAYOUT = "{6A4D000000000003}UI/layouts/Briefing/Lobby_Briefing_Inline_Decoration.layout";
    protected static const int INLINE_TAG_ACTION = 0;
    protected static const int INLINE_TAG_MARKER = 1;
    protected static const int INLINE_TAG_UNDERLINE = 2;
    protected static const int INLINE_TAG_STRIKE = 3;
    protected static const int INLINE_TAG_SIZE = 4;
    protected static const int INLINE_BASE_FONT_SIZE = 24;
    protected static const float MIN_INLINE_FONT_SCALE = 0.75;
    protected static const float MAX_INLINE_FONT_SCALE = 2.0;
    protected static const float COMPACT_WIDTH = 287;
    protected static const float EXPANDED_WIDTH = 720;

    protected GC_BriefingTitlesComponent m_Titles;
    protected GC_LobbyManager m_LobbyManager;
    protected SizeLayoutWidget m_wViewSize;
    protected Widget m_wDocumentPanel;
    protected TextWidget m_wSectionTitle;
    protected TextWidget m_wPlainSectionBody;
    protected RichTextWidget m_wRichSectionBody;
    protected ScrollLayoutWidget m_wSectionScroll;
    protected Widget m_wActionMarkerFlow;
    protected RichTextWidget m_wActionMarkerBefore;
    protected Widget m_wActionInlineFlow;
    protected RichTextWidget m_wActionMarkerAfter;
    protected ref array<SCR_ButtonBaseComponent> m_aInlineButtons = {};
    protected ref array<string> m_aInlineActionIds = {};
    protected ref array<string> m_aInlineMarkerNames = {};

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wViewSize = SizeLayoutWidget.Cast(w);
        m_wDocumentPanel = w.FindAnyWidget("BriefingDocumentPanel");
        m_wSectionTitle = TextWidget.Cast(w.FindAnyWidget("BriefingSectionTitle"));
        m_wPlainSectionBody = TextWidget.Cast(w.FindAnyWidget("BriefingSectionPlainBody"));
        m_wRichSectionBody = RichTextWidget.Cast(w.FindAnyWidget("BriefingSectionRichBody"));
        m_wSectionScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("BriefingSectionScroll"));
        m_wActionMarkerFlow = w.FindAnyWidget("BriefingActionMarkerFlow");
        m_wActionMarkerBefore = RichTextWidget.Cast(w.FindAnyWidget("BriefingActionMarkerBefore"));
        m_wActionInlineFlow = w.FindAnyWidget("BriefingActionInlineFlow");
        m_wActionMarkerAfter = RichTextWidget.Cast(w.FindAnyWidget("BriefingActionMarkerAfter"));

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
        ClearActionMarker();

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
        m_wDocumentPanel = null;
        m_wSectionTitle = null;
        m_wPlainSectionBody = null;
        m_wRichSectionBody = null;
        m_wSectionScroll = null;
        m_wActionMarkerFlow = null;
        m_wActionMarkerBefore = null;
        m_wActionInlineFlow = null;
        m_wActionMarkerAfter = null;
        m_aInlineButtons.Clear();
        m_aInlineActionIds.Clear();
        m_aInlineMarkerNames.Clear();

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

            return;
        }

        SetViewWidth(EXPANDED_WIDTH);

        if (m_wDocumentPanel)
            m_wDocumentPanel.SetVisible(true);

        if (m_wSectionTitle)
            m_wSectionTitle.SetText(briefing.GetOrder().ToString(2) + "  " + briefing.GetTitle());

        string content = briefing.GetContent();
        bool isRichText = HasSupportedRichTextTag(content);
        bool hasCustomTag = isRichText && (content.Contains(ACTION_TAG_PREFIX)
            || content.Contains("<marker")
            || content.Contains(UNDERLINE_TAG_OPEN)
            || content.Contains(STRIKE_TAG_OPEN)
            || content.Contains(STRIKE_LONG_TAG_OPEN)
            || content.Contains("<size"));

        ClearActionMarker();
        bool hasInteractiveTag = hasCustomTag && ConfigureInteractiveTag(content);
        if (m_wPlainSectionBody)
        {
            m_wPlainSectionBody.SetText(content);
            m_wPlainSectionBody.SetVisible(!isRichText);
        }

        if (m_wRichSectionBody)
        {
            m_wRichSectionBody.SetVisible(false);

            if (isRichText && !hasCustomTag)
            {
                m_wRichSectionBody.SetText(content);
                m_wRichSectionBody.SetVisible(true);
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
        if (content.Contains(ACTION_TAG_PREFIX) || content.Contains(MARKER_TAG_PREFIX))
            return true;
        if (content.Contains(UNDERLINE_TAG_OPEN) || content.Contains(STRIKE_TAG_OPEN))
            return true;
        if (content.Contains(STRIKE_LONG_TAG_OPEN) || content.Contains(SIZE_TAG_PREFIX))
            return true;

        return false;
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

        return tagStart >= 0;
    }

    protected bool AddInlineTag(string content, int tagType, int tagStart, out int afterTag)
    {
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
        }

        if (tagType == INLINE_TAG_ACTION || tagType == INLINE_TAG_MARKER || tagType == INLINE_TAG_SIZE)
        {
            int valueStart = tagStart + tagOpen.Length();
            string valueTail = content.Substring(valueStart, content.Length() - valueStart);
            int attributeEnd = valueTail.IndexOf(TAG_ATTRIBUTE_CLOSE);
            if (attributeEnd < 0)
                return false;

            tagValue = valueTail.Substring(0, attributeEnd);
            if (tagValue.IsEmpty() || tagValue.Length() > MAX_TAG_VALUE_LENGTH)
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

        bool underline = tagType == INLINE_TAG_UNDERLINE;
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
            return AddInlineAction(cleanLabel, tagValue, string.Empty, underline, strike, fontScale);

        if (tagType == INLINE_TAG_MARKER)
            return AddInlineAction(cleanLabel, string.Empty, tagValue, underline, strike, fontScale);

        return AddInlineDecoration(cleanLabel, underline, strike, fontScale);
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

    protected bool AddInlineAction(string label, string actionId, string markerName, bool underline, bool strike, float fontScale)
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

        string markerName = m_aInlineMarkerNames[buttonIndex];
        if (!markerName.IsEmpty())
        {
            PrintFormat("GC | Briefing marker link requested (map focus is not implemented): %1", markerName);
            return;
        }

        ExecuteBriefingAction(m_aInlineActionIds[buttonIndex]);
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

        // A FrameWidgetSlot owns the instantiated view's final dimensions and
        // otherwise keeps the authored 720-pixel width despite the size override.
        if (FrameWidget.Cast(m_wViewSize.GetParent()))
            FrameSlot.SetSizeX(m_wViewSize, width);
    }
}
