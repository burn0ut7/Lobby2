//! Replicated body hold owned only by the active Lobby2 admin control-freeze service.
modded class SCR_EditableEntityComponent
{
	[RplProp(onRplName: "GC_OnAdminPhysicsFreezeChanged")]
	protected bool m_bGC_AdminPhysicsFrozen;
	protected Physics m_GC_FrozenPhysics;
	protected SimulationState m_eGC_PreFreezeSimulation;
	protected bool m_bGC_ChangedSimulation;
	protected GC_LobbyGameMode m_GC_FreezeMode;

	//------------------------------------------------------------------------------------------------
	bool GC_SetAdminPhysicsFrozen(bool frozen)
	{
		if (!Replication.IsServer() || (frozen && !GC_LobbyGameMode.GetGameMode()))
			return false;
		if (frozen && (!GetOwner() || !GetOwner().GetPhysics()))
			return false;
		if (m_bGC_AdminPhysicsFrozen == frozen)
			return true;
		m_bGC_AdminPhysicsFrozen = frozen;
		Replication.BumpMe();
		GC_OnAdminPhysicsFreezeChanged();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_OnAdminPhysicsFreezeChanged()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateAdminPhysicsFreeze);
		if (m_bGC_AdminPhysicsFrozen)
			GetGame().GetCallqueue().CallLater(GC_UpdateAdminPhysicsFreeze, 100, true);
		else
			m_GC_FreezeMode = null;
		GC_UpdateAdminPhysicsFreeze();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_UpdateAdminPhysicsFreeze()
	{
		if (!m_bGC_AdminPhysicsFrozen || !GetOwner())
		{
			GC_ReleaseAdminPhysicsFreeze();
			return;
		}
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode)
		{
			GC_RestoreAdminPhysics();
			// A streamed snapshot may precede game-mode initialization. It may wait
			// only before its first mode; an unrelated mode or mode exit stops it.
			if (m_GC_FreezeMode || GetGame().GetGameMode() || SCR_Global.IsEditMode())
				GC_ReleaseAdminPhysicsFreeze();
			return;
		}
		if (m_GC_FreezeMode && m_GC_FreezeMode != mode)
		{
			GC_ReleaseAdminPhysicsFreeze();
			return;
		}
		m_GC_FreezeMode = mode;
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(GetOwner().FindComponent(SCR_DamageManagerComponent));
		if (damage && damage.GetState() == EDamageState.DESTROYED)
		{
			GC_ReleaseAdminPhysicsFreeze();
			return;
		}
		Physics physics = GetOwner().GetPhysics();
		if (physics != m_GC_FrozenPhysics)
		{
			GC_RestoreAdminPhysics();
			m_GC_FrozenPhysics = physics;
			if (!physics)
				return;
			m_eGC_PreFreezeSimulation = physics.GetSimulationState();
			m_bGC_ChangedSimulation = m_eGC_PreFreezeSimulation == SimulationState.SIMULATION;
		}
		if (physics)
		{
			// Ownership/seat changes can resume the same native body. Reassert the
			// hold only on a live body; never turn NONE back into colliding physics.
			if (physics.GetSimulationState() == SimulationState.SIMULATION)
			{
				m_eGC_PreFreezeSimulation = SimulationState.SIMULATION;
				m_bGC_ChangedSimulation = true;
				physics.ChangeSimulationState(SimulationState.COLLISION);
			}
			if (physics.IsDynamic())
			{
				physics.SetVelocity(vector.Zero);
				physics.SetAngularVelocity(vector.Zero);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RestoreAdminPhysics()
	{
		// Do not overwrite a later destruction/compartment change made by the engine.
		if (m_GC_FrozenPhysics && m_bGC_ChangedSimulation
			&& m_GC_FrozenPhysics.GetSimulationState() == SimulationState.COLLISION)
			m_GC_FrozenPhysics.ChangeSimulationState(m_eGC_PreFreezeSimulation);
		m_GC_FrozenPhysics = null;
		m_bGC_ChangedSimulation = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ReleaseAdminPhysicsFreeze()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateAdminPhysicsFreeze);
		GC_RestoreAdminPhysics();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GC_ReleaseAdminPhysicsFreeze();
		super.OnDelete(owner);
	}
}
