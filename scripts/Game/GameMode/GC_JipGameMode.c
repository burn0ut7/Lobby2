modded class GC_LobbyGameMode
{
	[Attribute("0", UIWidgets.ComboBox, "Join-in-progress teleport policy.", enums: ParamEnumArray.FromEnum(GC_EJipMode), category: "Join in progress")]
	protected GC_EJipMode m_eJipMode;

	[Attribute(desc: "Faction keys allowed to use JIP. Empty means all factions.", category: "Join in progress")]
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
