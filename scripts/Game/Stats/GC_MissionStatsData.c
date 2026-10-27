//! Stable wire names below are the external history contract, independent of script field names.
class GC_StatsRole
{
	string m_sPrefab;
	string m_sName;
	int m_iSlotId;
	int m_iSelections;
	float m_fSeconds;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("prefab", m_sPrefab);
		context.WriteValue("name", m_sName);
		context.WriteValue("slotId", m_iSlotId);
		context.WriteValue("selections", m_iSelections);
		context.WriteValue("seconds", m_fSeconds);
	}
}

class GC_StatsCounters
{
	float m_fPlaySeconds;
	float m_fWalkMetres;
	float m_fDriveMetres;
	float m_fPilotMetres;
	float m_fRadioSecondsReported;
	int m_iShots;
	int m_iGrenades;
	int m_iBandagesOthers;
	int m_iKills;
	int m_iAIKills;
	int m_iDeathsByPlayers;
	int m_iDeathsByAI;
	int m_iTeamKills;
	int m_iDeaths;
	int m_iCommanderMissions;
	int m_iLeaderMissions;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("playSeconds", m_fPlaySeconds);
		context.WriteValue("walkMetres", m_fWalkMetres);
		context.WriteValue("driveMetres", m_fDriveMetres);
		context.WriteValue("pilotMetres", m_fPilotMetres);
		context.WriteValue("radioSecondsClientReported", m_fRadioSecondsReported);
		context.WriteValue("shots", m_iShots);
		context.WriteValue("grenades", m_iGrenades);
		context.WriteValue("bandagesOthers", m_iBandagesOthers);
		context.WriteValue("kills", m_iKills);
		context.WriteValue("aiKills", m_iAIKills);
		context.WriteValue("deathsByPlayers", m_iDeathsByPlayers);
		context.WriteValue("deathsByAI", m_iDeathsByAI);
		context.WriteValue("teamKills", m_iTeamKills);
		context.WriteValue("deaths", m_iDeaths);
		context.WriteValue("commanderMissions", m_iCommanderMissions);
		context.WriteValue("leaderMissions", m_iLeaderMissions);
	}

	//------------------------------------------------------------------------------------------------
	void Add(GC_StatsCounters other, int sign = 1)
	{
		m_fPlaySeconds += other.m_fPlaySeconds * sign;
		m_fWalkMetres += other.m_fWalkMetres * sign;
		m_fDriveMetres += other.m_fDriveMetres * sign;
		m_fPilotMetres += other.m_fPilotMetres * sign;
		m_fRadioSecondsReported += other.m_fRadioSecondsReported * sign;
		m_iShots += other.m_iShots * sign;
		m_iGrenades += other.m_iGrenades * sign;
		m_iBandagesOthers += other.m_iBandagesOthers * sign;
		m_iKills += other.m_iKills * sign;
		m_iAIKills += other.m_iAIKills * sign;
		m_iDeathsByPlayers += other.m_iDeathsByPlayers * sign;
		m_iDeathsByAI += other.m_iDeathsByAI * sign;
		m_iTeamKills += other.m_iTeamKills * sign;
		m_iDeaths += other.m_iDeaths * sign;
	}

	//------------------------------------------------------------------------------------------------
	void SetDifference(GC_StatsCounters total, GC_StatsCounters start)
	{
		m_fPlaySeconds = total.m_fPlaySeconds - start.m_fPlaySeconds;
		m_fWalkMetres = total.m_fWalkMetres - start.m_fWalkMetres;
		m_fDriveMetres = total.m_fDriveMetres - start.m_fDriveMetres;
		m_fPilotMetres = total.m_fPilotMetres - start.m_fPilotMetres;
		// Radio is apportioned over its reporting interval, independently of counter differences.
		m_iShots = total.m_iShots - start.m_iShots;
		m_iGrenades = total.m_iGrenades - start.m_iGrenades;
		m_iBandagesOthers = total.m_iBandagesOthers - start.m_iBandagesOthers;
		m_iKills = total.m_iKills - start.m_iKills;
		m_iAIKills = total.m_iAIKills - start.m_iAIKills;
		m_iDeathsByPlayers = total.m_iDeathsByPlayers - start.m_iDeathsByPlayers;
		m_iDeathsByAI = total.m_iDeathsByAI - start.m_iDeathsByAI;
		m_iTeamKills = total.m_iTeamKills - start.m_iTeamKills;
		m_iDeaths = total.m_iDeaths - start.m_iDeaths;
	}

	//------------------------------------------------------------------------------------------------
	void RecordKill(bool ai, bool teamKill)
	{
		if (ai)
			m_iAIKills++;
		else
		{
			m_iKills++;
			if (teamKill)
				m_iTeamKills++;
		}
	}

	//------------------------------------------------------------------------------------------------
	void RecordDeath(bool byPlayer, bool byAI)
	{
		m_iDeaths++;
		if (byPlayer)
			m_iDeathsByPlayers++;
		else if (byAI)
			m_iDeathsByAI++;
	}
}

