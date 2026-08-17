//! Executes validated non-rendering actions emitted by briefing interaction tags.
class GC_BriefingActionHandler
{
	protected static const int MAX_LINK_URL_LENGTH = 512;
	protected static const string ACTION_DEBUG_PRINT = "debug_print";

	static bool IsValidExternalUrl(string url)
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

	static void OpenExternalLink(string url)
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

	static void Execute(string actionId)
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
}
