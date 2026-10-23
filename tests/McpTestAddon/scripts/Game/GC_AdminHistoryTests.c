//! Explicit production-recorder boundary test with isolated, restored history storage.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminHistoryRollover()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE authority/admin";

		ref array<string> original = m_aAdminSessionHistory;
		ref map<int, string> originalLabels = m_mAdminSessionLabels;
		m_mAdminSessionLabels = new map<int, string>();
		m_aAdminSessionHistory = new array<string>();
		for (int eventNumber = 1; eventNumber <= 258; eventNumber++)
			RecordAdminSessionEvent(100000 + eventNumber, false, eventNumber);

		string result = "PASS latest 256 retained, newest 64 reported, filter and denial correct";
		if (m_aAdminSessionHistory.Count() != 256)
			result = "FAIL capacity";
		for (int index = 0; index < m_aAdminSessionHistory.Count(); index++)
		{
			int expected = index + 3;
			string suffix = string.Format(" / left / session %1 / cause %2", 100000 + expected, expected);
			if (!m_aAdminSessionHistory[index].EndsWith(suffix))
				result = string.Format("FAIL retention index=%1 expected=%2 entry=%3", index, expected, m_aAdminSessionHistory[index]);
		}

		string report = GetAdminSessionReport(controller.GetPlayerId(), "");
		string newest;
		for (int index = 255; index >= 192 && index < m_aAdminSessionHistory.Count(); index--)
			newest += m_aAdminSessionHistory[index] + "\n";
		if (newest.IsEmpty() || !report.Contains(newest) || report.Contains("session 100194 /"))
			result = "FAIL newest-first report or 64 bound";
		string filtered = GetAdminSessionReport(controller.GetPlayerId(), "session 100258");
		if (!filtered.Contains("session 100258 / cause 258") || filtered.Contains("session 100257 /"))
			result = "FAIL filtering";
		string denied = GetAdminSessionReport(-1, "session 100258");
		if (denied.Contains("session 100258") || denied != "A current admin login is required.")
			result = "FAIL unauthorized report";

		m_aAdminSessionHistory = original;
		m_mAdminSessionLabels = originalLabels;
		return result;
	}
}
#endif
