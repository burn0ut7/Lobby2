//! Per-request collection; callers authorize the admin and validate the radius first.
class GC_AdminNearbyVehicleRepair
{
	protected ref array<IEntity> m_aVehicles = {};

	//------------------------------------------------------------------------------------------------
	string Execute(int requesterId, vector centre, int radiusMeters, string context)
	{
		m_aVehicles.Clear();
		TraceBox trace = new TraceBox();
		trace.Start = centre;
		trace.Mins = Vector(-radiusMeters, -radiusMeters, -radiusMeters);
		trace.Maxs = Vector(radiusMeters, radiusMeters, radiusMeters);
		trace.Flags = TraceFlags.ENTS;
		trace.LayerMask = EPhysicsLayerDefs.Projectile;
		GetGame().GetWorld().TracePosition(trace, CollectVehicle);
		int repaired;
		int skipped;
		foreach (IEntity vehicle : m_aVehicles)
		{
			if (!vehicle || vector.DistanceSq(centre, vehicle.GetOrigin()) > radiusMeters * radiusMeters)
				continue;
			SCR_VehicleDamageManagerComponent damage = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
			if (!damage)
				continue;
			string target = string.Format("%1 vehicle=%2 position=%3", context, Replication.FindId(vehicle), vehicle.GetOrigin());
			// Native repair does not reconstruct a destroyed vehicle.
			if (damage.GetState() == EDamageState.DESTROYED)
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "heal-vehicle", target, "skipped-destroyed");
				continue;
			}
			damage.FullHeal();
			array<HitZone> zones = {};
			damage.GetAllHitZonesInHierarchy(zones);
			bool restored = !zones.IsEmpty();
			foreach (HitZone zone : zones)
			{
				if (zone && zone.GetDamageState() != EDamageState.UNDAMAGED)
					restored = false;
			}
			if (restored)
			{
				repaired++;
				GC_AdminActionLogger.Log(requesterId, "heal-vehicle", target, "applied");
			}
			else
			{
				skipped++;
				GC_AdminActionLogger.Log(requesterId, "heal-vehicle", target, "health-not-fully-restored");
			}
		}
		return string.Format("Repaired %1 vehicle(s). Skipped %2 destroyed or unhealed vehicle(s).", repaired, skipped);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectVehicle(IEntity entity)
	{
		IEntity root = entity.GetRootParent();
		if (root && root.FindComponent(SCR_VehicleDamageManagerComponent) && !m_aVehicles.Contains(root))
			m_aVehicles.Insert(root);
		// Reject hits so the trace visits all candidates; distance filtering happens above.
		return false;
	}
}
