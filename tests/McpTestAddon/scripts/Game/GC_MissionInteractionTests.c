#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected GC_MissionInteraction m_GC_TestInteraction;
	protected IEntity m_GC_TestInteractionEntity;
	protected vector m_vGC_TestDestination;

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionInteraction(string step)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "UNAVAILABLE active GAME authority";
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!player || !SCR_AIDamageHandling.IsAlive(player))
			return "UNAVAILABLE live possessed character";
		if (step == "flag" || step == "teleport")
		{
			if (m_GC_TestInteractionEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(m_GC_TestInteractionEntity);
			ResourceName prefab = "{DA07433A05DA478F}Prefabs/Logic/MissionFramework/SetFlagInteraction.et";
			if (step == "teleport")
				prefab = "{EB3802402E4C407B}Prefabs/Logic/MissionFramework/TeleportInteraction.et";
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			player.GetWorldTransform(params.Transform);
			params.Transform[3] = player.GetOrigin() + "1 0 0";
			m_GC_TestInteractionEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
			if (!m_GC_TestInteractionEntity)
				return "FAIL interaction prefab";
			ActionsManagerComponent manager = ActionsManagerComponent.Cast(m_GC_TestInteractionEntity.FindComponent(ActionsManagerComponent));
			array<BaseUserAction> actions = {};
			manager.GetActionsList(actions);
			foreach (BaseUserAction action : actions)
			{
				m_GC_TestInteraction = GC_MissionInteraction.Cast(action);
				if (m_GC_TestInteraction)
					break;
			}
			if (!m_GC_TestInteraction)
				return "FAIL action not registered";
			m_GC_TestInteraction.m_aFactions = {"US"};
			GC_SetFlagInteraction flag = GC_SetFlagInteraction.Cast(m_GC_TestInteraction);
			if (flag)
			{
				flag.m_sFlag = "Interaction complete";
				flag.m_sTitle = "Complete objective";
			}
			GC_TeleportInteraction teleport = GC_TeleportInteraction.Cast(m_GC_TestInteraction);
			if (teleport)
			{
				teleport.m_sDestination = "MissionFrameworkMode";
				teleport.m_sTitle = "Return to staging";
				m_vGC_TestDestination = GetOwner().GetOrigin();
			}
			return "prepared; start on next call after native registration";
		}
		if (!m_GC_TestInteraction)
			return "UNAVAILABLE prepare an interaction first";
		if (step == "delete")
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_GC_TestInteractionEntity);
			m_GC_TestInteraction = null;
			m_GC_TestInteractionEntity = null;
			return string.Format("sourceDeleted=1 flag=%1", GC_IsMissionFlagSet("Interaction complete"));
		}
		if (step == "start")
		{
			bool allowed = m_GC_TestInteraction.CanBePerformed(player);
			if (allowed)
				player.DoStartObjectAction(m_GC_TestInteraction);
			return string.Format("allowed=%1; native action started", allowed);
		}
		if (step == "perform")
		{
			player.DoPerformObjectAction(m_GC_TestInteraction);
			return "native action dispatched; inspect on next call";
		}
		if (step == "denied")
		{
			GC_GetMissionRuntime().SetFlag("Interaction complete", false);
			m_GC_TestInteraction.m_aFactions = {"USSR"};
			bool denied = !m_GC_TestInteraction.CanBePerformed(player);
			player.DoPerformObjectAction(m_GC_TestInteraction);
			return string.Format("factionDenied=%1; inspect flag on next call", denied);
		}
		bool flagSet = GC_IsMissionFlagSet("Interaction complete");
		bool canPerform = m_GC_TestInteraction.CanBePerformed(player);
		float distance = vector.Distance(player.GetOrigin(), m_vGC_TestDestination);
		return string.Format("flag=%1 canPerform=%2 distanceToDestination=%3", flagSet, canPerform, distance);
	}
}
#endif
