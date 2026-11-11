[BaseContainerProps(), BaseContainerCustomStringTitleField("Map Marker Visibility Action")]
class GC_MapMarkerVisibilityAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Exact names of entities with a GC Briefing Marker component. Changes the existing marker and its text without deleting them. Missing markers are skipped. Player-drawn markers, map shapes and legacy mission-description entities are not targets.", enums: {ParamEnum("", "", "rst-choices:2:GC_MarkerEntityChoices")})]
	ref array<string> m_aMarkers;
	[Attribute("", UIWidgets.Object, "Add each faction to change and check Visible to show it, or uncheck to hide it. Unlisted factions keep their current visibility. The marker's game-mode phase requirement still applies.")]
	ref array<ref GC_MarkerFactionVisibility> m_aFactions;
	[Attribute("0", UIWidgets.CheckBox, "Checked means visible (true) to players with no assigned faction; unchecked means hidden (false). This is separate from faction entries and does not reveal markers to faction-assigned spectators.")]
	bool m_bVisibleWithoutFaction;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.MARKER_VISIBILITY;
		action.m_aFlagNames = {};
		if (m_aMarkers)
			action.m_aFlagNames.Copy(m_aMarkers);
		if (!action.m_aFlagNames.IsEmpty())
			action.m_sTextOrName = action.m_aFlagNames[0];
		action.m_bFlagAnswer = m_bVisibleWithoutFaction;
		action.m_aMarkerFactions = {};
		if (m_aFactions)
		{
			foreach (GC_MarkerFactionVisibility source : m_aFactions)
			{
				GC_MarkerFactionVisibility copy;
				if (source)
				{
					copy = new GC_MarkerFactionVisibility();
					copy.m_sFaction = source.m_sFaction;
					copy.m_bVisible = source.m_bVisible;
				}
				action.m_aMarkerFactions.Insert(copy);
			}
		}
		return action;
	}
}
