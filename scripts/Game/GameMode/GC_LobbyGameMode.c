//! Add-on-owned game-mode root used only by GameMode_Lobby.et. Other game modes
//! continue to instantiate their original game-mode classes.
class GC_LobbyGameModeClass : SCR_BaseGameModeClass
{
}

class GC_LobbyGameMode : SCR_BaseGameMode
{
	protected int m_iLobbyViewOpenAttempts;
	protected bool m_bStartupLogged;

	//! Returns the active Lobby2 game mode, or null while another mode or the
	//! editor is active.
	static GC_LobbyGameMode GetGameMode()
	{
		if (SCR_Global.IsEditMode())
			return null;

		return GC_LobbyGameMode.Cast(GetGame().GetGameMode());
	}

	void ~GC_LobbyGameMode()
	{
		GetGame().GetCallqueue().Remove(OpenInitialLobbyView);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (SCR_Global.IsEditMode())
			return;

		GetGame().GetCallqueue().CallLater(OpenInitialLobbyView, 1);
	}

	protected void OpenInitialLobbyView()
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
		{
			if (m_iLobbyViewOpenAttempts++ < 40)
				GetGame().GetCallqueue().CallLater(OpenInitialLobbyView, 250);
			else
				Print("GC | Phase | startup unavailable: Lobby2 manager not found", LogLevel.WARNING);
			return;
		}

		if (!m_bStartupLogged)
		{
			m_bStartupLogged = true;
			PrintFormat("GC | Phase | startup: lobby=%1 base=%2", GC_LobbyManager.GetPhaseName(lobbyManager.GetLobbyPhase()), GC_LobbyManager.GetBaseStateName(GetState()));
		}

		if (lobbyManager.GetLobbyPhase() != GC_ELobbyPhase.PREGAME)
			return;

		if (!GetGame().GetPlayerController())
		{
			if (m_iLobbyViewOpenAttempts++ < 40)
				GetGame().GetCallqueue().CallLater(OpenInitialLobbyView, 250);
			return;
		}

		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager)
			menuManager.OpenMenu(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
	}

	override protected bool CanStartGameMode()
	{
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return super.CanStartGameMode();

		if (lobbyManager.GetLobbyPhase() == GC_ELobbyPhase.GAME)
			return super.CanStartGameMode();

		return false;
	}
}
