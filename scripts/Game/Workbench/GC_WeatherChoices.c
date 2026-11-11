#ifdef WORKBENCH
class GC_WeatherChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		if (!api)
			return {};
		return GC_WeatherAction.GetPresetNames(api.GetWorld());
	}
}
#endif
