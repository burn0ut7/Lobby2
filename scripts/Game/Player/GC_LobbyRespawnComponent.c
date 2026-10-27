class GC_LobbyRespawnComponentClass : SCR_RespawnComponentClass
{
}

//! Lobby2-owned component: record the accepted Respawn action before its native death.
class GC_LobbyRespawnComponent : SCR_RespawnComponent
{
	//------------------------------------------------------------------------------------------------
	override void RequestPlayerSuicide()
	{
		if (!GC_LobbyGameMode.GetGameMode())
		{
			super.RequestPlayerSuicide();
			return;
		}
		PlayerController controller = m_PlayerController;
		if (!controller || GetGame().GetPlayerController() != controller)
			return;
		IEntity victim = controller.GetControlledEntity();
		if (!victim)
			return;
		RplComponent replication = RplComponent.Cast(victim.FindComponent(RplComponent));
		if (replication)
			Rpc(RpcAsk_RespawnDeath, replication.Id());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnDeath(RplId expectedVictim)
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		PlayerController controller = m_PlayerController;
		if (!controller)
			return;
		ChimeraCharacter victim = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!victim || victim.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		RplComponent replication = RplComponent.Cast(victim.FindComponent(RplComponent));
		CharacterControllerComponent character = victim.GetCharacterController();
		if (!replication || replication.Id() != expectedVictim || !character || character.IsDead())
			return;
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (stats)
			stats.MarkRespawn(controller.GetPlayerId(), victim);
		character.ForceDeath();
	}
}
