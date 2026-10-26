[ComponentEditorProps(category: "Tests", description: "Opt-in mission script probe")]
class GC_MissionProbeComponentClass : GC_MissionScriptComponentClass
{
}

class GC_MissionProbeComponent : GC_MissionScriptComponent
{
	int m_iCalls;

	//------------------------------------------------------------------------------------------------
	override bool Run()
	{
		m_iCalls++;
		return true;
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionPresentation()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionRuntime runtime = GC_GetMissionRuntime();
		context.Bind(runtime);
		GC_RunCustomScriptAction script = new GC_RunCustomScriptAction();
		script.m_sEntity = "FrameworkTestScript";
		IEntity entity = GetGame().GetWorld().FindEntityByName(script.m_sEntity);
		if (!entity)
			return "UNAVAILABLE FrameworkTestScript fixture";
		GC_MissionProbeComponent probe = GC_MissionProbeComponent.Cast(entity.FindComponent(GC_MissionProbeComponent));
		int before = probe.m_iCalls;
		bool once = script.Execute(context) && probe.m_iCalls == before + 1;
		once = !script.Execute(context) && probe.m_iCalls == before + 1 && once;
		script.m_bAllowRepeat = true;
		bool repeat = script.Execute(context) && probe.m_iCalls == before + 2;
		script.m_bOnClients = true;
		bool localClient = script.Execute(context) && probe.m_iCalls == before + 3;
		GC_ShowMessageAction message = new GC_ShowMessageAction();
		message.m_sTitle = "Mission Framework";
		message.m_sText = "Objective captured. Reinforcements are on their way.";
		message.m_fSeconds = 30;
		bool hint = message.Execute(context);
		GC_ShowChatMessageAction chat = new GC_ShowChatMessageAction();
		chat.m_sText = "Mission Framework: objective captured.";
		bool chatDispatched = chat.Execute(context);
		return string.Format("once=%1 repeat=%2 localClient=%3 hintDispatched=%4 chatDispatched=%5", once, repeat, localClient, hint, chatDispatched);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionEnvironment()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		TimeAndWeatherManagerEntity weather = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager();
		if (!weather)
			return "UNAVAILABLE native time/weather manager";
		int hour, minute, second;
		weather.GetHoursMinutesSeconds(hour, minute, second);
		GC_SetTimeAction time = new GC_SetTimeAction();
		time.m_iHour = 15;
		time.m_iMinute = 30;
		bool applied = time.Execute(context);
		int actualHour, actualMinute, actualSecond;
		weather.GetHoursMinutesSeconds(actualHour, actualMinute, actualSecond);
		bool matched = applied && actualHour == 15 && actualMinute == 30;
		time.m_iHour = hour;
		time.m_iMinute = minute;
		time.m_iSecond = second;
		bool restored = time.Execute(context);
		GC_SetWeatherAction change = new GC_SetWeatherAction();
		change.m_sPreset = weather.GetCurrentWeatherState().GetStateName();
		change.m_bAllowRandomChanges = true;
		string error;
		bool preset = change.Validate(context, error) && change.Execute(context);
		return string.Format("time=%1 restored=%2 weatherDispatched=%3 preset=%4", matched, restored, preset, change.m_sPreset);
	}
}
#endif
