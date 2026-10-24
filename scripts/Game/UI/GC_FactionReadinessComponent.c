//! Briefing-only presentation; the manager owns readiness and command eligibility.
class GC_FactionReadinessComponent : SCR_ScriptedWidgetComponent
{
	protected const ResourceName TILE_LAYOUT = "{6A71000000000001}UI/layouts/Briefing/Lobby_Faction_Readiness.layout";
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_Controller;
	protected Widget m_wFactions;
	protected ref map<string, Widget> m_mTiles = new map<string, Widget>();

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		if (SCR_Global.IsEditMode())
			return;
		m_Manager = GC_LobbyManager.GetManager();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_wFactions = w.FindAnyWidget("ReadinessFactions");
		if (!m_Manager || !m_Controller || !m_wFactions)
			return;

		m_Manager.m_OnFactionTopologyChanged.Insert(OnTopologyChanged);
		m_Manager.m_OnRegistryReset.Insert(OnRegistryReset);
		m_Manager.m_OnFactionReadinessChanged.Insert(Refresh);
		m_Manager.m_OnSlotPresentationChanged.Insert(OnSlotChanged);
		m_Manager.m_OnPlayerAffiliationChanged.Insert(Refresh);
		m_Manager.m_OnPlayerDisconnected.Insert(Refresh);
		m_Manager.m_OnPlayerRoleChanged.Insert(Refresh);
		m_Manager.m_OnLobbyPhaseChanged.Insert(OnPhaseChanged);
		m_Controller.m_OnFactionReadinessResult.Insert(OnResult);
		OnTopologyChanged(null);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_Manager)
		{
			m_Manager.m_OnFactionTopologyChanged.Remove(OnTopologyChanged);
			m_Manager.m_OnRegistryReset.Remove(OnRegistryReset);
			m_Manager.m_OnFactionReadinessChanged.Remove(Refresh);
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlotChanged);
			m_Manager.m_OnPlayerAffiliationChanged.Remove(Refresh);
			m_Manager.m_OnPlayerDisconnected.Remove(Refresh);
			m_Manager.m_OnPlayerRoleChanged.Remove(Refresh);
			m_Manager.m_OnLobbyPhaseChanged.Remove(OnPhaseChanged);
		}
		if (m_Controller)
			m_Controller.m_OnFactionReadinessResult.Remove(OnResult);
		m_mTiles.Clear();
		m_Manager = null;
		m_Controller = null;
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTopologyChanged(Faction faction)
	{
		array<string> keys = {};
		foreach (Faction registeredFaction : m_Manager.GetRegisteredFactions())
		{
			string key = registeredFaction.GetFactionKey();
			if (!m_Manager.GetFactionCommanderSlot(key))
				continue;
			keys.Insert(key);
			if (m_mTiles.Contains(key))
				continue;
			Widget tile = GetGame().GetWorkspace().CreateWidgets(TILE_LAYOUT, m_wFactions);
			if (!tile)
				continue;
			GC_FactionReadyTileComponent component = GC_FactionReadyTileComponent.Cast(tile.FindHandler(GC_FactionReadyTileComponent));
			component.SetFactionKey(key);
			m_mTiles.Set(key, tile);
		}

		array<string> removed = {};
		foreach (string key, Widget tile : m_mTiles)
		{
			if (!keys.Contains(key))
			{
				tile.RemoveFromHierarchy();
				removed.Insert(key);
			}
		}
		foreach (string key : removed)
			m_mTiles.Remove(key);
		Refresh(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh(int unused)
	{
		if (!m_Manager || GC_LobbyManager.GetManager() != m_Manager)
			return;
		m_wRoot.SetVisible(m_Manager.GetLobbyPhase() == SCR_EGameModeState.BRIEFING);
		foreach (string key, Widget tile : m_mTiles)
		{
			GC_FactionReadyTileComponent component = GC_FactionReadyTileComponent.Cast(tile.FindHandler(GC_FactionReadyTileComponent));
			component.Refresh(m_Manager, m_Controller);
		}
		TextWidget error = TextWidget.Cast(m_wRoot.FindAnyWidget("ReadinessError"));
		if (error)
		{
			error.SetText(m_Controller.GetFactionReadinessError());
			error.SetVisible(!m_Controller.GetFactionReadinessError().IsEmpty());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRegistryReset(int unused)
	{
		OnTopologyChanged(null);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlotChanged(GC_Slot slot)
	{
		Refresh(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPhaseChanged(SCR_EGameModeState phase)
	{
		Refresh(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(string error)
	{
		Refresh(0);
	}
}

//! One faction's state and its clickable readiness icon.
class GC_FactionReadyTileComponent : SCR_ScriptedWidgetComponent
{
	protected string m_sFactionKey;
	protected int m_iSlotId;
	protected int m_iRevision;
	protected bool m_bReady;
	protected SCR_ButtonBaseComponent m_Button;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		Widget button = w.FindAnyWidget("ReadinessAction");
		m_Button = SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent));
		m_Button.m_OnClicked.Insert(OnClicked);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_Button)
			m_Button.m_OnClicked.Remove(OnClicked);
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	void SetFactionKey(string factionKey)
	{
		m_sFactionKey = factionKey;
		m_wRoot.FindAnyWidget("ReadinessAction").SetName("ReadinessAction_" + factionKey);
	}

	//------------------------------------------------------------------------------------------------
	void Refresh(GC_LobbyManager manager, GC_LobbyPlayerController controller)
	{
		Faction faction = manager.GetRegisteredFaction(m_sFactionKey);
		GC_Slot slot = manager.GetFactionCommanderSlot(m_sFactionKey);
		if (!faction || !slot)
			return;
		m_iSlotId = slot.GetID();
		m_iRevision = manager.GetFactionReadinessRevision();
		m_bReady = manager.IsFactionReady(m_sFactionKey);
		int commanderId = manager.GetFactionCommanderPlayerId(m_sFactionKey);
		TextWidget name = TextWidget.Cast(m_wRoot.FindAnyWidget("ReadinessFactionName"));
		name.SetText(WidgetManager.Translate(faction.GetFactionName()));
		ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("ReadinessAccent"));
		accent.SetColor(faction.GetFactionColor());
		ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget("ReadinessIcon"));
		string imageName = "disable";
		if (m_bReady)
			imageName = "check";
		icon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", imageName);
		icon.SetColor(Color.White);
		if (m_bReady)
			icon.SetColor(Color.FromRGBA(126, 218, 139, 255));

		TextWidget commander = TextWidget.Cast(m_wRoot.FindAnyWidget("ReadinessCommander"));
		string commanderName = "Vacant";
		if (commanderId > 0)
			commanderName = GetGame().GetPlayerManager().GetPlayerName(commanderId);
		commander.SetText(commanderName);
		if (commanderId > 0)
			GC_PlayerNamePresentation.Apply(commander, commanderId);
		else
			commander.SetColor(Color.FromRGBA(150, 150, 150, 255));
		bool canChange = commanderId == controller.GetPlayerId() && manager.GetLobbyPhase() == SCR_EGameModeState.BRIEFING;
		m_Button.SetEnabled(canChange && !controller.IsFactionReadinessPending());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnClicked(SCR_ButtonBaseComponent button)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!manager || !controller || manager.GetFactionCommanderPlayerId(m_sFactionKey) != controller.GetPlayerId())
			return;
		controller.RequestFactionReadiness(m_sFactionKey, m_iSlotId, m_iRevision, !m_bReady);
	}
}
