#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref GC_LobbyMissionContext m_GC_MovementContext;
	protected ref GC_MissionRuntime m_GC_MovementRuntime;
	protected IEntity m_GC_MovementCharacter;
	protected AIGroup m_GC_MovementGroup;
	protected AIWaypoint m_GC_MovementWaypoint;
	protected vector m_vGC_MovementOrigin;

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionMovement(string step)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return "UNAVAILABLE active authority";
		if (step == "start")
		{
			if (m_GC_MovementContext)
				return "UNAVAILABLE already started";
			m_GC_MovementContext = new GC_LobbyMissionContext(this);
			m_GC_MovementRuntime = new GC_MissionRuntime();
			GC_MissionFlag flag = new GC_MissionFlag();
			flag.m_sName = "Movement test";
			array<ref GC_MissionEvent> events = {};
			array<ref GC_MissionFlag> flags = {flag};
			m_GC_MovementRuntime.Start(m_GC_MovementContext, events, flags);
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			vector position = GetOwner().GetOrigin() + "5 0 5";
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
			params.Transform[3] = position;
			m_GC_MovementGroup = AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{F72EF3429D8C8DF5}Prefabs/Groups/BLUFOR/Group_US_AmmoTeam.et"), GetGame().GetWorld(), params));
			position += "500 0 0";
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
			params.Transform[3] = position;
			m_GC_MovementWaypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et"), GetGame().GetWorld(), params));
			return string.Format("group=%1 waypoint=%2; allow native member spawning", m_GC_MovementGroup != null, m_GC_MovementWaypoint != null);
		}
		if (!m_GC_MovementGroup || !m_GC_MovementWaypoint)
			return "UNAVAILABLE fixture";
		if (!m_GC_MovementCharacter)
		{
			array<AIAgent> agents = {};
			m_GC_MovementGroup.GetAgents(agents);
			if (agents.IsEmpty())
				return "UNAVAILABLE native group has no members";
			m_GC_MovementCharacter = agents[0].GetControlledEntity();
		}
		AIControlComponent control = AIControlComponent.Cast(m_GC_MovementCharacter.FindComponent(AIControlComponent));
		CharacterControllerComponent characterController = CharacterControllerComponent.Cast(m_GC_MovementCharacter.FindComponent(CharacterControllerComponent));
		if (step == "takeover")
		{
			bool possessed = GetGame().GetPlayerController().SetControlledEntity(m_GC_MovementCharacter);
			m_GC_MovementRuntime.Update(10);
			return string.Format("possessed=%1 disabled=%2", possessed, characterController.GetDisableMovementControls());
		}
		if (step == "stop")
			m_GC_MovementRuntime.Stop();
		if (step == "controls" || step == "controlsRelease")
		{
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(m_GC_MovementCharacter.FindComponent(CharacterControllerComponent));
			controller.SetDisableMovementControls(step == "controls");
			m_vGC_MovementOrigin = m_GC_MovementCharacter.GetOrigin();
		}
		if (step == "fullSimulation")
			control.GetAIAgent().SetPermanentLOD(0);
		if (step == "move")
		{
			array<AIAgent> members = {};
			m_GC_MovementGroup.GetAgents(members);
			foreach (AIAgent member : members)
			{
				if (member != control.GetAIAgent())
					SCR_EntityHelper.DeleteEntityAndChildren(member.GetControlledEntity());
			}
			m_GC_MovementGroup.RemoveWaypoint(m_GC_MovementWaypoint);
			m_GC_MovementGroup.AddWaypoint(m_GC_MovementWaypoint);
			m_vGC_MovementOrigin = m_GC_MovementCharacter.GetOrigin();
		}
		if (step == "hold" || step == "release")
		{
			m_vGC_MovementOrigin = m_GC_MovementCharacter.GetOrigin();
			bool before = characterController.GetDisableMovementControls();
			bool applied = m_GC_MovementRuntime.SetAIHoldPosition(m_GC_MovementCharacter, step == "hold");
			return string.Format("applied=%1 position=%2 before=%3 after=%4", applied, m_vGC_MovementOrigin, before, characterController.GetDisableMovementControls());
		}
		AICharacterMovementComponent movement = AICharacterMovementComponent.Cast(m_GC_MovementCharacter.FindComponent(AICharacterMovementComponent));
		return string.Format("distance=%1 position=%2 wantedSpeed=%3", vector.Distance(m_vGC_MovementOrigin, m_GC_MovementCharacter.GetOrigin()),
			m_GC_MovementCharacter.GetOrigin(), movement.GetMovementTypeWanted())
			+ string.Format(" lod=%1 override=%2 disabled=%3", control.GetAIAgent().GetLOD(), movement.GetMovementTypeOverride(), characterController.GetDisableMovementControls());
	}
}
#endif
