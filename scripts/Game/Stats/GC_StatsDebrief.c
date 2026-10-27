//! Immutable, bounded presentation summaries built once at mission end.
class GC_StatsDebrief
{
	protected ref GC_StatsCounters m_Counters = new GC_StatsCounters();
	protected ref map<string, int> m_mAmmo = new map<string, int>();
	protected ref set<string> m_Participants = new set<string>();
	protected string m_sSummary;
	protected string m_sAmmunition;

	//------------------------------------------------------------------------------------------------
	void AddAssignment(GC_StatsAssignment assignment, string identity, bool includeCounters)
	{
		if (includeCounters)
			m_Counters.Add(assignment);
		m_Participants.Insert(identity);
		foreach (GC_StatsAmmo ammo : assignment.m_aAmmo)
			m_mAmmo.Set(ammo.m_sProjectile, m_mAmmo.Get(ammo.m_sProjectile) + ammo.m_iRounds);
	}

	//------------------------------------------------------------------------------------------------
	void UsePersonalCounters(GC_StatsPlayer player)
	{
		m_Counters.Add(player);
		m_Counters.m_iCommanderMissions = player.m_iCommanderMissions;
		m_Counters.m_iLeaderMissions = player.m_iLeaderMissions;
	}

	//------------------------------------------------------------------------------------------------
	void Seal(bool personal)
	{
		m_sSummary = FormatTime(m_Counters.m_fPlaySeconds);
		m_sSummary += "\n" + m_Counters.m_iShots.ToString();
		m_sSummary += "\n" + m_Counters.m_iGrenades.ToString();
		m_sSummary += "\n" + m_Counters.m_iBandagesOthers.ToString();
		m_sSummary += "\n" + m_Counters.m_iKills.ToString();
		m_sSummary += "\n" + m_Counters.m_iAIKills.ToString();
		m_sSummary += "\n" + m_Counters.m_iTeamKills.ToString();
		m_sSummary += "\n" + m_Counters.m_iDeaths.ToString();
		m_sSummary += "\n" + m_Counters.m_iDeathsByPlayers.ToString();
		m_sSummary += "\n" + m_Counters.m_iDeathsByAI.ToString();
		int otherDeaths = m_Counters.m_iDeaths - m_Counters.m_iDeathsByPlayers - m_Counters.m_iDeathsByAI;
		m_sSummary += "\n" + otherDeaths.ToString();
		m_sSummary += string.Format("\n%1 m", Math.Round(m_Counters.m_fWalkMetres));
		m_sSummary += string.Format("\n%1 m / %2 m", Math.Round(m_Counters.m_fDriveMetres), Math.Round(m_Counters.m_fPilotMetres));
		m_sSummary += "\n" + FormatTime(m_Counters.m_fRadioSecondsReported);
		string last = m_Participants.Count().ToString();
		if (personal)
		{
			last = "None";
			if (m_Counters.m_iCommanderMissions > 0)
				last = "CO";
			if (m_Counters.m_iLeaderMissions > 0)
			{
				if (last == "None")
					last = "Squad leader";
				else
					last += " / Squad leader";
			}
		}
		m_sSummary += "\n" + last;
		array<string> types = {};
		foreach (string projectile, int rounds : m_mAmmo)
			types.Insert(projectile);
		types.Sort();
		int displayed;
		foreach (string type : types)
		{
			if (displayed >= 32)
				break;
			array<string> parts = {};
			type.Split("/", parts, false);
			string label = "Unknown ammunition";
			if (!parts.IsEmpty())
			{
				label = parts[parts.Count() - 1];
				label.Replace(".et", "");
				label.Replace("Bullet_", "");
				label.Replace("Ammo_", "");
				label.Replace("_", " ");
				if (label.Length() > 80)
					label = label.Substring(0, 77) + "...";
			}
			m_sAmmunition += string.Format("%1: %2\n", label, m_mAmmo.Get(type));
			displayed++;
		}
		if (types.Count() > displayed)
			m_sAmmunition += string.Format("%1 more ammunition types saved in mission history.", types.Count() - displayed);
		if (m_sAmmunition.IsEmpty())
			m_sAmmunition = "No ammunition fired.";
		// No identities or mutable collections are retained in the presentation cache.
		m_Participants.Clear();
		m_mAmmo.Clear();
	}

	//------------------------------------------------------------------------------------------------
	string GetSummary()
	{
		return m_sSummary;
	}

	//------------------------------------------------------------------------------------------------
	string GetAmmunition()
	{
		return m_sAmmunition;
	}

	//------------------------------------------------------------------------------------------------
	protected string FormatTime(float seconds)
	{
		int whole = Math.Floor(seconds);
		return string.Format("%1m %2s", whole / 60, whole % 60);
	}
}
