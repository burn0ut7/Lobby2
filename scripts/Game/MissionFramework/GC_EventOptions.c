[BaseContainerProps(), BaseContainerCustomStringTitleField("Advanced")]
class GC_EventOptions
{
	[Attribute("0", UIWidgets.CheckBox, "Checked: run again on later matching notifications. Unchecked: run once. This does not run continuously while conditions are true.")]
	bool m_bRepeatable;
	[Attribute("0", UIWidgets.SpinBox, "Delay in seconds before running. For example, 5 means wait 5 seconds. Zero means no delay. Conditions must still pass when the delay ends.", "0 86400 0.1")]
	float m_fDelay;
	[Attribute("1", UIWidgets.Slider, "Chance to run this event's actions, from 0 to 1. For example, 0.8 is 80%, 1 is 100%, and 0 never runs. The default is 1; the default setting means 100%. This does not spawn anything unless the actions do so.", "0 1 0.0001")]
	float m_fRandomChance;
	[Attribute("0", UIWidgets.CheckBox, "Keep this event in the list without running it.")]
	bool m_bDisabled;
}
