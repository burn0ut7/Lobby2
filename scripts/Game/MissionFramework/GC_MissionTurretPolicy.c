//! The native AI offers no per-character setter for its turret-investigation
//! dismount rule. This hook only affects explicitly marked mission-spawned crew.
modded class SCR_AICombatComponent
{
	protected GC_LobbyMissionContext m_GC_TurretPolicyContext;

	//------------------------------------------------------------------------------------------------
	void GC_KeepMissionTurretSeated(GC_LobbyMissionContext context)
	{
		if (context && context.IsActive() && context.GetRuntime())
			m_GC_TurretPolicyContext = context;
	}

	//------------------------------------------------------------------------------------------------
	override bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided, out float threatPriority)
	{
		if (m_GC_TurretPolicyContext && m_GC_TurretPolicyContext.IsActive() && m_GC_TurretPolicyContext.GetRuntime())
		{
			threatPriority = 0;
			return false;
		}
		m_GC_TurretPolicyContext = null;
		return super.DismountTurretCondition(targetPos, targetPosProvided, threatPriority);
	}
}
