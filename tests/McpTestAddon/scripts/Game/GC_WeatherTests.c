//! Explicit readback for the stock weather handler; nothing runs on load.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestWeatherSnapshot()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return "No Chimera world";

		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		if (!weather)
			return "No weather manager";

		bool handlerPresent = GetOwner().FindComponent(SCR_TimeAndWeatherHandlerComponent) != null;
		return string.Format("server=%1 handler=%2 fogOverride=%3 fog=%4 heightOverride=%5 height=%6", Replication.IsServer(), handlerPresent, weather.IsFogAmountOverridden(), weather.GetFogAmount(), weather.IsFogHeightDensityOverridden(), weather.GetFogHeightDensity());
	}
}
#endif
