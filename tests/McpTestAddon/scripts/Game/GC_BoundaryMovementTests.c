//! Test-only observations and physical approach fixtures; production owns enforcement.
modded class GC_LobbyManager
{
	protected int m_iGC_BoundaryFades;
	protected SCR_PlayerTeleportedFeedbackComponent m_GC_BoundaryFeedback;

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryWatch()
	{
		if (m_GC_BoundaryFeedback)
			m_GC_BoundaryFeedback.GetOnPlayerTeleported().Remove(GC_TestBoundaryFade);
		m_GC_BoundaryFeedback = SCR_PlayerTeleportedFeedbackComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_PlayerTeleportedFeedbackComponent));
		if (!m_GC_BoundaryFeedback)
			return "UNAVAILABLE feedback";
		m_iGC_BoundaryFades = 0;
		m_GC_BoundaryFeedback.GetOnPlayerTeleported().Insert(GC_TestBoundaryFade);
		return "watching vanilla teleport-feedback invoker";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBoundaryFade(bool editorOpen, bool longFade, SCR_EPlayerTeleportedReason reason)
	{
		m_iGC_BoundaryFades++;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryArrange(vector offset, vector velocity, float yaw)
	{
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		BaseGameEntity body = BaseGameEntity.Cast(CompartmentAccessComponent.GetVehicleIn(character));
		if (!body)
			body = BaseGameEntity.Cast(character);
		if (!body || !Replication.IsServer())
			return "UNAVAILABLE body/authority";
		vector transform[4];
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), transform);
		transform[3] = m_vGC_AOOrigin + offset;
		body.Teleport(transform);
		Physics physics = body.GetPhysics();
		if (physics)
		{
			physics.SetVelocity(velocity);
			physics.SetAngularVelocity(vector.Zero);
		}
		return GC_TestBoundarySnapshot();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBoundarySnapshot()
	{
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		IEntity body = CompartmentAccessComponent.GetVehicleIn(character);
		if (!body)
			body = character;
		if (!body)
			return "UNAVAILABLE body";
		vector velocity, spin;
		Physics physics = body.GetPhysics();
		if (physics)
		{
			velocity = physics.GetVelocity();
			spin = physics.GetAngularVelocity();
		}
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(body.FindComponent(SCR_DamageManagerComponent));
		array<HitZone> zones = {};
		int damaged;
		if (damage)
		{
			damage.GetAllHitZonesInHierarchy(zones);
			foreach (HitZone zone : zones)
			{
				if (zone && zone.GetHealthScaled() < 0.999)
					damaged++;
			}
		}
		return string.Format("body=%1 offset=%2 angles=%3 velocity=%4 spin=%5 damaged=%6 fades=%7 alive=%8",
			body, body.GetOrigin() - m_vGC_AOOrigin, body.GetAngles(), velocity, spin, damaged, m_iGC_BoundaryFades, SCR_AIDamageHandling.IsAlive(character));
	}

	//------------------------------------------------------------------------------------------------
	override void Shutdown()
	{
		if (m_GC_BoundaryFeedback)
			m_GC_BoundaryFeedback.GetOnPlayerTeleported().Remove(GC_TestBoundaryFade);
		super.Shutdown();
	}
}
