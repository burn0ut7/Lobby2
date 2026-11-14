[BaseContainerProps(), BaseContainerCustomTitleField("m_sFaction")]
class GC_MarkerFactionVisibility
{
	[Attribute("", UIWidgets.Auto, "Exact faction key, such as US or USSR. Only this faction's visibility changes. Other factions keep their current setting.")]
	FactionKey m_sFaction;
	[Attribute("1", UIWidgets.CheckBox, "Checked means visible (true); unchecked means hidden (false) for this faction. The marker's game-mode phase requirement still applies.")]
	bool m_bVisible;
}
