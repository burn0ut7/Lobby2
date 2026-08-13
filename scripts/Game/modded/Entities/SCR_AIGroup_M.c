modded class SCR_AIGroup : ChimeraAIGroup
{
	override void EOnInit(IEntity owner)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if(m_bPlayable && manager)
			manager.RegisterGroup(this);

		super.EOnInit(owner);
	}

	void ~SCR_AIGroup()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.UnregisterGroup(this);
	}

	override void SpawnUnits()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if(m_bPlayable && manager && !GetWorld().IsEditMode())
			return;
		
		super.SpawnUnits();
	}
}
