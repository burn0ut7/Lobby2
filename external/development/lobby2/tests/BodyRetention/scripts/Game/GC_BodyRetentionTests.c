//! Explicit local development checks. Never mounted in a Workshop build.
modded class GC_LobbyManager
{
	protected IEntity m_GC_TestRetainedBody;
	protected int m_iGC_TestBodyPlayer;

	string GC_KillBodyRetentionPlayer()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "BLOCKED: local match required";
		ChimeraCharacter body = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!body || body.FindComponent(GC_LobbySpectatorBodyComponent) || body.GetCharacterController().IsDead())
			return "BLOCKED: living player character required";
		m_GC_TestRetainedBody = body;
		m_iGC_TestBodyPlayer = player.GetPlayerId();
		body.GetCharacterController().ForceDeath();
		return "death requested; inspect after native death handling";
	}

	string GC_CheckBodyRetention()
	{
		if (!m_GC_TestRetainedBody)
			return "FAIL: player body missing";
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(m_GC_TestRetainedBody);
		if (!garbage)
			return "BLOCKED: no native garbage system";
		float remaining = garbage.GetRemainingLifetime(m_GC_TestRetainedBody);
		return string.Format("player retained=%1 remaining=%2 seconds", garbage.IsInserted(m_GC_TestRetainedBody) && remaining > 3500, remaining);
	}

	string GC_TestBodyRetentionPolicies()
	{
		if (!Replication.IsServer() || !m_GC_TestRetainedBody)
			return "BLOCKED: run real player death first";
		IEntity body = m_GC_TestRetainedBody;
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(body);
		SCR_InstigatorContextData context = new SCR_InstigatorContextData(m_iGC_TestBodyPlayer, body, body, Instigator.CreateInstigator(body));
		int passed;
		garbage.Withdraw(body);
		GC_LobbyManager.Cast(this).GC_RetainPlayerBody(context);
		if (garbage.IsInserted(body) && garbage.GetRemainingLifetime(body) >= 3599)
			passed++;
		garbage.Withdraw(body);
		garbage.Insert(body, 30);
		GC_LobbyManager.Cast(this).GC_RetainPlayerBody(context);
		if (garbage.GetRemainingLifetime(body) >= 3599)
			passed++;
		garbage.Withdraw(body);
		garbage.Insert(body, 7200);
		GC_LobbyManager.Cast(this).GC_RetainPlayerBody(context);
		if (garbage.GetRemainingLifetime(body) >= 7199)
			passed++;
		garbage.Withdraw(body);
		garbage.UpdateBlacklist(body, true);
		GC_LobbyManager.Cast(this).GC_RetainPlayerBody(context);
		if (!garbage.IsInserted(body))
			passed++;
		garbage.UpdateBlacklist(body, false);
		garbage.Insert(body, 3600);
		return string.Format("policy checks=%1/4 (new, existing, longer, blacklist)", passed);
	}

	string GC_TestAIBodyRetention()
	{
		if (!Replication.IsServer() || !m_GC_TestRetainedBody)
			return "BLOCKED: run player death first";
		Resource resource = Resource.Load(m_GC_TestRetainedBody.GetPrefabData().GetPrefabName());
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		m_GC_TestRetainedBody.GetWorldTransform(spawn.Transform);
		spawn.Transform[3] = spawn.Transform[3] + "3 0 0";
		ChimeraCharacter ai = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawn));
		if (!ai)
			return "FAIL: AI fixture spawn failed";
		ai.GetCharacterController().ForceDeath();
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(ai);
		float before = garbage.GetRemainingLifetime(ai);
		SCR_InstigatorContextData context = new SCR_InstigatorContextData(-1, ai, ai, Instigator.CreateInstigator(ai));
		GC_LobbyManager.Cast(this).GC_RetainPlayerBody(context);
		float after = garbage.GetRemainingLifetime(ai);
		string result = string.Format("AI unchanged=%1 playerId=%2 before=%3 after=%4", Math.AbsFloat(before - after) < 1 && context.GetVictimPlayerID() <= 0, context.GetVictimPlayerID(), before, after);
		SCR_EntityHelper.DeleteEntityAndChildren(ai);
		return result;
	}
}
