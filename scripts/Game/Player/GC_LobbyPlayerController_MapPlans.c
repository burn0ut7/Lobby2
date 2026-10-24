modded class GC_LobbyPlayerController
{
	ref ScriptInvoker<int, string, string> m_GC_OnMapPlanResult = new ScriptInvoker<int, string, string>();
	protected ref map<int, string> m_GC_mMapPlanFiles = new map<int, string>();

	//------------------------------------------------------------------------------------------------
	void GC_RequestMapPlan(string action, string input, int requestId)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return;
		if (action == "save")
		{
			m_GC_mMapPlanFiles.Insert(requestId, input);
			input = string.Empty;
		}
		Rpc(GC_RpcAsk_MapPlan, action, input, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_MapPlan(string action, string input, int requestId)
	{
		GC_MapPlans plans = GC_MapPlans.GetInstance();
		string output;
		string outcome = "Lobby2 is no longer active.";
		if (plans)
			outcome = plans.Execute(GetPlayerId(), action, input, output);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "map-plan", action, "rejected-inactive-mode");
		Rpc(GC_RpcDo_MapPlan, requestId, outcome, output);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_MapPlan(int requestId, string outcome, string output)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return;
		string path;
		if (m_GC_mMapPlanFiles.Find(requestId, path))
		{
			m_GC_mMapPlanFiles.Remove(requestId);
			if (!output.IsEmpty())
			{
				FileIO.MakeDirectory("$profile:Lobby2MapPlans");
				if (GC_MapPlanData.WriteFile(path, output))
					outcome = "Saved plan to " + path;
				else
					outcome = "Plan could not be saved on this computer.";
			}
		}
		m_GC_OnMapPlanResult.Invoke(requestId, outcome, output);
	}
}
