#ifdef WORKBENCH
//! Explicit fixture creation; admission is exclusively the production discovery loop.
modded class GC_LobbyManager
{
	protected IEntity m_GC_DiscoveryAI;
	protected IEntity m_GC_DiscoveryPlayerBody;

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryDiscovery(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (GetManager() != this || !Replication.IsServer() || !player)
			return "unavailable";
		if (action == "spawn")
		{
			if (m_GC_DiscoveryAI)
				return "already-spawned";
			Resource prefab = Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et");
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			parameters.Transform[3] = "1180 39 2730";
			m_GC_DiscoveryAI = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters);
		}
		if (action == "switch")
		{
			m_GC_DiscoveryPlayerBody = player.GetControlledEntity();
			GC_Slot target = GetSlot(10);
			return ExecuteAdminAssignSlot(player.GetPlayerId(), player.GetPlayerId(), SCR_PlayerIdentityUtils.GetPlayerIdentityId(player.GetPlayerId()), 10, target.GetUUID());
		}
		array<AIAgent> agents = {};
		GetGame().GetAIWorld().GetAIAgents(agents);
		bool found;
		foreach (AIAgent agent : agents)
		{
			if (agent && agent.GetControlledEntity() == m_GC_DiscoveryAI)
				found = true;
		}
		int oldOwner;
		if (m_GC_DiscoveryPlayerBody)
			oldOwner = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_GC_DiscoveryPlayerBody);
		vector position;
		if (m_GC_DiscoveryAI)
			position = m_GC_DiscoveryAI.GetOrigin();
		return string.Format("ai=%1 nativeRegistered=%2 position=%3 oldBody=%4 oldOwner=%5", m_GC_DiscoveryAI != null, found, position, m_GC_DiscoveryPlayerBody != null, oldOwner);
	}
}
#endif