class GC_StatsAmmo
{
	string m_sProjectile;
	string m_sMagazine;
	string m_sWeapon;
	int m_iRounds;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("projectile", m_sProjectile);
		context.WriteValue("magazine", m_sMagazine);
		context.WriteValue("weapon", m_sWeapon);
		context.WriteValue("rounds", m_iRounds);
	}
}

//! Membership intervals reconstruct squadmates by overlapping mission time and groupId.
class GC_StatsAssignment : GC_StatsCounters
{
	string m_sFaction;
	string m_sGroup;
	string m_sGroupName;
	string m_sLeader;
	string m_sRole;
	int m_iSlotId = -1;
	float m_fFrom;
	float m_fUntil;
	ref array<ref GC_StatsAmmo> m_aAmmo = {};
	//! Starting cumulative counters are bookkeeping, not serialized.
	ref GC_StatsCounters m_Start;

	//------------------------------------------------------------------------------------------------
	override void Write(SaveContext context)
	{
		super.Write(context);
		context.WriteValue("faction", m_sFaction);
		context.WriteValue("groupId", m_sGroup);
		context.WriteValue("groupName", m_sGroupName);
		context.WriteValue("leader", m_sLeader);
		context.WriteValue("role", m_sRole);
		context.WriteValue("slotId", m_iSlotId);
		context.WriteValue("fromSeconds", m_fFrom);
		context.WriteValue("untilSeconds", m_fUntil);
		int ammoCount = m_aAmmo.Count();
		context.StartArray("ammo", ammoCount);
		foreach (GC_StatsAmmo ammo : m_aAmmo)
		{
			context.StartObject();
			ammo.Write(context);
			context.EndObject();
		}
		context.EndArray();
	}
}

class GC_StatsPlayer : GC_StatsCounters
{
	string m_sIdentity;
	string m_sName;
	//! Session bookkeeping is deliberately omitted from the wire contract.
	int m_iLastSlotId = -1;
	ref array<ref GC_StatsRole> m_aRoles = {};
	ref array<ref GC_StatsAssignment> m_aAssignments = {};
	GC_StatsAssignment m_ActiveAssignment;

	//------------------------------------------------------------------------------------------------
	void SyncAssignment(float until)
	{
		if (!m_ActiveAssignment)
			return;
		m_ActiveAssignment.SetDifference(this, m_ActiveAssignment.m_Start);
		m_ActiveAssignment.m_fUntil = Math.Max(m_ActiveAssignment.m_fUntil, until);
	}

