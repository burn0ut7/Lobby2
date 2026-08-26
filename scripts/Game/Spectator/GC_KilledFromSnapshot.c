//! One captured skeletal bone used by a client-only visual reconstruction.
class GC_KilledFromBonePose
{
	string m_sBoneName;
	vector m_aTransform[4];
}

//! Facts required to recreate one killed-from presentation. Network-safe
//! primitive fields provide the authority fallback; visual entries and exact
//! bone poses exist only on the owning client.
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
	protected bool m_bHasLocalDamageCapture;
	protected ref array<ref SCR_BasePreviewEntry> m_aKillerPreviewEntries;
	protected vector m_aKillerRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aKillerBonePose;
	protected ref array<ref SCR_BasePreviewEntry> m_aVictimPreviewEntries;
	protected vector m_aVictimRootTransform[4];
	protected ref array<ref GC_KilledFromBonePose> m_aVictimBonePose;

	void Capture(IEntity victim, notnull BaseDamageContext damageContext)
	{
		m_bHasLocalDamageCapture = true;
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

		CaptureVictimVisual(victim);
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

		if (localDamage.m_bHasLocalDamageCapture)
		{
			m_bHasLocalDamageCapture = true;
			m_vLineOrigin = localDamage.m_vLineOrigin;
			m_vHitPosition = localDamage.m_vHitPosition;
		}
		m_aVictimPreviewEntries = localDamage.m_aVictimPreviewEntries;
		Math3D.MatrixCopy(localDamage.m_aVictimRootTransform, m_aVictimRootTransform);
		m_aVictimBonePose = localDamage.m_aVictimBonePose;
		if (!localDamage.HasKillerPreview())
			return;

		m_sKillerPrefab = localDamage.m_sKillerPrefab;
		m_vKillerPosition = localDamage.m_vKillerPosition;
		m_vKillerAngles = localDamage.m_vKillerAngles;
		m_iStance = localDamage.m_iStance;
		m_fDynamicStance = localDamage.m_fDynamicStance;
		m_iFlags = localDamage.m_iFlags;
		m_aKillerPreviewEntries = localDamage.m_aKillerPreviewEntries;
		Math3D.MatrixCopy(localDamage.m_aKillerRootTransform, m_aKillerRootTransform);
		m_aKillerBonePose = localDamage.m_aKillerBonePose;
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
		CaptureVisual(killer, m_aKillerPreviewEntries, m_aKillerRootTransform, m_aKillerBonePose);

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

	//! Refresh immediately before local possession leaves the dead character.
	//! This preserves the terminal death location and pose even when the retained
	//! attacker came from an earlier hit that caused unconsciousness or bleed-out.
	void CaptureVictimVisual(IEntity victim)
	{
		CaptureVisual(victim, m_aVictimPreviewEntries, m_aVictimRootTransform, m_aVictimBonePose);
	}

	protected void CaptureVisual(IEntity entity, out array<ref SCR_BasePreviewEntry> entries, out vector rootTransform[4], out array<ref GC_KilledFromBonePose> bonePose)
	{
		if (!entity)
			return;

		EntitySpawnParams spawnParams;
		entries = SCR_GenericPreviewEntity.GetPreviewEntriesFromEntity(entity, spawnParams);
		if (spawnParams)
			Math3D.MatrixCopy(spawnParams.Transform, rootTransform);

		bonePose = {};
		Animation animation = entity.GetAnimation();
		if (!animation)
			return;

		array<string> boneNames = {};
		animation.GetBoneNames(boneNames);
		foreach (string boneName : boneNames)
		{
			TNodeId boneIndex = animation.GetBoneIndex(boneName);
			GC_KilledFromBonePose pose = new GC_KilledFromBonePose();
			pose.m_sBoneName = boneName;
			if (!animation.GetBoneMatrix(boneIndex, pose.m_aTransform))
				continue;
			bonePose.Insert(pose);
		}
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
	bool HasKillerVisual() { return m_aKillerPreviewEntries && !m_aKillerPreviewEntries.IsEmpty(); }
	bool HasVictimVisual() { return m_aVictimPreviewEntries && !m_aVictimPreviewEntries.IsEmpty(); }
	array<ref SCR_BasePreviewEntry> GetKillerPreviewEntries() { return m_aKillerPreviewEntries; }
	array<ref SCR_BasePreviewEntry> GetVictimPreviewEntries() { return m_aVictimPreviewEntries; }
	array<ref GC_KilledFromBonePose> GetKillerBonePose() { return m_aKillerBonePose; }
	array<ref GC_KilledFromBonePose> GetVictimBonePose() { return m_aVictimBonePose; }
	void GetKillerRootTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aKillerRootTransform, transform); }
	void GetVictimRootTransform(out vector transform[4]) { Math3D.MatrixCopy(m_aVictimRootTransform, transform); }
	string GetDebugReport()
	{
		int killerBoneCount;
		if (m_aKillerBonePose)
			killerBoneCount = m_aKillerBonePose.Count();
		int victimBoneCount;
		if (m_aVictimBonePose)
			victimBoneCount = m_aVictimBonePose.Count();
		string report = string.Format("valid=%1 prefab=%2 killerPosition=%3 killerAngles=%4 lineOrigin=%5 hitPosition=%6",
			HasKillerPreview(), m_sKillerPrefab, m_vKillerPosition, m_vKillerAngles, m_vLineOrigin, m_vHitPosition);
		report = report + string.Format(" stance=%1 dynamicStance=%2 weaponRaised=%3 ads=%4 flags=%5 localDamage=%6", m_iStance, m_fDynamicStance,
			(m_iFlags & FLAG_WEAPON_RAISED) != 0, (m_iFlags & FLAG_WEAPON_ADS) != 0, m_iFlags, m_bHasLocalDamageCapture);
		return report + string.Format(" killerVisual=%1 killerBones=%2 victimVisual=%3 victimBones=%4", HasKillerVisual(), killerBoneCount, HasVictimVisual(), victimBoneCount);
	}
}

