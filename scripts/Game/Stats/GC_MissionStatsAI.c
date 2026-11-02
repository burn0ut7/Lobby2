//! Match-local AI counters share collection, summary and delivery owners with player statistics.
//! These assignments are never inserted into the persistent mission batch.
class GC_MissionStatsAI
{
	protected ref map<string, ref GC_StatsAssignment> m_mGC_AIStats = new map<string, ref GC_StatsAssignment>();
	protected ref map<string, ref GC_StatsDebrief> m_mGC_AIReports = new map<string, ref GC_StatsDebrief>();
	protected bool m_bGC_AIPartial;

	//------------------------------------------------------------------------------------------------
	protected Faction AIFaction(IEntity character)
	{
		if (!character)
			return null;
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		if (affiliation)
			return affiliation.GetAffiliatedFaction();
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_StatsAssignment AIStats(IEntity character)
	{
		Faction faction = AIFaction(character);
		if (!faction)
			return null;
		string key = faction.GetFactionKey();
		GC_StatsAssignment stats = m_mGC_AIStats.Get(key);
		if (!stats)
		{
			if (m_mGC_AIStats.Count() >= 32)
			{
				m_bGC_AIPartial = true;
				return null;
			}
			stats = new GC_StatsAssignment();
			stats.m_sFaction = key;
			m_mGC_AIStats.Set(key, stats);
		}
		return stats;
	}

	//------------------------------------------------------------------------------------------------
	void RecordShot(GC_MissionStatsComponent owner, IEntity character, BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!owner.IsCollecting() || !character || owner.IsPlayerCharacter(character))
			return;
		GC_StatsAssignment stats = AIStats(character);
		if (!stats)
			return;
		stats.m_iShots++;
		if (!GC_StatsAmmunition.Record(owner, stats, weapon, projectile, false))
			m_bGC_AIPartial = true;
		owner.ObserveProjectile(weapon, projectile);
	}

	//------------------------------------------------------------------------------------------------
	//! Called once after the existing death deduplication and player-credit resolution.
	void RecordDeath(IEntity victim, IEntity killer, string victimKind, string killerKind)
	{
		GC_StatsAssignment stats;
		bool distinct = killer && killer != victim;
		if (victimKind == "ai")
		{
			stats = AIStats(victim);
			if (stats)
				stats.RecordDeath(distinct && killerKind == "player", distinct && killerKind == "ai");
		}
		if (killerKind != "ai" || !distinct || (victimKind != "player" && victimKind != "ai"))
			return;
		stats = AIStats(killer);
		if (!stats)
			return;
		stats.RecordKill(victimKind == "ai", false);
		Faction killerFaction = AIFaction(killer);
		Faction victimFaction = AIFaction(victim);
		if (killerFaction && victimFaction && killerFaction.IsFactionFriendly(victimFaction))
			stats.m_iTeamKills++;
	}

	//------------------------------------------------------------------------------------------------
	void BuildReports()
	{
		m_mGC_AIReports.Clear();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return;
		foreach (Faction faction : manager.GetRegisteredFactions())
		{
			string key = faction.GetFactionKey();
			GC_StatsAssignment stats = m_mGC_AIStats.Get(key);
			GC_StatsDebrief report = new GC_StatsDebrief();
			if (stats)
				report.AddAssignment(stats, "AI", true);
			report.Seal(false, true);
			m_mGC_AIReports.Set(key, report);
		}
	}

	//------------------------------------------------------------------------------------------------
	GC_StatsDebrief GetReport(string faction)
	{
		return m_mGC_AIReports.Get(faction);
	}

	//------------------------------------------------------------------------------------------------
	void MarkIncomplete()
	{
		m_bGC_AIPartial = true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsPartial()
	{
		return m_bGC_AIPartial;
	}
}
