//! Read-only roster snapshot; available to every player without changing faction.
class GC_FactionInfoAction : GC_ContextMenuAction
{
	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "Get info";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		return context && context.GetTargetType() == GC_EContextTargetType.FACTION;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		return IsVisible(context) && manager && manager.GetRegisteredFaction(context.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		Faction selectedFaction = manager.GetRegisteredFaction(context.GetFactionKey());
		array<string> keys;
		array<int> targets;
		array<int> players;
		manager.GetFactionBalanceTargets(keys, targets, players);

		string message = string.Format("Total slots: %1\nConnected players: %2\n\nCurrent / needed players",
			manager.GetFactionSlotCount(context.GetFactionKey()), players.Count());
		foreach (Faction faction : manager.GetRegisteredFactions())
		{
			int currentPlayers;
			foreach (int playerId : players)
			{
				if (manager.GetPlayerFaction(playerId) == faction)
					currentPlayers++;
			}

			int neededPlayers;
			int index = keys.Find(faction.GetFactionKey());
			if (index >= 0)
				neededPlayers = targets[index];
			message += string.Format("\n%1: %2 / %3", WidgetManager.Translate(faction.GetFactionName()), currentPlayers, neededPlayers);
		}
		message += "\n\nTargets use total slots, including locked slots.";

		SCR_HintManagerComponent.ShowCustomHint(message,
			WidgetManager.Translate(selectedFaction.GetFactionName()), 15);
	}
}
