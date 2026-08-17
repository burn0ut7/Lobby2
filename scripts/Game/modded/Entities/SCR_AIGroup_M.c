//! Lobby2 integration for the base group type.
//!
//! This is intentionally a guarded modded extension rather than a replacement
//! group class. Every world keeps using the base SCR_AIGroup/prefab resources;
//! the Lobby2 behavior is active only while GC_LobbyGameMode is the active mode.
modded class SCR_AIGroup : ChimeraAIGroup
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !m_bPlayable)
			return;

		manager.RegisterGroup(this);
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
		if (manager && m_bPlayable && !GetWorld().IsEditMode())
			return;

		super.SpawnUnits();
	}
}
