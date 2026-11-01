#ifdef WORKBENCH
//! Session continuity checks around the production menu actions.
modded class GC_LobbyManager
{
	protected IEntity m_GC_LivePrevious;
	protected BaseGameMode m_GC_LiveMode;

	//------------------------------------------------------------------------------------------------
	string GC_TestLiveBegin()
	{
		m_GC_LiveMode = GetGame().GetGameMode();
		m_GC_LivePrevious = GetGame().GetPlayerController().GetControlledEntity();
		return GC_TestLiveState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLiveState()
	{
		IEntity owner = GetGame().GetPlayerController().GetControlledEntity();
		return string.Format("sameSession=%1 sameCharacter=%2 controlled=%3 editing=%4", m_GC_LiveMode && m_GC_LiveMode == GetGame().GetGameMode(), owner == m_GC_LivePrevious, owner != null, GC_WorkbenchItemBrowser.GetEditingPrefab());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLiveLoad(string prefab)
	{
		bool loaded = GC_WorkbenchItemBrowser.LoadPrefab(prefab);
		return string.Format("loaded=%1 %2", loaded, GC_TestLiveState());
	}
}
#endif
