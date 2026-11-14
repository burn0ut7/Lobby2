//! Authored callsigns still use the native allocator, replication and release path.
modded class SCR_CallsignManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void AssignCallGroupCallsign(notnull SCR_AIGroup group, Faction faction, SCR_CallsignGroupComponent masterCallsignComponent, out int companyIndex, out int platoonIndex, out int squadIndex)
	{
		if (GC_LobbyGameMode.GetGameMode() && Replication.IsServer() && !masterCallsignComponent && group.GC_GetAuthoredCallsign(faction, companyIndex, platoonIndex, squadIndex))
		{
			SCR_FactionCallsignData data;
			if (m_mAvailableCallsigns.Find(faction, data) && data.GC_FindAuthoredCallsign(group, companyIndex, platoonIndex, squadIndex))
			{
				RemoveAvailableGroupCallsign(faction, companyIndex, platoonIndex, squadIndex);
				return;
			}

			PrintFormat("GC | Call sign | requested call sign unavailable for %1: %2/%3/%4; using automatic assignment", group.GetName(), companyIndex, platoonIndex, squadIndex, level: LogLevel.WARNING);
		}

		super.AssignCallGroupCallsign(group, faction, masterCallsignComponent, companyIndex, platoonIndex, squadIndex);
	}
}

//! Query the native availability tree; allocation/release remain with the manager.
//! The native API cannot constrain individual levels while leaving others automatic.
modded class SCR_FactionCallsignData
{
	bool GC_FindAuthoredCallsign(SCR_AIGroup group, inout int company, inout int platoon, inout int squad)
	{
		if (company >= 0)
		{
			SCR_CallsignCompanyData data;
			if (company < m_iOverflowIndex)
				m_mCompanyCallsigns.Find(company, data);
			else
				m_mCompanyOverflowCallsigns.Find(company, data);
			return data && data.GC_FindAuthoredCallsign(platoon, squad, m_bRandomizedCallsigns);
		}

		SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
		SCR_FactionCallsignInfo info;
		if (faction)
			info = faction.GetCallsignInfo();
		bool roleBased = info && !m_bRandomizedCallsigns && info.GetAssignCallsignBasedOnGroupRole();
		if (roleBased)
		{
			array<int> roleCompanies = {};
			info.GetGroupRoleSpecificCompanies(group.GetGroupRole(), roleCompanies);
			foreach (int roleCompany : roleCompanies)
			{
				SCR_CallsignCompanyData roleData = m_mCompanyCallsigns.Get(roleCompany);
				if (roleData && roleData.GC_FindAuthoredCallsign(platoon, squad, false))
				{
					company = roleCompany;
					return true;
				}
			}
		}

		// Native role assignment falls back to random when no role company fits.
		bool randomized = m_bRandomizedCallsigns || roleBased;
		if (GC_FindAuthoredCompany(m_mCompanyCallsigns, company, platoon, squad, randomized))
			return true;
		return GC_FindAuthoredCompany(m_mCompanyOverflowCallsigns, company, platoon, squad, randomized);
	}

	protected bool GC_FindAuthoredCompany(map<int, ref SCR_CallsignCompanyData> companies, out int company, inout int platoon, inout int squad, bool randomized)
	{
		array<int> eligible = {};
		foreach (int index, SCR_CallsignCompanyData data : companies)
		{
			if (data && data.GC_HasAuthoredCallsign(platoon, squad))
				eligible.Insert(index);
		}
		if (eligible.IsEmpty())
			return false;
		eligible.Sort();
		int choice = 0;
		if (randomized)
			choice = Math.RandomInt(0, eligible.Count());
		company = eligible[choice];
		return companies.Get(company).GC_FindAuthoredCallsign(platoon, squad, randomized);
	}
}

modded class SCR_CallsignCompanyData
{
	bool GC_HasAuthoredCallsign(int platoon, int squad)
	{
		foreach (int index, SCR_CallsignPlatoonData data : m_mPlatoonCallsigns)
		{
			if ((platoon < 0 || index == platoon) && data && data.GC_HasAuthoredSquad(squad))
				return true;
		}
		return false;
	}

	bool GC_FindAuthoredCallsign(inout int platoon, inout int squad, bool randomized)
	{
		array<int> eligible = {};
		foreach (int index, SCR_CallsignPlatoonData data : m_mPlatoonCallsigns)
		{
			if ((platoon < 0 || index == platoon) && data && data.GC_HasAuthoredSquad(squad))
				eligible.Insert(index);
		}
		if (eligible.IsEmpty())
			return false;
		eligible.Sort();
		int choice = 0;
		if (randomized)
			choice = Math.RandomInt(0, eligible.Count());
		platoon = eligible[choice];
		if (squad < 0)
		{
			if (randomized)
				squad = m_mPlatoonCallsigns.Get(platoon).GetRandomSquad();
			else
				squad = m_mPlatoonCallsigns.Get(platoon).GetFirstAvailibleSquad();
		}
		return true;
	}
}

modded class SCR_CallsignPlatoonData
{
	bool GC_HasAuthoredSquad(int squad)
	{
		return (squad < 0 && !m_mSquadCallsigns.IsEmpty()) || m_mSquadCallsigns.Contains(squad);
	}
}
