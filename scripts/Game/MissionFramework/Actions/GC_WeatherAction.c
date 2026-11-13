//! Keep the serialized class and weather field names so existing missions retain their settings.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Time and Weather Action")]
class GC_WeatherAction : GC_ActionBase
{
	[Attribute("1", UIWidgets.CheckBox, "Change the weather when this action runs. Uncheck to leave weather alone and change only the time.")]
	bool m_bChangeWeather;
	[Attribute("Clear", UIWidgets.EditComboBox, "Choose or type a weather preset name from this map. Only used when Change Weather is checked. An unavailable preset rejects the action before changing weather or time.", enums: {ParamEnum("", "", "gc-choices:1:GC_WeatherChoices")})]
	string m_sWeatherPreset;
	[Attribute("0", UIWidgets.EditBox, "Weather transition duration in seconds of the game's weather timeline. Zero changes immediately. Time acceleration can affect real time taken. Only used when Change Weather is checked; it does not delay the time change.", "0 86400")]
	float m_fTransitionSeconds;
	[Attribute("0", UIWidgets.CheckBox, "Only used when Change Weather is checked. Unchecked: keep this weather until another system changes it. Checked: allow the map's normal weather progression after reaching this preset.")]
	bool m_bAllowNaturalChanges;
	[Attribute("0", UIWidgets.CheckBox, "Set the clock when this action runs. Uncheck to leave the current time alone. This does not change the date or the mode's normal time progression.")]
	bool m_bChangeTime;
	[Attribute("8", UIWidgets.EditBox, "Hour on a 24-hour clock, from 0 to 23. For example, 18 means 6 PM. Only used when Change Time is checked.", "0 23")]
	int m_iHours;
	[Attribute("0", UIWidgets.EditBox, "Minutes from 0 to 59. Only used when Change Time is checked.", "0 59")]
	int m_iMinutes;
	[Attribute("0", UIWidgets.EditBox, "Seconds from 0 to 59. Only used when Change Time is checked.", "0 59")]
	int m_iSeconds;

	//------------------------------------------------------------------------------------------------
	//! Read the current map each time; never persist list indices or a cache from another world.
	static array<string> GetPresetNames(BaseWorld world)
	{
		array<string> names = {};
		ChimeraWorld chimeraWorld = ChimeraWorld.CastFrom(world);
		if (!chimeraWorld)
			return names;
		TimeAndWeatherManagerEntity manager = chimeraWorld.GetTimeAndWeatherManager();
		if (!manager)
			return names;
		array<ref WeatherState> states = {};
		manager.GetWeatherStatesList(states);
		foreach (WeatherState state : states)
		{
			if (state && !state.GetStateName().IsEmpty() && !names.Contains(state.GetStateName()))
				names.Insert(state.GetStateName());
		}
		return names;
	}

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.WEATHER;
		action.m_bChangeWeather = m_bChangeWeather;
		action.m_bChangeTime = m_bChangeTime;
		action.m_iHours = m_iHours;
		action.m_iMinutes = m_iMinutes;
		action.m_iSeconds = m_iSeconds;
		action.m_sTextOrName = m_sWeatherPreset;
		action.m_sTextOrName.TrimInPlace();
		action.m_fTransitionSeconds = m_fTransitionSeconds;
		action.m_bFlagAnswer = m_bAllowNaturalChanges;
		return action;
	}
}
