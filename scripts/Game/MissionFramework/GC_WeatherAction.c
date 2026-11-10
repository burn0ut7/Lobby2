[BaseContainerProps(), BaseContainerCustomStringTitleField("Weather Action")]
class GC_WeatherAction : GC_ActionBase
{
	[Attribute("Clear", UIWidgets.EditBox, "Exact weather preset name from this map's weather configuration, for example Clear, Cloudy or Overcast. Unknown names do not change the weather. This changes weather only, not the time of day.")]
	string m_sWeatherPreset;
	[Attribute("0", UIWidgets.SpinBox, "How many seconds of the game's weather timeline the change takes. Zero changes immediately. Time acceleration can affect the real time taken. This is the transition duration, not a delay before the action.", "0 86400 1")]
	float m_fTransitionSeconds;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: keep this weather until another system changes it. Checked: allow the map's normal weather progression after reaching this preset. This does not choose a random preset when the action runs.")]
	bool m_bAllowNaturalChanges;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.WEATHER;
		action.m_sTextOrName = m_sWeatherPreset;
		action.m_sTextOrName.TrimInPlace();
		action.m_fTransitionSeconds = m_fTransitionSeconds;
		action.m_bFlagAnswer = m_bAllowNaturalChanges;
		return action;
	}
}
