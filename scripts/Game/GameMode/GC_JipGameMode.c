modded class GC_LobbyGameMode
{
	[Attribute("0", UIWidgets.ComboBox, "Join-in-progress teleport policy.", enums: ParamEnumArray.FromEnum(GC_EJipMode), category: "Join in progress")]
	protected GC_EJipMode m_eJipMode;

	[Attribute(uiwidget: UIWidgets.EditComboBox, desc: "Faction keys allowed to use JIP. Empty means all factions.", category: "Join in progress", enums: {ParamEnum("", "", "gc-choices:1:GC_FactionChoices")})]
	protected ref array<string> m_aJipFactions;

	[Attribute("0", UIWidgets.EditBox, "Seconds after mission start when JIP closes for everyone. Zero disables the cutoff.", category: "Join in progress", params: "0 inf 1")]
	protected int m_iJipDenySeconds;

	protected ref GC_JipSystem m_Jip;

	//------------------------------------------------------------------------------------------------
	GC_JipSystem GetJip()
	{
		return m_Jip;
	}

	//------------------------------------------------------------------------------------------------
	GC_EJipMode GetJipMode()
	{
		return m_eJipMode;
	}

	//------------------------------------------------------------------------------------------------
	//! Policy remains server-owned. Existing eligibility and cutoff rules are not reset.
	bool SetJipMode(GC_EJipMode mode)
	{
		if (!Replication.IsServer() || GC_LobbyGameMode.GetGameMode() != this || !GC_JipSystem.IsValidMode(mode))
			return false;
		m_eJipMode = mode;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	int GetJipDenySeconds()
	{
		return m_iJipDenySeconds;
	}

	//------------------------------------------------------------------------------------------------
	bool IsJipFactionAllowed(string key)
	{
		return !m_aJipFactions || m_aJipFactions.IsEmpty() || m_aJipFactions.Contains(key);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (SCR_Global.IsEditMode() || !Replication.IsServer())
			return;
		m_Jip = new GC_JipSystem();
		m_Jip.Start(this);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyGameMode()
	{
		if (m_Jip && GetGame())
			m_Jip.Stop();
	}
}
