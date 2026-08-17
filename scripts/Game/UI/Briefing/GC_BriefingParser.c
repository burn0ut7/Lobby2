//! Pure syntax inspection for Lobby2 briefing markup. It never creates widgets
//! or executes actions, which keeps malformed-content handling deterministic.
class GC_BriefingParser
{
	static const string ACTION_TAG_PREFIX = "<action id=\"";
	static const string UNDERLINE_TAG_CLOSE = "</u>";
	static const string STRIKE_TAG_CLOSE = "</s>";
	static const string STRIKE_LONG_TAG_CLOSE = "</strike>";
	static const string SIZE_TAG_CLOSE = "</size>";
	static const string TAG_ATTRIBUTE_CLOSE = "\">";
	static const string MARKER_TAG_PREFIX = "<marker name=\"";
	static const string LINK_TAG_PREFIX = "<link url=\"";
	static const string UNDERLINE_TAG_OPEN = "<u>";
	static const string STRIKE_TAG_OPEN = "<s>";
	static const string STRIKE_LONG_TAG_OPEN = "<strike>";
	static const string SIZE_TAG_PREFIX = "<size scale=\"";
	static const string IMAGE_TAG_PREFIX = "<img ";
	static const string IMAGE_TAG_CLOSE = "/>";
	static const string SCENARIO_TAG = "<scenario />";
	static const int INLINE_TAG_ACTION = 0;
	static const int INLINE_TAG_MARKER = 1;
	static const int INLINE_TAG_UNDERLINE = 2;
	static const int INLINE_TAG_STRIKE = 3;
	static const int INLINE_TAG_SIZE = 4;
	static const int INLINE_TAG_LINK = 5;
	static const int INLINE_TAG_IMAGE = 6;
	static const float MIN_INLINE_FONT_SCALE = 0.75;
	static const float MAX_INLINE_FONT_SCALE = 2.0;

	static bool HasSupportedRichTextTag(string content)
	{
		if (content.Contains("<b>") || content.Contains("<i>")) return true;
		if (content.Contains("<outline ") || content.Contains("<shadow ")) return true;
		if (content.Contains("<image ") || content.Contains("<color ") || content.Contains("<font ")) return true;
		if (content.Contains(IMAGE_TAG_PREFIX) || content.Contains(SCENARIO_TAG)) return true;
		if (content.Contains("<p>") || content.Contains("<p ")) return true;
		if (content.Contains("<h>") || content.Contains("<h ")) return true;
		if (content.Contains("<h1>") || content.Contains("<h1 ")) return true;
		if (content.Contains("<h2>") || content.Contains("<h2 ")) return true;
		if (content.Contains("<br") || content.Contains("<ucs ")) return true;
		if (content.Contains("<action name=") || content.Contains("<key ")) return true;
		if (content.Contains(ACTION_TAG_PREFIX) || content.Contains(MARKER_TAG_PREFIX)) return true;
		if (content.Contains(LINK_TAG_PREFIX) || content.Contains(UNDERLINE_TAG_OPEN)) return true;
		if (content.Contains(STRIKE_TAG_OPEN) || content.Contains(STRIKE_LONG_TAG_OPEN)) return true;
		return content.Contains(SIZE_TAG_PREFIX);
	}

	static bool HasInlineImageTag(string content)
	{
		int cursor;
		while (cursor < content.Length())
		{
			int tagStart = content.IndexOfFrom(cursor, IMAGE_TAG_PREFIX);
			if (tagStart < 0)
				return false;

			int tagEnd = content.IndexOfFrom(tagStart, IMAGE_TAG_CLOSE);
			if (tagEnd < 0)
				return false;

			int afterTag = tagEnd + IMAGE_TAG_CLOSE.Length();
			if (content.Substring(tagStart, afterTag - tagStart).Contains("inline=\"true\""))
				return true;

			cursor = afterTag;
		}

		return false;
	}

	static bool HasBlockImageTag(string content)
	{
		int cursor;
		while (cursor < content.Length())
		{
			int tagStart = content.IndexOfFrom(cursor, IMAGE_TAG_PREFIX);
			if (tagStart < 0)
				return false;

			int tagEnd = content.IndexOfFrom(tagStart, IMAGE_TAG_CLOSE);
			if (tagEnd < 0)
				return true;

			int afterTag = tagEnd + IMAGE_TAG_CLOSE.Length();
			if (!content.Substring(tagStart, afterTag - tagStart).Contains("inline=\"true\""))
				return true;

			cursor = afterTag;
		}

		return false;
	}

	static bool GetAttribute(string tag, string attribute, out string value)
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

	static bool ParseOptionalDimension(string tag, string attribute, float minimum, float maximum, out float dimension)
	{
		dimension = 0;
		if (!tag.Contains(attribute + "=\""))
			return true;

		string value;
		if (!GetAttribute(tag, attribute, value))
			return false;

		dimension = value.ToFloat(-1);
		return dimension >= minimum && dimension <= maximum;
	}

	static bool FindNextInlineTag(string content, int searchStart, out int tagType, out int tagStart)
	{
		tagStart = -1;
		tagType = -1;
		ConsiderTag(content.IndexOfFrom(searchStart, ACTION_TAG_PREFIX), INLINE_TAG_ACTION, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, MARKER_TAG_PREFIX), INLINE_TAG_MARKER, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, UNDERLINE_TAG_OPEN), INLINE_TAG_UNDERLINE, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, STRIKE_TAG_OPEN), INLINE_TAG_STRIKE, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, STRIKE_LONG_TAG_OPEN), INLINE_TAG_STRIKE, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, SIZE_TAG_PREFIX), INLINE_TAG_SIZE, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, LINK_TAG_PREFIX), INLINE_TAG_LINK, tagStart, tagType);
		ConsiderTag(content.IndexOfFrom(searchStart, IMAGE_TAG_PREFIX), INLINE_TAG_IMAGE, tagStart, tagType);
		return tagStart >= 0;
	}

	protected static void ConsiderTag(int candidate, int candidateType, inout int tagStart, inout int tagType)
	{
		if (candidate < 0 || (tagStart >= 0 && candidate >= tagStart))
			return;

		tagStart = candidate;
		tagType = candidateType;
	}

	static bool ParseLabelFormatting(string label, out string cleanLabel, inout bool underline, inout bool strike, inout float fontScale)
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

		if (cleanLabel.IsEmpty() || cleanLabel.Contains(ACTION_TAG_PREFIX) || cleanLabel.Contains(MARKER_TAG_PREFIX))
			return false;
		if (cleanLabel.Contains(LINK_TAG_PREFIX) || cleanLabel.Contains(UNDERLINE_TAG_OPEN))
			return false;
		if (cleanLabel.Contains(STRIKE_TAG_OPEN) || cleanLabel.Contains(STRIKE_LONG_TAG_OPEN) || cleanLabel.Contains("<size"))
			return false;
		return true;
	}
}