	//------------------------------------------------------------------------------------------------
	override void Write(SaveContext context)
	{
		super.Write(context);
		context.WriteValue("identity", m_sIdentity);
		context.WriteValue("name", m_sName);
		int assignmentsCount = m_aAssignments.Count();
		context.StartArray("assignments", assignmentsCount);
		foreach (GC_StatsAssignment assignment : m_aAssignments)
		{
			context.StartObject();
			assignment.Write(context);
			context.EndObject();
		}
		context.EndArray();
		int rolesCount = m_aRoles.Count();
		context.StartArray("roles", rolesCount);
		foreach (GC_StatsRole role : m_aRoles)
		{
			context.StartObject();
			role.Write(context);
			context.EndObject();
		}
		context.EndArray();
	}
}

//! One death is one event shared by the victim and killer histories.
class GC_StatsDeath
{
	string m_sVictimKind = "other";
	string m_sKillerKind = "other";
	string m_sVictimPrefab;
	string m_sKillerPrefab;
	string m_sCause = "other";
	string m_sCreditSource;
	int m_iDamageEventSequence;
	int m_iSequence;
	float m_fMissionSeconds;
	string m_sVictim;
	string m_sKiller;
	string m_sVictimName;
	string m_sKillerName;
	int m_iRelation;
	string m_sWeapon;
	float m_fDistance = -1;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("sequence", m_iSequence);
		context.WriteValue("victimKind", m_sVictimKind);
		context.WriteValue("killerKind", m_sKillerKind);
		context.WriteValue("victimPrefab", m_sVictimPrefab);
		context.WriteValue("killerPrefab", m_sKillerPrefab);
		context.WriteValue("missionSeconds", m_fMissionSeconds);
		context.WriteValue("deathCause", m_sCause);
		context.WriteValue("creditSource", m_sCreditSource);
		context.WriteValue("damageEventSequence", m_iDamageEventSequence);
		context.WriteValue("victim", m_sVictim);
		context.WriteValue("killer", m_sKiller);
		context.WriteValue("victimName", m_sVictimName);
		context.WriteValue("killerName", m_sKillerName);
		context.WriteValue("relation", m_iRelation);
		context.WriteValue("weapon", m_sWeapon);
		context.WriteValue("distanceMetres", m_fDistance);
	}
}

//! Cumulative player snapshots plus append-only death events. Never add snapshots together.
class GC_MissionStatsBatch
{
	string m_sMissionId;
	string m_sWorld;
	int m_iStartedAt;
	int m_iRevision;
	bool m_bFinal;
	bool m_bComplete = true;
	float m_fDuration;
	ref array<ref GC_StatsPlayer> m_aPlayers = {};
	ref array<ref GC_StatsDeath> m_aDeaths = {};

	//------------------------------------------------------------------------------------------------
	//! Local serialization context avoids backend JsonApiStruct payload limits.
	string Encode()
	{
		JsonSaveContext context = new JsonSaveContext();
		Write(context);
		if (!context.IsValid())
			return string.Empty;
		string payload = context.SaveToString();
		if (payload.Length() <= 2)
			return string.Empty;
		return payload;
	}

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("schemaVersion", 2);
		context.WriteValue("missionId", m_sMissionId);
		context.WriteValue("world", m_sWorld);
		context.WriteValue("startedAt", m_iStartedAt);
		context.WriteValue("revision", m_iRevision);
		context.WriteValue("final", m_bFinal);
		context.WriteValue("complete", m_bComplete);
		context.WriteValue("durationSeconds", m_fDuration);
		context.WriteValue("minimumPlayersForPlaytime", 5);
		int playersCount = m_aPlayers.Count();
		context.StartArray("players", playersCount);
		foreach (GC_StatsPlayer player : m_aPlayers)
		{
			context.StartObject();
			player.Write(context);
			context.EndObject();
		}
		context.EndArray();
		int deathsCount = m_aDeaths.Count();
		context.StartArray("deaths", deathsCount);
		foreach (GC_StatsDeath death : m_aDeaths)
		{
			context.StartObject();
			death.Write(context);
			context.EndObject();
		}
		context.EndArray();
	}
}
