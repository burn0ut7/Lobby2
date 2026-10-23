//! Display-only wrapping shared by admin chat and popup text. Stored messages stay intact.
class GC_AdminTextWrapping
{
	//------------------------------------------------------------------------------------------------
	static void Apply(TextWidget body, Widget viewport, string original)
	{
		if (!body || !viewport)
			return;
		float width;
		float height;
		viewport.GetScreenSize(width, height);
		float textWidth;
		float textHeight;
		body.GetScreenSize(textWidth, textHeight);
		// The scroll viewport includes its scrollbar; the text child excludes it.
		float available = GetGame().GetWorkspace().DPIUnscale(Math.Min(width, textWidth)) - 4;
		if (available <= 0)
			return;

		// Native splitting preserves empty entries and avoids scanning every byte in script.
		array<string> lines = {};
		original.Split("\n", lines, false);
		string display;
		body.SetTextWrapping(false);
		foreach (int lineIndex, string line : lines)
		{
			if (lineIndex > 0)
				display += "\n";
			display += WrapLine(body, line, available);
		}
		body.SetTextWrapping(true);
		body.SetText(display);
	}

	//------------------------------------------------------------------------------------------------
	protected static string WrapLine(TextWidget body, string original, float available)
	{
		array<string> separators = {" ", "\t", "\r"};
		foreach (string separator : separators)
		{
			if (!original.Contains(separator))
				continue;
			array<string> parts = {};
			original.Split(separator, parts, false);
			string visibleSeparator = separator;
			// Keep a visible word boundary for tabs, expanding only the display copy.
			if (separator == "\t")
				visibleSeparator = "    ";
			string display;
			foreach (int partIndex, string part : parts)
			{
				if (partIndex > 0)
					display += visibleSeparator;
				// Each level removes one delimiter kind; recursion is bounded to three levels.
				display += WrapLine(body, part, available);
			}
			return display;
		}
		return WrapWord(body, original, available);
	}

	//------------------------------------------------------------------------------------------------
	protected static string WrapWord(TextWidget body, string word, float available)
	{
		if (word.IsEmpty())
			return word;
		float width;
		float height;
		body.SetText(word);
		// GetTextSize measures current text without laying out the entire chat hierarchy.
		body.GetTextSize(width, height);
		if (width <= available)
			return word;

		int length = word.Length();
		string display;
		int first = 0;
		while (first < length)
		{
			// Measure whole prefixes, including font kerning, at valid character boundaries.
			int low = first + 1;
			while (low < length && (word.ToAscii(low) & 192) == 128)
				low++;
			int high = length;
			int fit = low;
			while (low <= high)
			{
				int middle = low + (high - low) / 2;
				while (middle > low && middle < length && (word.ToAscii(middle) & 192) == 128)
					middle--;
				body.SetText(word.Substring(first, middle - first));
				body.GetTextSize(width, height);
				if (width <= available)
				{
					fit = middle;
					low = middle + 1;
					while (low < length && (word.ToAscii(low) & 192) == 128)
						low++;
				}
				else
					high = middle - 1;
			}
			if (first > 0)
				display += "\n";
			display += word.Substring(first, fit - first);
			first = fit;
		}
		return display;
	}
}
