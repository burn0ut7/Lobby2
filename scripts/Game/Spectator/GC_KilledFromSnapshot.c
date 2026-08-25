//! Server-captured facts required to recreate one killed-from presentation.
//! Base-game damage and character systems remain authoritative; this record is
//! retained only for the owning KIA player's spectator presentation.
class GC_KilledFromSnapshot
{
	static const int FLAG_VALID_KILLER = 1;
	static const int FLAG_WEAPON_RAISED = 2;
	static const int FLAG_WEAPON_ADS = 4;

	protected ResourceName m_sKillerPrefab;
	protected vector m_vKillerPosition;
	protected vector m_vKillerAngles;
	protected vector m_vLineOrigin;
	protected vector m_vHitPosition;
	protected int m_iStance;
	protected float m_fDynamicStance;
	protected int m_iFlags;

	void Capture(IEntity victim, notnull BaseDamageContext damageContext)
	{
		m_vHitPosition = damageContext.hitPosition;
		IEntity killer;
		if (damageContext.instigator)
			killer = damageContext.instigator.GetInstigatorEntity();

		IEntity damageSource = damageContext.damageSource;
		if (killer)
			CaptureKiller(killer);
		else if (victim)
			m_vLineOrigin = victim.GetOrigin();

		// For blast damage, the explosive's final position is more truthful than
		// the thrower's current position. Fragment projectiles retain their own
		// impact path, so they continue to point back to the instigator.
		if (damageSource && damageContext.damageType == EDamageType.EXPLOSIVE)
			m_vLineOrigin = damageSource.GetOrigin();
	}

	//! One-shot server fallback for killers that were not streamed to the owning
	//! client. It deliberately performs no continuous damage tracking.
	void CaptureDeathFallback(IEntity victim, IEntity killer)
	{
		if (victim)
		{
			m_vHitPosition = victim.GetOrigin();
			m_vLineOrigin = m_vHitPosition;
		}
		if (killer)
			CaptureKiller(killer);
	}

	//! Client damage facts carry the precise impact/explosive origin. Prefer its
	//! killer pose too when that entity existed on the owning client.
	void MergeLocalDamage(GC_KilledFromSnapshot localDamage)
	{
		if (!localDamage)
			return;

		m_vLineOrigin = localDamage.m_vLineOrigin;
		m_vHitPosition = localDamage.m_vHitPosition;
		if (!localDamage.HasKillerPreview())
			return;

		m_sKillerPrefab = localDamage.m_sKillerPrefab;
		m_vKillerPosition = localDamage.m_vKillerPosition;
		m_vKillerAngles = localDamage.m_vKillerAngles;
		m_iStance = localDamage.m_iStance;
		m_fDynamicStance = localDamage.m_fDynamicStance;
		m_iFlags = localDamage.m_iFlags;
	}

	protected void CaptureKiller(IEntity killer)
	{
		m_vKillerPosition = killer.GetOrigin();
		m_vKillerAngles = killer.GetAngles();
		m_vLineOrigin = m_vKillerPosition;

		EntityPrefabData prefabData = killer.GetPrefabData();
		if (prefabData)
			m_sKillerPrefab = prefabData.GetPrefabName();
		if (m_sKillerPrefab != string.Empty)
			m_iFlags |= FLAG_VALID_KILLER;

		ChimeraCharacter character = ChimeraCharacter.Cast(killer);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		m_iStance = controller.GetStance();
		m_fDynamicStance = controller.GetDynamicStance();
		if (controller.IsWeaponRaised())
			m_iFlags |= FLAG_WEAPON_RAISED;
		if (controller.IsWeaponADS())
			m_iFlags |= FLAG_WEAPON_ADS;
	}

	void SetNetworkData(ResourceName killerPrefab, vector killerPosition, vector killerAngles, vector lineOrigin, vector hitPosition, int stance, float dynamicStance, int flags)
	{
		m_sKillerPrefab = killerPrefab;
		m_vKillerPosition = killerPosition;
		m_vKillerAngles = killerAngles;
		m_vLineOrigin = lineOrigin;
		m_vHitPosition = hitPosition;
		m_iStance = stance;
		m_fDynamicStance = dynamicStance;
		m_iFlags = flags;
	}

	ResourceName GetKillerPrefab() { return m_sKillerPrefab; }
	vector GetKillerPosition() { return m_vKillerPosition; }
	vector GetKillerAngles() { return m_vKillerAngles; }
	vector GetLineOrigin() { return m_vLineOrigin; }
	vector GetHitPosition() { return m_vHitPosition; }
	int GetStance() { return m_iStance; }
	float GetDynamicStance() { return m_fDynamicStance; }
	int GetFlags() { return m_iFlags; }
	bool HasKillerPreview() { return (m_iFlags & FLAG_VALID_KILLER) != 0; }
}

//! One server-only adapter per playable slot entity. It retains only the most
//! recent damage context, which is the fatal context when destruction follows.
class GC_KilledFromDamageTracker
{
	protected IEntity m_Victim;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected ref GC_KilledFromSnapshot m_LastExternalSnapshot;

	bool Start(IEntity victim)
	{
		if (!victim)
			return false;

		m_DamageManager = SCR_DamageManagerComponent.Cast(victim.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
			return false;

		m_Victim = victim;
		m_DamageManager.GetOnDamage().Insert(OnDamage);
		return true;
	}

	void Destroy()
	{
		if (m_DamageManager)
			m_DamageManager.GetOnDamage().Remove(OnDamage);
		m_Victim = null;
		m_DamageManager = null;
		m_Snapshot = null;
		m_LastExternalSnapshot = null;
	}

	GC_KilledFromSnapshot GetSnapshot()
	{
		if (m_LastExternalSnapshot)
			return m_LastExternalSnapshot;
		return m_Snapshot;
	}

	protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		if (damageContext.damageType == EDamageType.HEALING || damageContext.damageType == EDamageType.REGENERATION)
			return;

		bool passiveDamage = damageContext.damageType == EDamageType.BLEEDING || damageContext.damageType == EDamageType.FIRE;
		if (passiveDamage && m_Snapshot)
			return;

		GC_KilledFromSnapshot snapshot = new GC_KilledFromSnapshot();
		snapshot.Capture(m_Victim, damageContext);
		m_Snapshot = snapshot;

		// A later bleed-out, unconscious respawn, or self-attributed damage tick
		// must not replace the player/AI/vehicle that directly caused the state.
		IEntity instigatorEntity;
		int instigatorPlayerId;
		if (damageContext.instigator)
		{
			instigatorEntity = damageContext.instigator.GetInstigatorEntity();
			instigatorPlayerId = damageContext.instigator.GetInstigatorPlayerID();
		}
		int victimPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_Victim);
		bool externalEntity = instigatorEntity && instigatorEntity != m_Victim;
		bool externalPlayer = instigatorPlayerId > 0 && instigatorPlayerId != victimPlayerId;
		if (!passiveDamage && (externalEntity || externalPlayer))
			m_LastExternalSnapshot = snapshot;
	}
}
