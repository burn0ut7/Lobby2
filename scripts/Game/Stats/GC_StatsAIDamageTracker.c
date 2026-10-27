//! Game-mode-owned subscription. Player lives retain their existing credit tracker.
class GC_StatsAIDamageTracker
{
	protected GC_MissionStatsComponent m_Stats;
	protected IEntity m_Character;
	protected SCR_DamageManagerComponent m_Damage;
	protected int m_iLastInstigator;
	protected float m_fLastImpact = -1;

	//------------------------------------------------------------------------------------------------
	void Start(GC_MissionStatsComponent stats, IEntity character)
	{
		m_Stats = stats;
		m_Character = character;
		m_Damage = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
		if (m_Damage)
			m_Damage.GetOnDamage().Insert(OnDamage);
	}

	//------------------------------------------------------------------------------------------------
	void Detach()
	{
		if (m_Damage)
			m_Damage.GetOnDamage().Remove(OnDamage);
		m_Damage = null;
		m_Character = null;
		m_Stats = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDamage(notnull BaseDamageContext context)
	{
		if (!m_Character || !m_Stats || !m_Stats.IsCollecting() || m_Stats.IsPlayerCharacter(m_Character)
			|| context.damageType == EDamageType.HEALING || context.damageType == EDamageType.REGENERATION)
			return;
		int instigatorId;
		if (context.instigator)
			instigatorId = context.instigator.GetInstigatorPlayerID();
		float now = GetGame().GetWorld().GetWorldTime();
		// Match the player's suppression of the engine's empty post-impact notification.
		if (context.damageType == EDamageType.KINETIC && context.instigator
			&& instigatorId == m_iLastInstigator && now == m_fLastImpact
			&& !context.damageSource && context.hitPosition.LengthSq() < 0.0001
			&& context.impactVelocity.LengthSq() < 0.0001)
			return;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
			logs.RecordDamage(context, m_Character, m_Stats);
		m_iLastInstigator = instigatorId;
		m_fLastImpact = now;
	}
}
