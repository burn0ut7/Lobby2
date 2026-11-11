//! Explicit local fixtures; production contains no runtime naming setter.
#ifdef WORKBENCH
modded class SCR_AIGroup
{
	string GC_TestExchangeManualCallsign(string name)
	{
		string previous = m_sGC_ManualCallsign;
		m_sGC_ManualCallsign = name;
		return previous;
	}

	void GC_TestSetCallsign(int company, int platoon, int squad, string name)
	{
		m_iGC_CallsignCompany = company;
		m_iGC_CallsignPlatoon = platoon;
		m_iGC_CallsignSquad = squad;
		m_sGC_ManualCallsign = name;
	}

	string GC_TestCallsignPolicy()
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return "UNAVAILABLE authority/mode";
		SCR_CallsignGroupComponent component = SCR_CallsignGroupComponent.Cast(FindComponent(SCR_CallsignGroupComponent));
		SCR_CallsignManagerComponent manager = SCR_CallsignManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_CallsignManagerComponent));
		int oldCompany, oldPlatoon, oldSquad, characterIndex;
		if (!component || !manager || !component.GetCallsignIndexes(oldCompany, oldPlatoon, oldSquad, characterIndex))
			return "UNAVAILABLE native callsign";

		int authoredCompany = m_iGC_CallsignCompany;
		int authoredPlatoon = m_iGC_CallsignPlatoon;
		int authoredSquad = m_iGC_CallsignSquad;
		string authoredName = m_sGC_ManualCallsign;
		string originalDisplay = GC_GroupCallsignHelper.GetDisplayName(this);
		int company, platoon, squad;
		string result;

		GC_TestSetCallsign(oldCompany, oldPlatoon, oldSquad, "  Viper One  ");
		bool valid = GC_GetAuthoredCallsign(GetFaction(), company, platoon, squad);
		result += string.Format("validIndices=%1 ", valid && company == oldCompany && platoon == oldPlatoon && squad == oldSquad);
		string companyName, platoonName, squadName, characterName, format;
		component.GetCallsignNames(companyName, platoonName, squadName, characterName, format);
		result += string.Format("manualLobby=%1 manualNative=%2 manualCustom=%3 ", GC_GroupCallsignHelper.GetDisplayName(this) == "Viper One", WidgetManager.Translate(format, companyName, platoonName, squadName, characterName) == "Viper One", GetCustomNameWithOriginal() == "Viper One");

		GC_TestSetCallsign(oldCompany, oldPlatoon, oldSquad, "   ");
		result += string.Format("blankIgnored=%1 ", GC_GetManualCallsign().IsEmpty());
		manager.MakeGroupCallsignAvailible(GetFaction(), oldCompany, oldPlatoon, oldSquad);
		manager.AssignCallGroupCallsign(this, GetFaction(), null, company, platoon, squad);
		component.DoAssignCallsign(company, platoon, squad);
		result += string.Format("nativeAssignment=%1 ", company == oldCompany && platoon == oldPlatoon && squad == oldSquad);

		// A second request cannot steal the tuple we just reserved.
		manager.AssignCallGroupCallsign(this, GetFaction(), null, company, platoon, squad);
		result += string.Format("occupiedRejected=%1 ", company != oldCompany || platoon != oldPlatoon || squad != oldSquad);
		manager.MakeGroupCallsignAvailible(GetFaction(), company, platoon, squad);

		GC_TestSetCallsign(999999, 0, 0, "Override");
		result += string.Format("invalidRejected=%1 manualOverInvalid=%2 ", !GC_GetAuthoredCallsign(GetFaction(), company, platoon, squad), GC_GroupCallsignHelper.GetDisplayName(this) == "Override");
		GC_TestSetCallsign(-1, 0, 0, "");
		result += string.Format("automatic=%1 ", !GC_GetAuthoredCallsign(GetFaction(), company, platoon, squad));

		GC_TestSetCallsign(authoredCompany, authoredPlatoon, authoredSquad, authoredName);
		component.GetCallsignIndexes(company, platoon, squad, characterIndex);
		result += string.Format("restored=%1 group=%2 display=%3", company == oldCompany && platoon == oldPlatoon && squad == oldSquad && GC_GroupCallsignHelper.GetDisplayName(this) == originalDisplay, GetName(), originalDisplay);
		return result;
	}
}

modded class GC_LobbyManager
{
	protected SCR_AIGroup m_GC_TestCallsignPreviewGroup;
	protected string m_sGC_TestCallsignPreviewOriginal;

	string GC_TestGroupCallsignPreview(bool enable)
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return "UNAVAILABLE authority/mode";
		if (enable && !m_GC_TestCallsignPreviewGroup)
		{
			foreach (SCR_AIGroup group : GetRegisteredGroups())
			{
				if (!group)
					continue;
				m_GC_TestCallsignPreviewGroup = group;
				m_sGC_TestCallsignPreviewOriginal = group.GC_TestExchangeManualCallsign("Viper One");
				break;
			}
		}
		if (!m_GC_TestCallsignPreviewGroup)
			return "UNAVAILABLE preview";
		SCR_AIGroup target = m_GC_TestCallsignPreviewGroup;
		if (!enable)
		{
			target.GC_TestExchangeManualCallsign(m_sGC_TestCallsignPreviewOriginal);
			m_GC_TestCallsignPreviewGroup = null;
		}
		m_OnFactionPresentationChanged.Invoke(target.GetFaction());
		return GC_GroupCallsignHelper.GetDisplayName(target);
	}

	string GC_TestGroupCallsigns()
	{
		foreach (SCR_AIGroup group : GetRegisteredGroups())
		{
			if (group && !group.IsSlave())
				return group.GC_TestCallsignPolicy();
		}
		return "UNAVAILABLE group";
	}
}
#endif
