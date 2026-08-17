//! Owns briefing block/scenario widget creation, sizing, and cleanup.
class GC_BriefingRenderer
{
	protected static const int MAX_BLOCK_IMAGES = 16;
	protected static const int MAX_IMAGE_RESOURCE_LENGTH = 256;
	protected static const int MAX_IMAGE_NAME_LENGTH = 128;
	protected static const float MIN_BLOCK_IMAGE_DIMENSION = 16;
	protected static const float MAX_BLOCK_IMAGE_DIMENSION = 2048;
	protected static const float BLOCK_IMAGE_VERTICAL_PADDING = 16;
	protected static const string BLOCK_IMAGE_TAG_PREFIX = "<img ";
	protected static const string BLOCK_IMAGE_TAG_CLOSE = "/>";
	protected static const ResourceName BLOCK_TEXT_LAYOUT = "{6A4D000000000004}UI/layouts/Briefing/Lobby_Briefing_Block_Text.layout";
	protected static const ResourceName BLOCK_IMAGE_LAYOUT = "{6A4D000000000005}UI/layouts/Briefing/Lobby_Briefing_Block_Image.layout";

	protected Widget m_wBlockImageFlow;
	protected ScrollLayoutWidget m_wSectionScroll;
	protected GC_BriefingScenarioComponent m_ScenarioComponent;
	protected ref array<RichTextWidget> m_aBlockTextWidgets = {};
	protected ref array<SizeLayoutWidget> m_aBlockImageSizes = {};
	protected ref array<float> m_aBlockImageNativeWidths = {};
	protected ref array<float> m_aBlockImageNativeHeights = {};
	protected ref array<float> m_aBlockImageRequestedWidths = {};
	protected ref array<float> m_aBlockImageRequestedHeights = {};
	protected ref array<float> m_aBlockImageDisplayHeights = {};

	void Initialize(Widget blockFlow, ScrollLayoutWidget sectionScroll, GC_BriefingScenarioComponent scenarioComponent)
	{
		m_wBlockImageFlow = blockFlow;
		m_wSectionScroll = sectionScroll;
		m_ScenarioComponent = scenarioComponent;
	}

	bool ConfigureScenario()
	{
		if (!m_wBlockImageFlow || !m_ScenarioComponent || !m_ScenarioComponent.Show())
			return false;

		m_wBlockImageFlow.SetVisible(true);
		return true;
	}

	bool ConfigureBlocks(string content)
	{
	    Clear();
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
	            Clear();
	            return false;
	        }

	        int afterTag = tagEnd + BLOCK_IMAGE_TAG_CLOSE.Length();
	        string tag = content.Substring(tagStart, afterTag - tagStart);
	        if (!AddBlockImage(tag))
	        {
	            Clear();
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
	    return GC_BriefingParser.HasInlineImageTag(content);
	}

	protected bool HasBlockImageTag(string content)
	{
	    return GC_BriefingParser.HasBlockImageTag(content);
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
	    return GC_BriefingParser.GetAttribute(tag, attribute, value);
	}

	//! Missing width or height is valid and represented as zero. If both are
	//! present they form a maximum box rather than distorting the image.
	protected bool ParseBlockImageDimension(string tag, string attribute, out float dimension)
	{
	    return GC_BriefingParser.ParseOptionalDimension(tag, attribute,
	        MIN_BLOCK_IMAGE_DIMENSION, MAX_BLOCK_IMAGE_DIMENSION, dimension);
	}

	void Resize()
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

	void Clear()
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

	void Detach()
	{
		Clear();
		m_wBlockImageFlow = null;
		m_wSectionScroll = null;
		m_ScenarioComponent = null;
	}



	float GetContentHeight()
	{
		float contentHeight;
		if (m_ScenarioComponent)
			contentHeight += m_ScenarioComponent.GetContentHeight();
		foreach (RichTextWidget textBlock : m_aBlockTextWidgets)
		{
			if (textBlock)
				contentHeight += textBlock.GetContentHeight();
		}
		foreach (float imageHeight : m_aBlockImageDisplayHeights)
			contentHeight += imageHeight + BLOCK_IMAGE_VERTICAL_PADDING;
		return contentHeight;
	}
}
