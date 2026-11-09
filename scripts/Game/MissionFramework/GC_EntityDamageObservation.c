//! One runtime observation per condition. Entity/hit-zone references are weak.
//! Callbacks only mark changes; the authoritative framework processes them on its existing tick.
class GC_EntityDamageObservation
{
	protected IEntity m_Entity;
	protected SCR_HitZone m_HitZone;
	protected bool m_bBound;
	protected bool m_bAvailable;
	protected bool m_bDestroyed;
	protected float m_fDamage;
	protected bool m_bChanged;
	protected bool m_bReportedMissing;

	//------------------------------------------------------------------------------------------------
	void Bind(BaseWorld world, GC_RuleCondition condition)
	{
		if (m_bBound || !world)
			return;
		IEntity entity = world.FindEntityByName(condition.m_sDamageEntity);
		SCR_DamageManagerComponent manager;
		if (entity)
			manager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		HitZone zone;
		if (manager)
		{
			if (condition.m_sDamageArea.IsEmpty())
				zone = manager.GetDefaultHitZone();
			else
				zone = manager.GetHitZoneByName(condition.m_sDamageArea);
		}
		SCR_HitZone scriptedZone = SCR_HitZone.Cast(zone);
		if (!scriptedZone)
		{
			if (!m_bReportedMissing)
				Print("GC | Mission Framework | Entity Damage unavailable: '" + condition.m_sDamageEntity + "', area '" + condition.m_sDamageArea + "'. Check the entity name and existing damage area. Will retry at Match start.", LogLevel.WARNING);
			m_bReportedMissing = true;
			return;
		}
		m_Entity = entity;
		m_HitZone = scriptedZone;
		m_bBound = true;
		m_HitZone.GetOnHealthChanged().Insert(OnHealthChanged);
		m_HitZone.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnHealthChanged()
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDamageStateChanged(SCR_HitZone zone)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_Entity || !m_HitZone)
			return;
		m_bAvailable = true;
		m_bDestroyed = m_HitZone.GetDamageState() == EDamageState.DESTROYED;
		m_fDamage = Math.Clamp(1 - m_HitZone.GetHealthScaled(), 0, 1);
		m_bChanged = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Deletion is not destruction. Retain a confirmed destroyed result after wreck cleanup only.
	bool IsAvailable()
	{
		return m_bAvailable && ((m_Entity && m_HitZone) || m_bDestroyed);
	}

	//------------------------------------------------------------------------------------------------
	bool Passes(int kind, float damageNeeded)
	{
		return Matches(IsAvailable(), m_bDestroyed, m_fDamage, kind, damageNeeded);
	}

	//------------------------------------------------------------------------------------------------
	static bool Matches(bool available, bool destroyed, float damage, int kind, float damageNeeded)
	{
		if (!available)
			return false;
		if (kind == 0)
			return destroyed;
		if (kind == 1)
			return !destroyed;
		return kind == 2 && damageNeeded >= 0 && damageNeeded <= 1 && damage >= damageNeeded;
	}

	//------------------------------------------------------------------------------------------------
	bool ConsumeChange()
	{
		if (m_bAvailable && !IsAvailable())
		{
			m_bAvailable = false;
			m_bChanged = true;
		}
		bool changed = m_bChanged;
		m_bChanged = false;
		return changed;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (m_HitZone)
		{
			m_HitZone.GetOnHealthChanged().Remove(OnHealthChanged);
			m_HitZone.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
		}
		m_Entity = null;
		m_HitZone = null;
		m_bAvailable = false;
		m_bBound = false;
		m_bDestroyed = false;
		m_bChanged = false;
	}
}
