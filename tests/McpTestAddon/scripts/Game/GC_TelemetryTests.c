//! Opt-in observers only. Production handlers remain the owners of recorded events.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Start and change native health in the same call, before scheduled discovery.
	string GC_TestTelemetryImmediateDeath()
	{
		PlayerController player = GetGame().GetPlayerController();
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !player || !logs)
			return "UNAVAILABLE authority/session";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return "UNAVAILABLE playing character";
		if (!logs.SetTelemetry(player.GetPlayerId(), true, false))
			return "REJECTED recording start";
		character.GetDamageManager().GetDefaultHitZone().SetHealthScaled(0);
		return logs.GetTelemetryReport();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryShotProbe()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || !player.GetControlledEntity())
			return "UNAVAILABLE character";
		IEntity entity = player.GetControlledEntity();
		EventHandlerManagerComponent events = EventHandlerManagerComponent.Cast(entity.FindComponent(EventHandlerManagerComponent));
		if (!events)
			return "UNAVAILABLE events";
		events.RegisterScriptHandler("OnProjectileShot", this, GC_TestTelemetryShot, false, true);
		return "ARMED one native shot observation";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestTelemetryShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		PrintFormat("GC | Telemetry test | native shot player=%1 weapon=%2 projectile=%3", playerId, weapon, projectile);
	}
}
