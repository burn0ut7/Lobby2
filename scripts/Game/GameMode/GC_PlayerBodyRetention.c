//! One server-side adjustment per player death; native garbage collection owns
//! expiry and proximity handling. No polling, body registry or global hook.
modded class GC_LobbyManager
{
	protected static const float GC_PLAYER_BODY_LIFETIME_SECONDS = 3600;

	//------------------------------------------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		GC_RetainPlayerBody(instigatorContextData);
		super.OnControllableDestroyed(instigatorContextData);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RetainPlayerBody(SCR_InstigatorContextData context)
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode() || GC_LobbyManager.GetManager() != this
			|| !context || context.GetVictimPlayerID() <= 0)
			return;

		ChimeraCharacter body = ChimeraCharacter.Cast(context.GetVictimEntity());
		if (!body || body.FindComponent(GC_LobbySpectatorBodyComponent))
			return;

		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(body);
		if (!garbage)
			return;

		// Death notification and native insertion can occur in either order.
		// Keep a longer existing lifetime and respect native blacklist policy.
		if (!garbage.IsInserted(body))
		{
			garbage.Insert(body, GC_PLAYER_BODY_LIFETIME_SECONDS);
			return;
		}

		float remaining = garbage.GetRemainingLifetime(body);
		if (remaining < GC_PLAYER_BODY_LIFETIME_SECONDS)
			garbage.Bump(body, GC_PLAYER_BODY_LIFETIME_SECONDS - remaining);
	}
}
