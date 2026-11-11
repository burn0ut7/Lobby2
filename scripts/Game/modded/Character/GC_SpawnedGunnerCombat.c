//! Keep only explicitly configured Lobby2 spawn-action gunners in their turrets.
modded class SCR_AICombatComponent
{
	protected GC_LobbyGameMode m_GC_GunnerSeatMode;

	//------------------------------------------------------------------------------------------------
	void GC_KeepSpawnedGunnerSeated()
	{
		m_GC_GunnerSeatMode = GC_LobbyGameMode.GetGameMode();
	}

	//------------------------------------------------------------------------------------------------
	override bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided, out float threatPriority)
	{
		if (m_GC_GunnerSeatMode && m_GC_GunnerSeatMode == GC_LobbyGameMode.GetGameMode())
		{
			threatPriority = 0;
			return false;
		}
		m_GC_GunnerSeatMode = null;
		return super.DismountTurretCondition(targetPos, targetPosProvided, threatPriority);
	}
}
