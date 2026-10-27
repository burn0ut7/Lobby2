//! Per-identity subscriptions. A disconnected living body retains its current-life damage history.
class GC_StatsCharacterTracker
{
	protected GC_MissionStatsComponent m_Stats;
	protected GC_StatsPlayer m_Record;
	protected int m_iPlayerId;
	protected IEntity m_Character;
	protected EventHandlerManagerComponent m_Events;
	protected SCR_CharacterControllerComponent m_Controller;
	protected SCR_DamageManagerComponent m_Damage;
	protected ref GC_StatsRole m_Role;
	protected int m_iSlotId = -1;
	protected vector m_vPreviousPosition;
	protected IEntity m_MovementEntity;
	protected int m_iMovementKind = -1;
	protected int m_iDamageInstigator;
	protected string m_sDamageWeapon;
	protected float m_fDamageImpactTime = -1;
	protected EntityID m_DamageCharacterId;
	protected EntityID m_LastPlayerDamageVictim;
	protected GC_StatsPlayer m_LastAttacker;
	protected string m_sLastPlayerWeapon;
	protected int m_iLastPlayerRelation;
	protected int m_iLastPlayerDamageSequence;
	protected float m_fLastPlayerDistance = -1;
	protected EntityID m_RespawnVictim;
	protected EntityID m_LifeCharacterId;

	//------------------------------------------------------------------------------------------------
	void Start(GC_MissionStatsComponent stats, int playerId, GC_StatsPlayer record)
	{
		m_Stats = stats;
		m_iPlayerId = playerId;
		m_Record = record;
	}

	//------------------------------------------------------------------------------------------------
	GC_StatsPlayer GetRecord()
	{
		return m_Record;
	}

	//------------------------------------------------------------------------------------------------
	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	bool TracksLife(IEntity victim)
	{
		return victim && victim.GetID() == m_LifeCharacterId;
	}

	//------------------------------------------------------------------------------------------------
	void Disconnect(float until)
	{
		m_Record.SyncAssignment(until);
		m_Record.m_ActiveAssignment = null;
		m_iPlayerId = 0;
		m_MovementEntity = null;
	}

