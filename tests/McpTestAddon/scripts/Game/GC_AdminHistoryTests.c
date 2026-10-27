//! Explicit pagination/retention contract test; restore live storage after exercising the production recorder.
#ifdef WORKBENCH
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminHistoryRollover()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE authority/admin";
		ref array<ref GC_SessionEvent> recent = m_aRecent;
		ref array<ref GC_SessionEvent> queued = m_aQueued;
		int sequence = m_iSequence;
		int cursor = m_iRingCursor;
		m_aRecent = {};
		m_aQueued = {};
		m_iSequence = 0;
		m_iRingCursor = 0;
		for (int index; index < MAX_RECENT + 2; index++)
		{
			GC_SessionEvent entry = CreateEvent("damage", controller.GetPlayerId(), controller.GetPlayerId());
			entry.m_sActorName = "DamageTest";
			entry.m_fDamage = index;
			Record(entry);
		}
		array<string> rows;
		int next;
		string status;
		ReadPage(controller.GetPlayerId(), "DamageTest", "damage", 0, rows, next, status);
		string result = "PASS individual damage, newest-first paging, retention and denied access";
		if (rows.Count() != 100 || rows[0].ToInt() != MAX_RECENT + 2 || next != MAX_RECENT - 17)
			result = "FAIL newest page";
		ReadPage(controller.GetPlayerId(), "DamageTest", "damage", next, rows, next, status);
		if (rows.Count() != 100 || rows[0].ToInt() != MAX_RECENT - 18)
			result = "FAIL older page";
		if (m_aRecent.Count() != MAX_RECENT || m_aQueued.Count() != MAX_RECENT + 2)
			result = "FAIL retention or individual damage persistence";
		ReadPage(-1, "DamageTest", "damage", 0, rows, next, status);
		if (!rows.IsEmpty() || status != "Access denied.")
			result = "FAIL authorization";
		m_aRecent = recent;
		m_aQueued = queued;
		m_iSequence = sequence;
		m_iRingCursor = cursor;
		return result;
	}
}
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminHistoryRollover()
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!logs)
			return "UNAVAILABLE session log";
		return logs.GC_TestAdminHistoryRollover();
	}
}
#endif