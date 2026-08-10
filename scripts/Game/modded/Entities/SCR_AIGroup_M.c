modded class SCR_AIGroup : ChimeraAIGroup
{
	override void EOnInit(IEntity owner)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if(m_bPlayable && manager)
			manager.RegisterGroup(this);

		super.EOnInit(owner);
	}

	override void SpawnUnits()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if(m_bPlayable && manager && !GetWorld().IsEditMode())
			return;
		
		super.SpawnUnits();
	}
}