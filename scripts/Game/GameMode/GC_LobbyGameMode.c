//! Add-on-owned game-mode root used only by GameMode_Lobby.et. Other game modes
//! continue to instantiate their original game-mode classes.
class GC_LobbyGameModeClass : SCR_BaseGameModeClass
{
}

class GC_LobbyGameMode : SCR_BaseGameMode
{
	protected bool m_bStartupLogged;
	protected ref GC_MapPlans m_MapPlans;
	protected ref GC_AOLimitSystem m_AOLimits;
	protected ref GC_MapShapeSystem m_MapShapes;
	protected ref GC_BoundaryAdmin m_BoundaryAdmin;

	//------------------------------------------------------------------------------------------------
	GC_BoundaryAdmin GetBoundaryAdmin()
	{
		return m_BoundaryAdmin;
	}

	//------------------------------------------------------------------------------------------------
	GC_MapShapeSystem GetMapShapes()
	{
		return m_MapShapes;
	}

	//------------------------------------------------------------------------------------------------
	GC_AOLimitSystem GetAOLimits()
	{
		return m_AOLimits;
	}

	//------------------------------------------------------------------------------------------------
	GC_MapPlans GetMapPlans()
	{
		return m_MapPlans;
	}

	//! Returns the active Lobby2 game mode, or null while another mode or the
	//! editor is active.
	static GC_LobbyGameMode GetGameMode()
	{
		if (SCR_Global.IsEditMode())
			return null;

		return GC_LobbyGameMode.Cast(GetGame().GetGameMode());
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (SCR_Global.IsEditMode())
			return;

		if (Replication.IsServer())
		{
			m_BoundaryAdmin = new GC_BoundaryAdmin();
			m_BoundaryAdmin.Start(this);
			m_MapShapes = new GC_MapShapeSystem();
			m_MapPlans = new GC_MapPlans();
			m_MapPlans.Start(this);
			m_AOLimits = new GC_AOLimitSystem();
			m_AOLimits.Start(this);
		}

		PrintFormat("GC | Phase UI Trace | game mode initialized: this=%1 state=%2 server=%3 rplMode=%4 playerController=%5 menuManager=%6", this, GC_LobbyManager.GetPhaseName(GetState()), Replication.IsServer(), RplSession.Mode(), GetGame().GetPlayerController(), GetGame().GetMenuManager());
	}

	//! Live transitions and JIP both use the replicated base game-mode state.
	override protected void OnGameStateChanged()
	{
		super.OnGameStateChanged();
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.Cast(FindComponent(GC_LobbyFreezeComponent));
		if (freeze)
			freeze.OnPhaseChanged();

		PrintFormat("GC | Phase UI Trace | game state callback: state=%1 server=%2 playerController=%3 menuManager=%4", GC_LobbyManager.GetPhaseName(GetState()), Replication.IsServer(), GetGame().GetPlayerController(), GetGame().GetMenuManager());
		UpdateLocalPhasePresentation(null, "game-state-changed");
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		// Native editor-mode listeners may change GAME_MASTER during an admin
		// notification. Dispatch outside that native stack to avoid recursive
		// invocation of the base game mode's role event. Authority remains native.
		GetGame().GetCallqueue().CallLater(DispatchPlayerRoleChange, 0, false, playerId, roleFlags);
	}

	//------------------------------------------------------------------------------------------------
	protected void DispatchPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		if (GetGame().GetGameMode() != this)
			return;

		super.OnPlayerRoleChange(playerId, roleFlags);

		if (GC_UIEventDiagnostics.ENABLED)
			PrintFormat("GC | Admin Trace | game-mode role callback: server=%1 changedPlayer=%2 roleFlags=%3 queriedAdmin=%4", Replication.IsServer(), playerId, roleFlags, SCR_Global.IsAdmin(playerId));

		if (!Replication.IsServer())
			return;

