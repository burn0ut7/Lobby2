#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Check time and weather action", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_TimeWeatherActionChecks : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		Resource resource = BaseContainerTools.CreateContainer("GC_WeatherAction");
		if (!resource || !resource.IsValid())
		{
			Print("GC_TIME_WEATHER_CHECK FAIL cannot create action", LogLevel.ERROR);
			return;
		}
		GC_WeatherAction definition = GC_WeatherAction.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		GC_RuleAction action = definition.BuildAction();
		Check(action.IsValid() && action.m_bChangeWeather && !action.m_bChangeTime && action.m_sTextOrName == "Clear", "existing weather defaults preserved");
		definition.m_iHours = 99;
		Check(definition.BuildAction().IsValid(), "weather only ignores unused clock");
		definition.m_bChangeWeather = false;
		definition.m_bChangeTime = true;
		definition.m_iHours = 23;
		definition.m_iMinutes = 59;
		definition.m_iSeconds = 59;
		definition.m_sWeatherPreset = "";
		definition.m_fTransitionSeconds = -1;
		Check(definition.BuildAction().IsValid(), "time only ignores unused weather settings");
		definition.m_iHours = 24;
		Check(!definition.BuildAction().IsValid(), "invalid clock rejected");
		definition.m_iHours = 0;
		definition.m_bChangeWeather = true;
		Check(!definition.BuildAction().IsValid(), "combined action rejects invalid weather settings");
		definition.m_sWeatherPreset = " Clear ";
		definition.m_fTransitionSeconds = 120;
		action = definition.BuildAction();
		Check(action.IsValid() && action.m_bChangeTime && action.m_bChangeWeather && action.m_sTextOrName == "Clear" && action.m_fTransitionSeconds == 120, "combined changes compile and trim preset name");
		definition.m_bChangeTime = false;
		definition.m_bChangeWeather = false;
		Check(!definition.BuildAction().IsValid(), "empty action rejected");
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		array<string> names = GC_WeatherAction.GetPresetNames(editor.GetApi().GetWorld());
		Check(!names.IsEmpty(), "loaded map exposes weather presets");
		Print("GC_TIME_WEATHER_CHECK complete: 8 checks; no world changes.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string description)
	{
		if (passed)
			Print("GC_TIME_WEATHER_CHECK PASS " + description);
		else
			Print("GC_TIME_WEATHER_CHECK FAIL " + description, LogLevel.ERROR);
	}
}
#endif
