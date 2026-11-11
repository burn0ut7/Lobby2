//! Private machine credentials shared by voting and authenticated admin catalogue requests.
class GC_MissionServerConnection
{
	static RestContext Open(out string status)
	{
		status = "#GC-Vote_NotConfigured";
		if (!Replication.IsServer())
			return null;
		FileHandle file = FileIO.OpenFile("$profile:Lobby2Voting.json", FileMode.READ);
		if (!file)
			return null;
		string payload;
		file.Read(payload, 8192);
		file.Close();
		JsonLoadContext json = new JsonLoadContext(false);
		string url, key;
		if (!json.LoadFromString(payload) || !json.ReadValue("url", url) || !json.ReadValue("apiKey", key)
			|| key.Length() < 32 || key.Contains(",") || key.Contains("\n") || key.Contains("\r")
			|| url.Contains("@") || url.Contains("?") || url.Contains("#") || url.Contains("\n") || url.Contains("\r")
			|| (!url.StartsWith("https://") && !url.StartsWith("http://127.0.0.1:")))
		{
			status = "#GC-Vote_InvalidConfig";
			return null;
		}
		if (!url.EndsWith("/"))
			url += "/";
		RestContext context = GetGame().GetRestApi().GetContext(url);
		context.SetHeaders("Authorization,Bearer " + key + ",Content-Type,application/json");
		context.SetTimeout(15);
		return context;
	}
}