		GC_LobbyManager lobbyManager = GC_LobbyManager.Cast(FindComponent(GC_LobbyManager));
		if (lobbyManager)
			lobbyManager.PublishPlayerRoleChanged(playerId, roleFlags);
	}

	//! The local player controller calls this from OnInit. State changes call the
	//! same function so initial and subsequent presentation cannot diverge.
	void UpdateLocalPhasePresentation(PlayerController localController = null, string source = "unspecified")
	{
		// A headed listen server owns local UI just like a connected client. Only
		// a dedicated server has no local presentation.
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			PrintFormat("GC | Phase UI Trace | presentation stopped: source=%1 reason=dedicated-server state=%2", source, GC_LobbyManager.GetPhaseName(GetState()));
			return;
		}

		PlayerController publishedController = GetGame().GetPlayerController();
		MenuManager menuManager = GetGame().GetMenuManager();
		MenuBase topMenu;
		if (menuManager)
			topMenu = menuManager.GetTopMenu();
		PrintFormat("GC | Phase UI Trace | presentation requested: source=%1 state=%2 suppliedController=%3 publishedController=%4 menuManager=%5 top=%6", source, GC_LobbyManager.GetPhaseName(GetState()), localController, publishedController, menuManager, topMenu);

		if (!localController)
			localController = publishedController;
		if (!localController)
		{
			PrintFormat("GC | Phase UI Trace | presentation stopped: source=%1 reason=no-player-controller state=%2", source, GC_LobbyManager.GetPhaseName(GetState()));
			return;
		}

		if (!menuManager)
		{
			PrintFormat("GC | Phase UI Trace | presentation stopped: source=%1 reason=no-menu-manager state=%2", source, GC_LobbyManager.GetPhaseName(GetState()), level: LogLevel.ERROR);
			return;
		}

		SCR_EGameModeState phase = GetState();
		if (!m_bStartupLogged)
		{
			m_bStartupLogged = true;
			PrintFormat("GC | Phase | local presentation ready: state=%1", GC_LobbyManager.GetPhaseName(phase));
		}

		// Remove any stale local lobby view without disturbing spectator, pause,
		// Game Master, or another game mode's menus.
		if (phase != SCR_EGameModeState.PREGAME)
			menuManager.CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_PreviewMenu);
		if (phase != SCR_EGameModeState.SLOTTING)
			menuManager.CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_SlottingMenu);
		if (phase != SCR_EGameModeState.BRIEFING)
			menuManager.CloseMenuByPreset(ChimeraMenuPreset.GC_Lobby_BriefingMenu);

		PrintFormat("GC | Phase UI Trace | applying authoritative presentation: source=%1 phase=%2 controller=%3 topBefore=%4", source, GC_LobbyManager.GetPhaseName(phase), localController, menuManager.GetTopMenu());
		MenuBase openedMenu;
		if (phase == SCR_EGameModeState.PREGAME)
			openedMenu = menuManager.OpenMenu(ChimeraMenuPreset.GC_Lobby_PreviewMenu, 0, true);
		else if (phase == SCR_EGameModeState.SLOTTING)
			openedMenu = menuManager.OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
		else if (phase == SCR_EGameModeState.BRIEFING)
		{
			GC_LobbyManager lobbyManager = GC_LobbyManager.Cast(FindComponent(GC_LobbyManager));
			if (lobbyManager && lobbyManager.GetPlayerFaction(localController.GetPlayerId()))
				openedMenu = menuManager.OpenMenu(ChimeraMenuPreset.GC_Lobby_BriefingMenu, 0, true);
		}

		PrintFormat("GC | Phase UI Trace | menu open result: source=%1 phase=%2 menu=%3 top=%4", source, GC_LobbyManager.GetPhaseName(phase), openedMenu, menuManager.GetTopMenu());
	}

	override protected bool CanStartGameMode()
	{
		if (GetState() == SCR_EGameModeState.GAME)
			return super.CanStartGameMode();

		return false;
	}
	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyGameMode()
	{
		if (m_BoundaryAdmin && GetGame())
			m_BoundaryAdmin.Stop();
		if (m_MapShapes)
			m_MapShapes.Stop();
		if (m_AOLimits && GetGame())
			m_AOLimits.Stop();
		if (m_MapPlans && GetGame())
			m_MapPlans.Stop();
		if (GetGame())
			GetGame().GetCallqueue().Remove(DispatchPlayerRoleChange);
	}

}
