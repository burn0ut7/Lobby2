#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionEntityAuthoring()
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE active authority";
		GC_MissionEventEntity root = GC_MissionEventEntity.Cast(GetGame().SpawnEntity(GC_MissionEventEntity));
		GC_ConditionGroupEntity group = GC_ConditionGroupEntity.Cast(GetGame().SpawnEntity(GC_ConditionGroupEntity));
		GC_FlagConditionEntity check = GC_FlagConditionEntity.Cast(GetGame().SpawnEntity(GC_FlagConditionEntity));
		GC_SetFlagActionEntity setFlag = GC_SetFlagActionEntity.Cast(GetGame().SpawnEntity(GC_SetFlagActionEntity));
		GC_EndMissionActionEntity end = GC_EndMissionActionEntity.Cast(GetGame().SpawnEntity(GC_EndMissionActionEntity));
		root.AddChild(group, -1);
		group.AddChild(check, -1);
		root.AddChild(setFlag, -1);
		root.AddChild(end, -1);
		root.m_sName = "Entity authoring fixture";
		root.m_iOrder = 2000000000;
		root.m_bAllowBeforeGame = true;
		check.m_sFlag = "GC entity fixture complete";
		check.m_bInvert = true;
		setFlag.m_sFlag = "GC entity fixture complete";
		setFlag.m_bSet = true;
		setFlag.m_fDelayAfterEventStarts = 2;
		end.m_iOrder = 1;
		end.m_sWinningFaction = "US";
		end.m_eEnding = EGameOverTypes.EDITOR_FACTION_VICTORY;
		end.m_fDelayAfterEventStarts = 3;
		string error;
		GC_MissionEvent definition = root.BuildEvent(error);
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(this);
		GC_MissionRuntime runtime = new GC_MissionRuntime();
		array<ref GC_MissionEvent> definitions = {definition};
		bool previewBefore = m_bPreviewMissionEnd;
		m_bPreviewMissionEnd = true;
		bool started = definition && runtime.Start(context, definitions);
		runtime.Update(100);
		runtime.Update(101);
		bool waited = !runtime.IsFlagSet("GC entity fixture complete");
		runtime.Update(102);
		bool applied = runtime.IsFlagSet("GC entity fixture complete");
		runtime.Update(103);
		runtime.Update(104);
		string state = runtime.Describe();
		runtime.Stop();
		m_bPreviewMissionEnd = previewBefore;
		end.m_iOrder = setFlag.m_iOrder;
		bool rejectedOrder = !root.BuildEvent(error);
		SCR_EntityHelper.DeleteEntityAndChildren(root);
		return string.Format("pass=%1 started=%2 waited=%3 applied=%4 duplicateOrderRejected=%5 phase=%6 state=%7",
			started && waited && applied && rejectedOrder, started, waited, applied, rejectedOrder, GetPhaseName(GetLobbyPhase()), state);
	}
}
#endif