//! One owning-client adapter for the locally controlled playable entity. It
//! retains the latest damage plus the last direct external damage source.
class GC_KilledFromDamageTracker
{
	protected IEntity m_Victim;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected ref GC_KilledFromSnapshot m_Snapshot;
	protected ref GC_KilledFromSnapshot m_LastExternalSnapshot;

	bool Start(IEntity victim)
	{
		if (!victim)
		{
			Print("GC | Killed From | event=tracker-start result=rejected reason=no-victim", LogLevel.WARNING);
			return false;
		}

		m_DamageManager = SCR_DamageManagerComponent.Cast(victim.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
		{
			PrintFormat("GC | Killed From | event=tracker-start result=rejected reason=no-damage-manager victim=%1", victim, level: LogLevel.WARNING);
			return false;
		}

		m_Victim = victim;
		m_DamageManager.GetOnDamage().Insert(OnDamage);
		PrintFormat("GC | Killed From | event=tracker-start result=subscribed victim=%1 damageManager=%2", m_Victim, m_DamageManager);
		return true;
	}

	void Destroy()
	{
		GC_KilledFromSnapshot retainedSnapshot = GetSnapshot();
		if (retainedSnapshot)
			PrintFormat("GC | Killed From | event=tracker-stop victim=%1 retainedSnapshot={%2}", m_Victim, retainedSnapshot.GetDebugReport());
		else
			PrintFormat("GC | Killed From | event=tracker-stop victim=%1 retainedSnapshot=none", m_Victim);

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

	void CaptureTerminalVictim()
	{
		if (!m_Snapshot)
			m_Snapshot = new GC_KilledFromSnapshot();
		m_Snapshot.CaptureVictimVisual(m_Victim);
		if (m_LastExternalSnapshot && m_LastExternalSnapshot != m_Snapshot)
			m_LastExternalSnapshot.CaptureVictimVisual(m_Victim);
	}

	protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		if (damageContext.damageType == EDamageType.HEALING || damageContext.damageType == EDamageType.REGENERATION)
		{
			PrintFormat("GC | Killed From | event=damage result=ignored reason=non-damage type=%1 victim=%2", damageContext.damageType, m_Victim);
			return;
		}

		bool passiveDamage = damageContext.damageType == EDamageType.BLEEDING || damageContext.damageType == EDamageType.FIRE;
		if (passiveDamage && m_Snapshot)
		{
			PrintFormat("GC | Killed From | event=damage result=ignored reason=passive-after-capture type=%1 victim=%2 retained={%3}", damageContext.damageType, m_Victim, GetSnapshot().GetDebugReport());
			return;
		}

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

		PrintFormat("GC | Killed From | event=damage result=captured type=%1 victim=%2 victimPlayer=%3 damageSource=%4 instigator=%5 instigatorPlayer=%6 passive=%7 retainedAsExternal=%8 snapshot={%9}",
			damageContext.damageType, m_Victim, victimPlayerId, damageContext.damageSource, instigatorEntity, instigatorPlayerId, passiveDamage,
			m_LastExternalSnapshot == snapshot, snapshot.GetDebugReport());
	}
}
