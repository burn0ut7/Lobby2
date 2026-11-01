#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected bool m_bGC_RestoreGameMasterBadgeRole;

	//------------------------------------------------------------------------------------------------
	//! Explicit fixture exercises native role events, never sets badge visibility.
	string GC_TestGameMasterBadge(string action)
	{
		PlayerController controller = GetGame().GetPlayerController();
		PlayerManager players = GetGame().GetPlayerManager();
		if (!controller || !players)
			return "UNAVAILABLE local player";
		int playerId = controller.GetPlayerId();
		if (action != "check")
		{
			if (!Replication.IsServer() || !SCR_Global.IsAdmin(playerId))
				return "REFUSED local server admin required";
			if (action == "clear" && players.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER))
				m_bGC_RestoreGameMasterBadgeRole = players.ClearPlayerRole(playerId, EPlayerRole.GAME_MASTER);
			else if (action == "restore" && m_bGC_RestoreGameMasterBadgeRole)
			{
				players.GivePlayerRole(playerId, EPlayerRole.GAME_MASTER);
				m_bGC_RestoreGameMasterBadgeRole = false;
			}
			else
				return "REFUSED unsupported action or fixture state";
		}
		string result = string.Format("admin=%1 gm=%2", SCR_Global.IsAdmin(playerId),
			players.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER));
		array<Widget> widgets = {GetGame().GetWorkspace()};
		for (int i; i < widgets.Count(); i++)
		{
			Widget widget = widgets[i];
			if (widget.GetName() == "GameMasterIcon")
				result += string.Format("; badge parent=%1 visible=%2", widget.GetParent().GetName(), widget.IsVisible());
			Widget child = widget.GetChildren();
			while (child)
			{
				widgets.Insert(child);
				child = child.GetSibling();
			}
		}
		if (action != "check")
			GC_AdminActionLogger.Log(playerId, "test-game-master-badge", action, result);
		return result;
	}
}
#endif
