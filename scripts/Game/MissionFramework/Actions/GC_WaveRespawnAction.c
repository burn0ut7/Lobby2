[BaseContainerProps(), BaseContainerCustomStringTitleField("Respawn Wave Action")]
class GC_WaveRespawnAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Faction keys whose dead players may respawn. Empty means all factions. Only connected players with a claimed KIA slot qualify, including those waiting in spectator after death. Unassigned spectators, AI and living players are excluded.", enums: {ParamEnum("", "", "rst-choices:1:GC_FactionChoices")})]
	ref array<string> m_aFactions;
	[Attribute("", UIWidgets.Object, "Optional destination for this wave only. Empty uses each faction's configured default, or original slot positions if none is set. Choose Original Slot Position to explicitly ignore the faction default. Named entities follow moving locations; World Position uses fixed coordinates.")]
	ref GC_RespawnLocation m_Location;
	[Attribute("0", UIWidgets.EditBox, "Maximum players to respawn in this wave, across all selected factions combined. Zero means everyone eligible. Only successful respawns count toward the limit.", "0 1000000")]
	int m_iMaximumPlayers;
	[Attribute("1", UIWidgets.CheckBox, "Checked: each successful respawn costs one ticket from that player's faction. Players without a ticket wait for a later wave. Failed respawns refund the ticket. Unchecked: respawn without requiring or spending tickets.")]
	bool m_bUseTickets;
	[Attribute("0", UIWidgets.CheckBox, "Checked: choose eligible players in random order, without choosing anyone twice. Unchecked: use the lobby slot order. Useful when the player limit or available tickets cannot cover everyone. One action runs one wave; use event conditions and timers to schedule more waves.")]
	bool m_bRandomOrder;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RESPAWN_WAVE;
		action.m_RespawnLocation = m_Location;
		action.m_iAmount = m_iMaximumPlayers;
		action.m_bFlagAnswer = m_bUseTickets;
		action.m_bRandomOrder = m_bRandomOrder;
		action.m_aFlagNames = {};
		if (m_aFactions)
			action.m_aFlagNames.Copy(m_aFactions);
		return action;
	}
}