	//------------------------------------------------------------------------------------------------
	void Detach()
	{
		if (m_Events)
		{
			m_Events.RemoveScriptHandler("OnProjectileShot", this, OnShot);
			m_Events.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenade);
		}
		if (m_Controller)
			m_Controller.m_OnItemUseEndedInvoker.Remove(OnItemUsed);
		if (m_Damage)
			m_Damage.GetOnDamage().Remove(OnDamage);
		m_Events = null;
		m_Controller = null;
		m_Damage = null;
		m_Character = null;
		m_MovementEntity = null;
		m_iMovementKind = -1;
		// Spectator possession can precede the game-mode death callback. Keep the
		// last impact keyed to its victim; a new body must never inherit it.
		m_fDamageImpactTime = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected void Attach(IEntity character)
	{
		Detach();
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(character);
		if (!player || player.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		m_Character = player;
		m_LifeCharacterId = player.GetID();
		if (m_LastPlayerDamageVictim != player.GetID())
			m_LastAttacker = null;
		m_Events = EventHandlerManagerComponent.Cast(player.FindComponent(EventHandlerManagerComponent));
		if (m_Events)
		{
			m_Events.RegisterScriptHandler("OnProjectileShot", this, OnShot);
			m_Events.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenade);
		}
		m_Controller = SCR_CharacterControllerComponent.Cast(player.GetCharacterController());
		if (m_Controller)
			m_Controller.m_OnItemUseEndedInvoker.Insert(OnItemUsed);
		m_Damage = SCR_DamageManagerComponent.Cast(player.FindComponent(SCR_DamageManagerComponent));
		if (m_Damage)
			m_Damage.GetOnDamage().Insert(OnDamage);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateRole(GC_LobbyManager manager)
	{
		m_Stats.UpdateAssignment(m_iPlayerId, m_Record);
		GC_Slot slot = manager.GetSlotByPlayerId(m_iPlayerId);
		if (!slot)
		{
			m_iSlotId = -1;
			m_Role = null;
			return;
		}
		if (slot.GetID() != m_iSlotId)
		{
			m_iSlotId = slot.GetID();
			m_Role = null;
			foreach (GC_StatsRole role : m_Record.m_aRoles)
			{
				if (role.m_iSlotId == m_iSlotId && role.m_sPrefab == slot.GetAuthoredPrefab())
					m_Role = role;
			}
			if (!m_Role)
			{
				m_Role = new GC_StatsRole();
				m_Role.m_iSlotId = m_iSlotId;
				m_Role.m_sPrefab = slot.GetAuthoredPrefab();
				SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(slot.GetAuthoredPrefab());
				if (info)
					m_Role.m_sName = info.GetName();
				m_Record.m_aRoles.Insert(m_Role);
			}
			if (m_Record.m_iLastSlotId != m_iSlotId)
				m_Role.m_iSelections++;
			m_Record.m_iLastSlotId = m_iSlotId;
		}
		Faction faction = manager.GetPlayerFaction(m_iPlayerId);
		if (faction && manager.GetFactionCommanderPlayerId(faction.GetFactionKey()) == m_iPlayerId)
			m_Record.m_iCommanderMissions = 1;
		SCR_AIGroup group = manager.GetPlayerGroup(m_iPlayerId);
		if (group && group.GetLeaderID() == m_iPlayerId)
			m_Record.m_iLeaderMissions = 1;
	}

	//------------------------------------------------------------------------------------------------
	void Sample(IEntity character, float elapsed, bool creditTime, GC_LobbyManager manager)
	{
		if (creditTime)
			m_Record.m_fPlaySeconds += elapsed;
		UpdateRole(manager);
		if (m_Role && creditTime)
			m_Role.m_fSeconds += elapsed;
		if (m_Character != character)
		{
			// Losing possession during reconnect does not end a living character's life.
			if (!character && m_Character && SCR_AIDamageHandling.IsAlive(m_Character))
				return;
			Attach(character);
		}
		if (!m_Character || !SCR_AIDamageHandling.IsAlive(m_Character))
		{
			m_MovementEntity = null;
			return;
		}
		IEntity moving = m_Character;
		int kind = 0;
		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(m_Character.FindComponent(CompartmentAccessComponent));
		if (access && access.IsInCompartment())
		{
			BaseCompartmentSlot seat = access.GetCompartment();
			moving = CompartmentAccessComponent.GetVehicleIn(m_Character);
			kind = -1;
			if (moving && seat && seat.GetType() == ECompartmentType.PILOT)
			{
				kind = 1;
				if (moving.FindComponent(HelicopterControllerComponent) || moving.FindComponent(AirplaneControllerComponent))
					kind = 2;
			}
		}
		if (!moving || kind < 0)
		{
			m_MovementEntity = null;
			return;
		}
		vector position = moving.GetOrigin();
		if (moving == m_MovementEntity && kind == m_iMovementKind && elapsed > 0)
		{
			float distance = vector.Distance(position, m_vPreviousPosition);
			float maximumSpeed = 15;
			if (kind == 1)
				maximumSpeed = 100;
			else if (kind == 2)
				maximumSpeed = 400;
			// Discard teleport-sized jumps; never convert them to credited capped distance.
			if (distance <= maximumSpeed * elapsed)
			{
				if (kind == 0)
					m_Record.m_fWalkMetres += distance;
				else if (kind == 1)
					m_Record.m_fDriveMetres += distance;
				else
					m_Record.m_fPilotMetres += distance;
			}
		}
		m_MovementEntity = moving;
		m_iMovementKind = kind;
		m_vPreviousPosition = position;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!m_Stats || !m_Stats.IsCollecting() || m_iPlayerId <= 0 || playerId != m_iPlayerId)
			return;
		m_Stats.UpdateAssignment(m_iPlayerId, m_Record);
		m_Record.m_iShots++;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
			logs.RecordShot("shot", playerId, weapon, projectile);
		RecordAmmo(weapon, projectile);
		m_Stats.ObserveProjectile(weapon, projectile);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenade(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!m_Stats || !m_Stats.IsCollecting() || m_iPlayerId <= 0 || playerId != m_iPlayerId)
			return;
		m_Stats.UpdateAssignment(m_iPlayerId, m_Record);
		m_Record.m_iGrenades++;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
			logs.RecordShot("grenade", playerId, weapon, projectile);
		RecordAmmo(weapon, projectile);
		m_Stats.ObserveProjectile(weapon, projectile);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnItemUsed(IEntity item, bool completed, ItemUseParameters parameters)
	{
		if (!m_Stats || !m_Stats.IsCollecting() || m_iPlayerId <= 0 || !item || !completed)
			return;
		SCR_ConsumableItemComponent consumable = SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
		if (!consumable || consumable.GetConsumableType() != SCR_EConsumableType.BANDAGE)
			return;
		IEntity target = consumable.GetTargetCharacter();
		if (consumable.GetCharacterOwner() == m_Character && target && target != m_Character
			&& GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(target) > 0)
		{
			m_Stats.UpdateAssignment(m_iPlayerId, m_Record);
			m_Record.m_iBandagesOthers++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RecordAmmo(BaseWeaponComponent weapon, IEntity projectile)
	{
		GC_StatsAssignment assignment = m_Record.m_ActiveAssignment;
		if (!assignment)
			return;
		string projectilePrefab;
		string magazinePrefab;
		string weaponPrefab;
		if (projectile && projectile.GetPrefabData())
			projectilePrefab = projectile.GetPrefabData().GetPrefabName();
		if (weapon)
		{
			if (weapon.GetOwner() && weapon.GetOwner().GetPrefabData())
				weaponPrefab = weapon.GetOwner().GetPrefabData().GetPrefabName();
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			if (magazine && magazine.GetOwner() && magazine.GetOwner().GetPrefabData())
				magazinePrefab = magazine.GetOwner().GetPrefabData().GetPrefabName();
		}
		foreach (GC_StatsAmmo existing : assignment.m_aAmmo)
		{
			if (existing.m_sProjectile == projectilePrefab && existing.m_sMagazine == magazinePrefab && existing.m_sWeapon == weaponPrefab)
			{
				existing.m_iRounds++;
				return;
			}
		}
		if (assignment.m_aAmmo.Count() >= 128)
		{
			m_Stats.MarkIncomplete();
			return;
		}
		if (!m_Stats.AllocateAmmoType())
			return;
		GC_StatsAmmo ammo = new GC_StatsAmmo();
		ammo.m_sProjectile = projectilePrefab;
		ammo.m_sMagazine = magazinePrefab;
		ammo.m_sWeapon = weaponPrefab;
		ammo.m_iRounds = 1;
		assignment.m_aAmmo.Insert(ammo);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDamage(notnull BaseDamageContext context)
	{
		if (!m_Stats || !m_Stats.IsCollecting() || context.damageType == EDamageType.HEALING
			|| context.damageType == EDamageType.REGENERATION)
			return;
		// The engine follows projectile impacts with an empty notification. It must
		// not erase the source captured by the actual impact callback.
		if (context.damageType == EDamageType.KINETIC && context.instigator
			&& context.instigator.GetInstigatorPlayerID() == m_iDamageInstigator
			&& GetGame().GetWorld().GetWorldTime() == m_fDamageImpactTime
			&& !context.damageSource && context.hitPosition.LengthSq() < 0.0001
			&& context.impactVelocity.LengthSq() < 0.0001)
			return;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		int damageSequence;
		if (logs)
		{
			int actorId;
			if (context.instigator)
				actorId = context.instigator.GetInstigatorPlayerID();
			GC_SessionEvent entry = logs.CreateEvent("damage", actorId, m_iPlayerId);
			entry.m_sTarget = m_Record.m_sIdentity;
			entry.m_sTargetName = m_Record.m_sName;
			entry.m_fDamage = context.damageValue;
			entry.m_iDamageType = context.damageType;
			entry.m_sWeapon = m_Stats.GetProjectileWeapon(context.damageSource);
			if (context.damageSource && !context.damageSource.FindComponent(BaseWeaponComponent) && context.damageSource.GetPrefabData())
				entry.m_sProjectile = context.damageSource.GetPrefabData().GetPrefabName();
			logs.Record(entry);
			damageSequence = entry.m_iSequence;
		}
		// Native bleed-out Kill() emits a source-less TRUE damage notification with
		// the retained instigator. Keep that event, but do not call it a new hit.
		if (context.damageType == EDamageType.BLEEDING
			|| (context.damageType == EDamageType.TRUE && m_Character && GetDeathCause(m_Character) != "other"))
			return;
		m_iDamageInstigator = 0;
		if (context.instigator)
			m_iDamageInstigator = context.instigator.GetInstigatorPlayerID();
		m_sDamageWeapon = m_Stats.GetProjectileWeapon(context.damageSource);
		m_fDamageImpactTime = GetGame().GetWorld().GetWorldTime();
		if (m_Character)
			m_DamageCharacterId = m_Character.GetID();
		// Snapshot historical credit from native ownership/relations at the damaging hit.
		// Self damage, bleeding ticks and healing cannot erase the last other player.
		if (m_Character && context.damageValue > 0 && m_iDamageInstigator > 0 && m_iDamageInstigator != m_iPlayerId
			&& context.instigator.GetInstigatorType() != InstigatorType.INSTIGATOR_GM)
		{
			GC_StatsPlayer attackerRecord = m_Stats.GetPlayer(m_iDamageInstigator);
			IEntity attacker = context.instigator.GetInstigatorEntity();
			if (!attackerRecord || attackerRecord == m_Record || attacker == m_Character)
				return;
			m_LastAttacker = attackerRecord;
			m_LastPlayerDamageVictim = m_Character.GetID();
			m_sLastPlayerWeapon = m_sDamageWeapon;
			m_iLastPlayerDamageSequence = damageSequence;
			SCR_InstigatorContextData relation = new SCR_InstigatorContextData(m_iPlayerId, m_Character, attacker, context.instigator);
			m_iLastPlayerRelation = relation.GetVictimKillerRelation();
			m_fLastPlayerDistance = -1;
			if (attacker)
				m_fLastPlayerDistance = vector.Distance(attacker.GetOrigin(), m_Character.GetOrigin());
		}
	}

	//------------------------------------------------------------------------------------------------
	void MarkRespawn(IEntity victim)
	{
		if (victim)
			m_RespawnVictim = victim.GetID();
	}

	//------------------------------------------------------------------------------------------------
	string GetDeathCause(IEntity victim)
	{
		if (victim && victim.GetID() == m_RespawnVictim)
			return "respawn";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(victim.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damage && damage.GetBloodHitZone() && damage.GetBloodHitZone().GetDamageState() == ECharacterBloodState.DESTROYED)
			return "bleeding";
		return "other";
	}

	//------------------------------------------------------------------------------------------------
	GC_StatsPlayer ApplyDelayedCredit(IEntity victim, GC_StatsDeath death)
	{
		if (!victim || victim.GetID() != m_LastPlayerDamageVictim || !m_LastAttacker)
			return null;
		death.m_sWeapon = m_sLastPlayerWeapon;
		death.m_iRelation = m_iLastPlayerRelation;
		death.m_fDistance = m_fLastPlayerDistance;
		death.m_iDamageEventSequence = m_iLastPlayerDamageSequence;
		death.m_sCreditSource = "lastPlayerDamage";
		return m_LastAttacker;
	}

	//------------------------------------------------------------------------------------------------
	string GetDamageWeapon(int killerId, IEntity victim)
	{
		if (!victim || victim.GetID() != m_DamageCharacterId || killerId != m_iDamageInstigator)
			return string.Empty;
		return m_sDamageWeapon;
	}
}
