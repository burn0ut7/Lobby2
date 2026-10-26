#ifdef WORKBENCH
//! Addon-independent fixture: use the same native call with and without Lobby2.
class GC_IsolationDismountProbe
{
	IEntity m_Turret;
	IEntity m_Character;

	//------------------------------------------------------------------------------------------------
	bool Spawn(vector position)
	{
		if (!Replication.IsServer() || m_Turret)
			return false;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		m_Turret = GetGame().SpawnEntityPrefab(Resource.Load("{73530808E4B455BA}Prefabs/Weapons/Tripods/Tripod_M3_M2HB.et"), GetGame().GetWorld(), params);
		if (!m_Turret)
			return false;
		SCR_BaseCompartmentManagerComponent manager = SCR_BaseCompartmentManagerComponent.Cast(m_Turret.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!manager)
			return false;
		array<BaseCompartmentSlot> seats = {};
		manager.GetCompartments(seats);
		foreach (BaseCompartmentSlot seat : seats)
		{
			if (seat.GetType() != ECompartmentType.TURRET)
				continue;
			AIGroup group;
			m_Character = seat.SpawnCharacterInCompartment("{EA158B6EB6A24B4B}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman_Variant_1.et", group);
			return m_Character != null;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	string Snapshot()
	{
		if (!m_Character || !m_Turret)
			return "UNAVAILABLE fixture";
		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(m_Character.FindComponent(SCR_AICombatComponent));
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Character);
		bool seated = character && character.GetCompartmentAccessComponent().GetCompartment() != null;
		if (!combat || !seated)
			return string.Format("UNAVAILABLE combat=%1 seated=%2", combat != null, seated);
		vector target = m_Turret.GetOrigin() + "0 100 0";
		float priority;
		bool dismount = combat.DismountTurretCondition(target, true, priority);
		return string.Format("seated=%1 dismount=%2 priority=%3", seated, dismount, priority);
	}
}

modded class GC_IsolationProbeComponent
{
	protected ref GC_IsolationDismountProbe m_GC_DismountProbe;

	//------------------------------------------------------------------------------------------------
	string DismountTest(string step)
	{
		if (step == "start")
		{
			if (m_GC_DismountProbe)
				return "UNAVAILABLE fixture already exists";
			m_GC_DismountProbe = new GC_IsolationDismountProbe();
			bool spawned = m_GC_DismountProbe.Spawn(GetOwner().GetOrigin() + "0 0 5");
			return string.Format("spawned=%1; inspect after boarding", spawned);
		}
		if (!m_GC_DismountProbe)
			return "UNAVAILABLE start first";
		return m_GC_DismountProbe.Snapshot();
	}
}
#endif
