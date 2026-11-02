//! Match-local AI counters share collection, summary and delivery owners with player statistics.
//! These assignments are never inserted into the persistent mission batch.
class GC_MissionStatsAI
{
	protected ref map<string, ref GC_StatsAssignment> m_mGC_AIStats = new map<string, ref GC_StatsAssignment>();
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
		return AIStatsForFaction(faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	protected GC_StatsAssignment AIStatsForFaction(string key)
	{
		if (key.IsEmpty())
			return null;
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
	void RecordDeath(IEntity victim, IEntity killer, string victimKind, string killerKind, GC_SessionEvent creditedHit = null)
	{
		GC_StatsAssignment stats;
		bool distinct = creditedHit || (killer && killer != victim);
		if (victimKind == "ai")
		{
			stats = AIStats(victim);
			if (stats)
				stats.RecordDeath(distinct && killerKind == "player", distinct && killerKind == "ai");
		}
		if (killerKind != "ai" || !distinct || (victimKind != "player" && victimKind != "ai"))
			return;
		if (creditedHit)
			stats = AIStatsForFaction(creditedHit.m_sActorFaction);
		else
			stats = AIStats(killer);
		if (!stats)
			return;
		stats.RecordKill(victimKind == "ai", false);
		if (creditedHit)
		{
			if (creditedHit.m_bFriendly)
				stats.m_iTeamKills++;
			return;
		}
		Faction killerFaction = AIFaction(killer);
		Faction victimFaction = AIFaction(victim);
		if (killerFaction && victimFaction && killerFaction.IsFactionFriendly(victimFaction))
			stats.m_iTeamKills++;
	}

	//------------------------------------------------------------------------------------------------
	void AddToReports(GC_StatsDebrief all, map<string, ref GC_StatsDebrief> factions)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			foreach (Faction registered : manager.GetRegisteredFactions())
			{
				if (!factions.Contains(registered.GetFactionKey()))
					factions.Set(registered.GetFactionKey(), new GC_StatsDebrief());
			}
		}
		foreach (string key, GC_StatsAssignment totals : m_mGC_AIStats)
		{
			GC_StatsDebrief faction = factions.Get(key);
			if (!faction)
			{
				faction = new GC_StatsDebrief();
				factions.Set(key, faction);
			}
			// AI contributes combat totals and ammunition, never a fake player identity.
			faction.AddAssignment(totals, "", true, false);
			all.AddAssignment(totals, "", true, false);
		}
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
