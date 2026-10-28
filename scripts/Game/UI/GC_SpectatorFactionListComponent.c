//! Spectator-only view over the existing connected-player and slot roster.
class GC_SpectatorFactionListComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyManager m_Manager;
	protected bool m_bActive;
	protected int m_iLifeFilter;
	protected string m_sFaction;
	protected ref map<SCR_ButtonBaseComponent, int> m_mLifeButtons = new map<SCR_ButtonBaseComponent, int>();
	protected ref map<SCR_ButtonBaseComponent, string> m_mFactionButtons = new map<SCR_ButtonBaseComponent, string>();
	protected static const ResourceName BUTTON = "{7C41F8A018B24C10}UI/layouts/Admin/Lobby_AdminToolButton.layout";

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Manager = GC_LobbyManager.GetManager();
		if (!m_Manager || SCR_Global.IsEditMode())
			return;
		array<string> labels = {"All", "Alive", "Dead"};
		foreach (int index, string label : labels)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(BUTTON, w.FindAnyWidget("LifeFilters"));
			row.SetName("FactionLife" + index.ToString());
			LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(row, 1);
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
			button.SetText(label);
			button.SetToggled(index == 0, false, false);
			button.m_OnClicked.Insert(OnLife);
			m_mLifeButtons.Set(button, index);
		}
		m_Manager.m_OnRegistryReset.Insert(OnRegistry);
		m_Manager.m_OnPlayerConnected.Insert(OnPlayer);
		m_Manager.m_OnPlayerDisconnected.Insert(OnPlayer);
		m_Manager.m_OnPlayerAffiliationChanged.Insert(OnPlayer);
		m_Manager.m_OnVoiceAssignmentChanged.Insert(OnPlayer);
		m_Manager.m_OnPlayerRoleChanged.Insert(OnPlayer);
		m_Manager.m_OnSlotPresentationChanged.Insert(OnSlot);
		SetActive(false);
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		m_bActive = active;
		m_wRoot.SetVisible(active);
		GetGame().GetCallqueue().Remove(Refresh);
		if (active)
			Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLife(SCR_ButtonBaseComponent button)
	{
		m_mLifeButtons.Find(button, m_iLifeFilter);
		foreach (SCR_ButtonBaseComponent option, int value : m_mLifeButtons)
			option.SetToggled(value == m_iLifeFilter, false, false);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFaction(SCR_ButtonBaseComponent button)
	{
		string key;
		if (!m_mFactionButtons.Find(button, key))
			return;
		if (key == m_sFaction)
			m_sFaction = "";
		else
			m_sFaction = key;
		// Keep the clicked control alive until its native event finishes.
		QueueRefresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesLife(int playerId)
	{
		GC_Slot slot = m_Manager.GetSlotByPlayerId(playerId);
		bool dead = slot && slot.GetState() == GC_SlotState.KIA;
		return m_iLifeFilter == 0 || (m_iLifeFilter == 1 && !dead) || (m_iLifeFilter == 2 && dead);
	}

	//------------------------------------------------------------------------------------------------
	protected void Clear(Widget parent)
	{
		while (parent.GetChildren())
			parent.GetChildren().RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_bActive || !m_Manager || GC_LobbyManager.GetManager() != m_Manager)
			return;
		Widget ratios = m_wRoot.FindAnyWidget("FactionRatios");
		Widget counts = m_wRoot.FindAnyWidget("FactionCounts");
		Widget roster = m_wRoot.FindAnyWidget("FactionRoster");
		m_mFactionButtons.Clear();
		Clear(ratios);
		Clear(counts);
		Clear(roster);
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<Faction> factions = m_Manager.GetRegisteredFactions();
		bool selectedExists = m_sFaction.IsEmpty();
		foreach (Faction registered : factions)
		{
			if (registered.GetFactionKey() == m_sFaction)
				selectedExists = true;
		}
		if (!selectedExists)
			m_sFaction = "";
		int visiblePlayers;
		foreach (Faction faction : factions)
		{
			array<int> members = {};
			foreach (int playerId : players)
			{
				if (m_Manager.GetPlayerFaction(playerId) == faction && MatchesLife(playerId))
					members.Insert(playerId);
			}
			string key = faction.GetFactionKey();
			if (!members.IsEmpty())
			{
				Widget segment = GetGame().GetWorkspace().CreateWidgets("{D389716BFE230B02}UI/layouts/Spectator/Lobby_Spectator_Faction_Segment.layout", ratios);
				segment.SetName("FactionRatio_" + key);
				segment.SetColor(faction.GetFactionColor());
				LayoutSlot.SetFillWeight(segment, members.Count());
			}
			Widget countRow = GetGame().GetWorkspace().CreateWidgets(BUTTON, counts);
			countRow.SetName("FactionCount_" + key);
			LayoutSlot.SetSizeMode(countRow, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(countRow, 1);
			SCR_ButtonTextComponent countButton = SCR_ButtonTextComponent.Cast(countRow.FindHandler(SCR_ButtonTextComponent));
			countButton.SetText(key + "  " + members.Count().ToString());
			Color background = faction.GetFactionColor().Copy();
			background.SetA(0.35);
			Color hovered = faction.GetFactionColor().Copy();
			hovered.SetA(0.5);
			countButton.SetBackgroundColors(background, hovered);
			countButton.SetToggled(key == m_sFaction, false, false);
			countButton.ColorizeBackground(false);
			countButton.m_OnClicked.Insert(OnFaction);
			m_mFactionButtons.Set(countButton, key);
			if (!m_sFaction.IsEmpty() && m_sFaction != key)
				continue;
			if (members.IsEmpty())
				continue;
			AddHeading(WidgetManager.Translate(faction.GetFactionName()), roster);
			array<SCR_AIGroup> groups = m_Manager.GetGroupsForFaction(key);
			foreach (SCR_AIGroup group : groups)
			{
				bool heading;
				foreach (int playerId : members)
				{
					if (m_Manager.GetPlayerGroup(playerId) != group)
						continue;
					if (!heading)
						AddHeading(GC_GroupCallsignHelper.GetDisplayName(group), roster);
					heading = true;
					AddPlayer(playerId, roster);
					visiblePlayers++;
				}
			}
			foreach (int playerId : members)
			{
				if (groups.Contains(m_Manager.GetPlayerGroup(playerId)))
					continue;
				AddPlayer(playerId, roster);
				visiblePlayers++;
			}
		}
		m_wRoot.FindAnyWidget("FactionEmpty").SetVisible(visiblePlayers == 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddHeading(string text, Widget parent)
	{
		TextWidget heading = TextWidget.Cast(GetGame().GetWorkspace().CreateWidgets("{D389716BFE230B03}UI/layouts/Spectator/Lobby_Spectator_Faction_Heading.layout", parent));
		heading.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddPlayer(int playerId, Widget parent)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets("{6A33000000000001}UI/layouts/Players/Lobby_Player_Row.layout", parent);
		row.SetName("FactionPlayer_" + playerId.ToString());
		GC_Slot slot = m_Manager.GetSlotByPlayerId(playerId);
		GC_VoicePlayerRowComponent playerRow = GC_VoicePlayerRowComponent.Cast(row.FindHandler(GC_VoicePlayerRowComponent));
		playerRow.SetShowLifeState(true);
		playerRow.SetPlayer(playerId, slot);
	}

	//------------------------------------------------------------------------------------------------
	protected void QueueRefresh()
	{
		if (!m_bActive)
			return;
		GetGame().GetCallqueue().Remove(Refresh);
		GetGame().GetCallqueue().CallLater(Refresh, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRegistry(int revision)
	{
		QueueRefresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayer(int playerId)
	{
		QueueRefresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlot(GC_Slot slot)
	{
		QueueRefresh();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Refresh);
		if (m_Manager)
		{
			m_Manager.m_OnRegistryReset.Remove(OnRegistry);
			m_Manager.m_OnPlayerConnected.Remove(OnPlayer);
			m_Manager.m_OnPlayerDisconnected.Remove(OnPlayer);
			m_Manager.m_OnPlayerAffiliationChanged.Remove(OnPlayer);
			m_Manager.m_OnVoiceAssignmentChanged.Remove(OnPlayer);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnPlayer);
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlot);
		}
		super.HandlerDeattached(w);
	}
}
