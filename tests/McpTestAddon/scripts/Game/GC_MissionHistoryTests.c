#ifdef WORKBENCH
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestHistorySnapshot()
	{
		if (!m_History)
			return "UNAVAILABLE history";
		return m_History.Encode();
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestHistoryEnd(string factionKey, int reason)
	{
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "UNAVAILABLE authority/admin";
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		int factionIndex = -1;
		if (faction)
			factionIndex = GetGame().GetFactionManager().GetFactionIndex(faction);
		ref SCR_GameModeEndData data = SCR_GameModeEndData.CreateSimple(reason, -1, factionIndex);
		bool ended = EnterPostGame(data);
		return string.Format("production end result=%1", ended);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPerformanceSummary()
	{
		GC_MissionPerformance samples = new GC_MissionPerformance();
		samples.Sample(0, 60, 3, 10);
		samples.Sample(10, 10, 5, 8);
		samples.Sample(25, 20, 2, 20);
		JsonSaveContext context = new JsonSaveContext();
		samples.Write(context);
		return context.SaveToString();
	}
}
#endif
