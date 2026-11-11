//! One runtime observation per resolved hit zone. Entity/hit-zone references are weak.
//! Native callbacks refresh the shared snapshot and immediately notify its owning framework.
class GC_EntityDamageObservation
{
	protected IEntity m_Entity;
	protected SCR_HitZone m_HitZone;
	protected bool m_bBound;
	protected bool m_bAvailable;
	protected bool m_bDestroyed;
	protected float m_fDamage;
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();
	protected ref array<GC_RuleCondition> m_aConditions = {};
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
		Refresh(false);
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
	protected void Refresh(bool notify = true)
	{
		if (!m_Entity || !m_HitZone)
			return;
		bool destroyed = m_HitZone.GetDamageState() == EDamageState.DESTROYED;
		float damage = Math.Clamp(1 - m_HitZone.GetHealthScaled(), 0, 1);
		bool changed = !m_bAvailable || destroyed != m_bDestroyed || damage != m_fDamage;
		m_bAvailable = true;
		m_bDestroyed = destroyed;
		m_fDamage = damage;
		if (notify && changed)
			m_OnChanged.Invoke(this);
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
	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	//------------------------------------------------------------------------------------------------
	SCR_HitZone GetHitZone()
	{
		return m_HitZone;
	}

	//------------------------------------------------------------------------------------------------
	void AddCondition(GC_RuleCondition check)
	{
		if (!m_aConditions.Contains(check))
			m_aConditions.Insert(check);
	}

	//------------------------------------------------------------------------------------------------
	array<GC_RuleCondition> GetConditions()
	{
		return m_aConditions;
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
		m_aConditions.Clear();
	}
}
