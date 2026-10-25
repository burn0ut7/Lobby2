//! Optional fixture configuration; production evaluators deliver the labels to the HUD.
modded class GC_AOLimitEntity
{
	//------------------------------------------------------------------------------------------------
	void GC_TestWarningText(string text)
	{
		m_sWarningText = text;
	}
}

modded class GC_SetupZoneEntity
{
	//------------------------------------------------------------------------------------------------
	void GC_TestDisplayText(string text)
	{
		m_sDisplayText = text;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestBoundaryLabels(int index, string warningText, string setupText)
	{
		GC_AOLimitEntity limit = GC_AOLimitEntity.Cast(GetGame().GetWorld().FindEntityByName("AOFixture" + index));
		GC_SetupZoneEntity setup = GC_SetupZoneEntity.Cast(GetGame().GetWorld().FindEntityByName("SetupFixture"));
		if (!Replication.IsServer() || !limit || !setup)
			return "UNAVAILABLE fixtures";
		limit.GC_TestWarningText(warningText);
		setup.GC_TestDisplayText(setupText);
		return limit.GetWarningText() + " / " + setup.GetDisplayText();
	}
}
