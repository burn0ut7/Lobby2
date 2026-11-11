[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Respawn Location Action")]
class GC_SetRespawnLocationAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Factions whose future default respawn destination changes. At least one faction is required. Does not move living players or respawn anyone.", enums: {ParamEnum("", "", "rst-choices:1:GC_FactionChoices")})]
	ref array<string> m_aFactions;
	[Attribute("", UIWidgets.Object, "Default destination for all replacement-character respawns, including individual heal-and-respawn and automatic/scripted waves. An explicit location choice overrides it. Empty restores each caller's normal original/current fallback. Named destinations follow their entities as they move. Later actions replace this setting.")]
	ref GC_RespawnLocation m_Location;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RESPAWN_LOCATION;
		action.m_RespawnLocation = m_Location;
		action.m_aFlagNames = {};
		if (m_aFactions)
			action.m_aFlagNames.Copy(m_aFactions);
		return action;
	}
}
