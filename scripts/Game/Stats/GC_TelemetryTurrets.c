//! A gun position is identified by its native manager/slot pair within the recorded vehicle.
class GC_TelemetryTurretState
{
	int m_iManager;
	int m_iSlot;
	int m_iSeat;
	int m_iType;
	bool m_bAvailable;
	bool m_bHasWeapon;
	vector m_vDirection;

	void Write(JsonSaveContext context)
	{
		context.WriteValue("managerId", m_iManager);
		context.WriteValue("slotId", m_iSlot);
		context.WriteValue("seatIndex", m_iSeat);
		context.WriteValue("compartmentType", m_iType);
		context.WriteValue("directionAvailable", m_bAvailable);
		context.WriteValue("hasWeapon", m_bHasWeapon);
		if (m_bAvailable)
			GC_TelemetryRecorder.WriteVector(context, "direction", m_vDirection);
	}
}

//! Replacement snapshot, including empty/unavailable states; never substitutes hull or head aim.
class GC_TelemetryTurrets
{
	protected bool m_bAvailable;
	protected ref array<ref GC_TelemetryTurretState> m_aTurrets = {};

	//------------------------------------------------------------------------------------------------
	bool Capture(IEntity entity)
	{
		m_aTurrets.Clear();
		SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(entity.FindComponent(SCR_BaseCompartmentManagerComponent));
		m_bAvailable = manager != null;
		if (!manager)
			return true;
		array<BaseCompartmentSlot> slots = {};
		manager.GetCompartments(slots);
		array<TurretControllerComponent> seen = {};
		foreach (BaseCompartmentSlot slot : slots)
		{
			TurretControllerComponent controller = TurretControllerComponent.Cast(slot.GetController());
			if (!controller)
				controller = slot.GetAttachedTurret();
			if (!controller || seen.Contains(controller))
				continue;
			seen.Insert(controller);
			if (m_aTurrets.Count() >= 64)
				return false;
			GC_TelemetryTurretState state = new GC_TelemetryTurretState();
			state.m_iManager = slot.GetCompartmentMgrID();
			state.m_iSlot = slot.GetCompartmentSlotID();
			state.m_iType = slot.GetType();
			array<BaseCompartmentSlot> sameType = {};
			manager.GetCompartmentsOfType(sameType, slot.GetType());
			state.m_iSeat = sameType.Find(slot);
			BaseWeaponManagerComponent weapons = controller.GetWeaponManager();
			array<IEntity> weaponEntities = {};
			if (weapons)
				weapons.GetWeaponsList(weaponEntities);
			state.m_bHasWeapon = !weaponEntities.IsEmpty();
			TurretComponent turret = controller.GetTurretComponent();
			if (turret)
			{
				state.m_vDirection = turret.GetAimingDirectionWorld();
				state.m_bAvailable = state.m_vDirection.LengthSq() > 0.000001;
				if (state.m_bAvailable)
					state.m_vDirection.Normalize();
			}
			m_aTurrets.Insert(state);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Write(JsonSaveContext context)
	{
		context.WriteValue("available", m_bAvailable);
		int count = m_aTurrets.Count();
		context.StartArray("turrets", count);
		foreach (GC_TelemetryTurretState turret : m_aTurrets)
		{
			context.StartObject();
			turret.Write(context);
			context.EndObject();
		}
		context.EndArray();
	}
}
