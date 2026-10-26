#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref GC_IsolationDismountProbe m_GC_MissionDismountProbe;
	protected ref GC_MissionRuntime m_GC_AITestRuntime;
	protected ref GC_LobbyMissionContext m_GC_AITestContext;
	protected bool m_bGC_PriorMovementRestriction;

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionAI(string step)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority required";
		if (step == "start")
		{
			if (m_GC_MissionDismountProbe)
				return "UNAVAILABLE fixture already exists";
			m_GC_AITestRuntime = new GC_MissionRuntime();
			m_GC_AITestContext = new GC_LobbyMissionContext(this);
			GC_MissionFlag flag = new GC_MissionFlag();
			flag.m_sName = "AI test";
			array<ref GC_MissionFlag> flags = {flag};
			array<ref GC_MissionEvent> events = {};
			bool started = m_GC_AITestRuntime.Start(m_GC_AITestContext, events, flags);
			m_GC_MissionDismountProbe = new GC_IsolationDismountProbe();
			bool spawned = m_GC_MissionDismountProbe.Spawn(GetOwner().GetOrigin() + "0 0 10");
			return string.Format("started=%1 spawned=%2", started, spawned);
		}
		if (!m_GC_MissionDismountProbe || !m_GC_MissionDismountProbe.m_Character)
			return "UNAVAILABLE start first";
		IEntity character = m_GC_MissionDismountProbe.m_Character;
		if (step == "mark")
		{
			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(character.FindComponent(SCR_AICombatComponent));
			combat.GC_KeepMissionTurretSeated(m_GC_AITestContext);
		}
		if (step == "hold")
		{
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
			m_bGC_PriorMovementRestriction = controller.GetDisableMovementControls();
			bool applied = m_GC_AITestRuntime.SetAIHoldPosition(character, true);

			bool idle = controller.GetDisableMovementControls();
			return string.Format("applied=%1 idle=%2", applied, idle);
		}
		if (step == "stop")
		{
			m_GC_AITestRuntime.Stop();
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
			bool restored = controller.GetDisableMovementControls() == m_bGC_PriorMovementRestriction;
			return string.Format("settingRestored=%1 ", restored) + m_GC_MissionDismountProbe.Snapshot();
		}
		return m_GC_MissionDismountProbe.Snapshot();
	}
}
#endif
