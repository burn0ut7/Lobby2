modded class GC_LobbyPlayerController
{
	protected int m_iGC_TestHealthPackets;
	protected int m_iGC_TestFocusPackets;
	protected int m_iGC_TestWorldPackets;

	string GC_TestFocus(string action, int playerId)
	{
		if (action == "select")
			GC_SelectSpectatorPlayer(playerId);
		else if (action == "world")
			GC_ToggleWorldHealth();
		else if (action == "more")
			GC_LoadFocusedEventPage();
		else if (action == "unauthorized")
			Rpc(GC_RpcAsk_FocusSettings, m_iGC_FocusGeneration, playerId, true);
		string report = string.Format("state=%1 focus=%2 health=%3 world=%4 rows=%5 next=%6", GetLobbyPlayerState(), m_iGC_FocusedPlayer, GC_GetPlayerHealth(playerId), m_bGC_WorldHealth, m_aGC_FocusEvents.Count() / 7, m_iGC_FocusNext);
		report += string.Format(" healthPackets=%1 eventPackets=%2 serverFocus=%3 serverWorld=%4 serverHealth=%5", m_iGC_TestHealthPackets, m_iGC_TestFocusPackets, m_iGC_ServerFocus, m_bGC_ServerWorldHealth, m_mGC_ServerHealth.Count());
		report += string.Format(" worldPackets=%1 worldValues=%2 worldRequested=%3", m_iGC_TestWorldPackets, m_mGC_WorldHealth.Count(), m_aGC_RequestedHealth.Count());
		if (!m_aGC_FocusEvents.IsEmpty())
			report += " first=" + m_aGC_FocusEvents[2] + " " + m_aGC_FocusEvents[3];
		return report;
	}

	override protected void GC_RpcDo_WorldHealth(int epoch, array<RplId> ids, array<int> values)
	{
		m_iGC_TestWorldPackets++;
		super.GC_RpcDo_WorldHealth(epoch, ids, values);
	}

	override protected void GC_RpcDo_HealthChanges(int generation, array<int> changes)
	{
		m_iGC_TestHealthPackets++;
		super.GC_RpcDo_HealthChanges(generation, changes);
	}

	override protected void GC_RpcDo_FocusEvents(int generation, int before, array<string> rows, int next, string status)
	{
		m_iGC_TestFocusPackets++;
		super.GC_RpcDo_FocusEvents(generation, before, rows, next, status);
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestFocus(int requester, string action, int target)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requester));
		if (!controller)
			return "NO CONTROLLER";
		return controller.GC_TestFocus(action, target);
	}

	string GC_TestFocusRow(int target)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "NO MENU";
		Widget row = menu.GetRootWidget().FindAnyWidget("FactionPlayer_" + target.ToString());
		if (!row)
			return "NO ROW";
		GC_VoicePlayerRowComponent handler = GC_VoicePlayerRowComponent.Cast(row.FindHandler(GC_VoicePlayerRowComponent));
		if (!handler)
			return "NO HANDLER";
		return "handled=" + handler.OnClick(row, 0, 0, 0).ToString();
	}

	string GC_TestFocusBurst(int target, int count)
	{
		if (count < 1 || count > 50)
			return "INVALID COUNT";
		for (int i; i < count; i++)
		{
			GC_TestFocusHealth(target, 1);
			GC_TestLogDamage(target, 1);
		}
		return "DISPATCHED native damage count=" + count.ToString();
	}

	//! Native authoritative hit-zone mutation for health/healing display verification.
	string GC_TestFocusHealth(int target, float health)
	{
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "DENIED";
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(target);
		if (!entity || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return "NO CHARACTER";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "NO DAMAGE MANAGER";
		damage.SetHealthScaled(Math.Clamp(health, 0, 1));
		return "health=" + damage.GetHealthScaled().ToString();
	}
}
#endif
