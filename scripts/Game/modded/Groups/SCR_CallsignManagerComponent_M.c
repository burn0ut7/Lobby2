//! Authored callsigns still use the native allocator, replication and release path.
modded class SCR_CallsignManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void AssignCallGroupCallsign(notnull SCR_AIGroup group, Faction faction, SCR_CallsignGroupComponent masterCallsignComponent, out int companyIndex, out int platoonIndex, out int squadIndex)
	{
		if (GC_LobbyGameMode.GetGameMode() && Replication.IsServer() && !masterCallsignComponent && group.GC_GetAuthoredCallsign(faction, companyIndex, platoonIndex, squadIndex))
		{
			SCR_FactionCallsignData data;
			if (m_mAvailableCallsigns.Find(faction, data) && data.GC_IsCallsignAvailable(companyIndex, platoonIndex, squadIndex))
			{
				RemoveAvailableGroupCallsign(faction, companyIndex, platoonIndex, squadIndex);
				return;
			}

			PrintFormat("GC | Call sign | requested call sign unavailable for %1: %2/%3/%4; using automatic assignment", group.GetName(), companyIndex, platoonIndex, squadIndex, level: LogLevel.WARNING);
		}

		super.AssignCallGroupCallsign(group, faction, masterCallsignComponent, companyIndex, platoonIndex, squadIndex);
	}
}

//! Read the existing availability tree rather than maintaining a second registry.
modded class SCR_FactionCallsignData
{
	bool GC_IsCallsignAvailable(int company, int platoon, int squad)
	{
		SCR_CallsignCompanyData data;
		if (company < m_iOverflowIndex)
			m_mCompanyCallsigns.Find(company, data);
		else
			m_mCompanyOverflowCallsigns.Find(company, data);
		return data && data.GC_IsCallsignAvailable(platoon, squad);
	}
}

modded class SCR_CallsignCompanyData
{
	bool GC_IsCallsignAvailable(int platoon, int squad)
	{
		SCR_CallsignPlatoonData data;
		return m_mPlatoonCallsigns.Find(platoon, data) && data && data.GC_IsCallsignAvailable(squad);
	}
}

modded class SCR_CallsignPlatoonData
{
	bool GC_IsCallsignAvailable(int squad)
	{
		return m_mSquadCallsigns.Contains(squad);
	}
}
