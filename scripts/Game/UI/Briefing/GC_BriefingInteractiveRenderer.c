//! Renders custom inline briefing markup and owns its button subscriptions.
class GC_BriefingInteractiveRenderer
{
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
	protected static const string TAG_ATTRIBUTE_CLOSE = "\">";
	protected static const int MAX_TAG_VALUE_LENGTH = 64;
	protected static const int MAX_LINK_URL_LENGTH = 512;
	protected static const int MAX_IMAGE_RESOURCE_LENGTH = 256;
	protected static const int MAX_IMAGE_NAME_LENGTH = 128;
	protected static const float MIN_BLOCK_IMAGE_DIMENSION = 16;
	protected static const float MAX_BLOCK_IMAGE_DIMENSION = 2048;
	protected static const float DEFAULT_INLINE_IMAGE_DIMENSION = 24;
	protected static const float MAX_INLINE_IMAGE_DIMENSION = 96;
	protected static const ResourceName INLINE_TEXT_LAYOUT = "{6A4D000000000001}UI/layouts/Briefing/Lobby_Briefing_Inline_Text.layout";
	protected static const ResourceName INLINE_ACTION_LAYOUT = "{6A4D000000000002}UI/layouts/Briefing/Lobby_Briefing_Inline_Action.layout";
	protected static const ResourceName INLINE_DECORATION_LAYOUT = "{6A4D000000000003}UI/layouts/Briefing/Lobby_Briefing_Inline_Decoration.layout";
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

	protected Widget m_wActionMarkerFlow;
	protected RichTextWidget m_wActionMarkerBefore;
	protected Widget m_wActionInlineFlow;
	protected RichTextWidget m_wActionMarkerAfter;
	protected GC_LobbyManager m_LobbyManager;
	protected SizeLayoutWidget m_wViewSize;
	protected GC_BriefingMapFocusController m_MapFocusController;
	protected ref array<SCR_ButtonBaseComponent> m_aInlineButtons = {};
	protected ref array<string> m_aInlineActionIds = {};
	protected ref array<string> m_aInlineMarkerNames = {};
	protected ref array<string> m_aInlineLinkUrls = {};

	void Initialize(Widget markerFlow, RichTextWidget before, Widget inlineFlow, RichTextWidget after)
	{
		m_wActionMarkerFlow = markerFlow;
		m_wActionMarkerBefore = before;
		m_wActionInlineFlow = inlineFlow;
		m_wActionMarkerAfter = after;
	}

	bool Configure(string content, GC_LobbyManager lobbyManager, SizeLayoutWidget viewSize, GC_BriefingMapFocusController mapFocusController)
	{
		m_LobbyManager = lobbyManager;
		m_wViewSize = viewSize;
		m_MapFocusController = mapFocusController;
	    Clear();

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
	            Clear();
	            return false;
	        }

	        cursor = afterTag;
	    }

	    m_wActionMarkerFlow.SetVisible(true);

	    return true;
	}

	protected bool FindNextInlineTag(string content, int searchStart, out int tagType, out int tagStart)
	{
	    return GC_BriefingParser.FindNextInlineTag(content, searchStart, tagType, tagStart);
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

	        if (tagType == INLINE_TAG_LINK && !GC_BriefingActionHandler.IsValidExternalUrl(tagValue))
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

	protected bool GetBlockImageAttribute(string tag, string attribute, out string value)
	{
		return GC_BriefingParser.GetAttribute(tag, attribute, value);
	}

	protected bool ParseBlockImageDimension(string tag, string attribute, out float dimension)
	{
		return GC_BriefingParser.ParseOptionalDimension(tag, attribute,
			MIN_BLOCK_IMAGE_DIMENSION, MAX_BLOCK_IMAGE_DIMENSION, dimension);
	}

	protected bool ParseLabelFormatting(string label, out string cleanLabel, inout bool underline, inout bool strike, inout float fontScale)
	{
	    return GC_BriefingParser.ParseLabelFormatting(label, cleanLabel, underline, strike, fontScale);
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

	void Clear()
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

	void Detach()
	{
		Clear();
		m_wActionMarkerFlow = null;
		m_wActionMarkerBefore = null;
		m_wActionInlineFlow = null;
		m_wActionMarkerAfter = null;
		m_LobbyManager = null;
		m_wViewSize = null;
		m_MapFocusController = null;
	}

	protected void OnInlineActionClicked(SCR_ButtonBaseComponent button)
	{
	    int buttonIndex = m_aInlineButtons.Find(button);
	    if (buttonIndex < 0)
	        return;

	    string linkUrl = m_aInlineLinkUrls[buttonIndex];
	    if (!linkUrl.IsEmpty())
	    {
	        GC_BriefingActionHandler.OpenExternalLink(linkUrl);
	        return;
	    }

	    string markerName = m_aInlineMarkerNames[buttonIndex];
	    if (!markerName.IsEmpty())
	    {
	        m_MapFocusController.Focus(markerName, m_LobbyManager, m_wViewSize);
	        return;
	    }

	    GC_BriefingActionHandler.Execute(m_aInlineActionIds[buttonIndex]);
	}


}
