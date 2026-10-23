//! Shared browsable roster with live name ranking and identity-bound selection.
class GC_AdminPlayerPicker : SCR_ScriptedWidgetComponent
{
	ref ScriptInvoker m_OnChanged = new ScriptInvoker();
	ref ScriptInvoker m_OnOpened = new ScriptInvoker();
	protected GC_LobbyManager m_Manager;
	protected SCR_ButtonTextComponent m_Field;
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected Widget m_Menu;
	protected bool m_bMultiple;
	protected ref array<int> m_aSelected = {};
	protected ref array<string> m_aIdentities = {};
	protected ref map<SCR_ButtonBaseComponent, int> m_mRows = new map<SCR_ButtonBaseComponent, int>();
	protected ref map<SCR_ButtonBaseComponent, string> m_mRowIdentities = new map<SCR_ButtonBaseComponent, string>();

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Manager = GC_LobbyManager.GetManager();
		m_Field = SCR_ButtonTextComponent.GetButtonText("PickerField", w);
		m_Menu = GetGame().GetWorkspace().CreateWidgets("{7C41F8A018B24B03}UI/layouts/Admin/Lobby_AdminPlayerRoster.layout", GetGame().GetWorkspace());
		m_Search = EditBoxWidget.Cast(m_Menu.FindAnyWidget("PickerSearch"));
		m_Search.SetPlaceholderText("Search players...");
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearchChanged);
		m_Menu.SetVisible(false);
		UpdateLabel();
		m_Field.m_OnClicked.Insert(OnOpen);
		if (m_Manager)
		{
			m_Manager.m_OnPlayerConnected.Insert(OnRosterChanged);
			m_Manager.m_OnPlayerDisconnected.Insert(OnRosterChanged);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetMultiple(bool multiple)
	{
		m_bMultiple = multiple;
		UpdateLabel();
	}

	//------------------------------------------------------------------------------------------------
	void SelectPlayer(int playerId, bool toggle = false)
	{
		if (!m_Manager || playerId <= 0)
			return;
		UUID identity = m_Manager.GetPlayerVoiceIdentity(playerId);
		if (identity.IsEmpty())
			return;
		if (!m_bMultiple)
		{
			m_aSelected.Clear();
			m_aIdentities.Clear();
		}
		int index = m_aSelected.Find(playerId);
		if (index >= 0 && toggle)
		{
			m_aSelected.RemoveOrdered(index);
			m_aIdentities.RemoveOrdered(index);
		}
		else if (index < 0)
		{
			m_aSelected.Insert(playerId);
			m_aIdentities.Insert(identity);
		}
		UpdateLabel();
		m_OnChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	bool GetSelection(out array<int> players, out array<string> identities)
	{
		players = {};
		identities = {};
		if (!m_Manager || m_aSelected.IsEmpty())
			return false;
		foreach (int index, int playerId : m_aSelected)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId) || m_Manager.GetPlayerVoiceIdentity(playerId) != m_aIdentities[index])
				return false;
			players.Insert(playerId);
			identities.Insert(m_aIdentities[index]);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateLabel()
	{
		string label = "Choose players...";
		if (!m_bMultiple)
			label = "Choose player...";
		if (m_aSelected.Count() == 1)
			label = GetGame().GetPlayerManager().GetPlayerName(m_aSelected[0]);
		else if (m_aSelected.Count() > 1)
			label = string.Format("%1 players selected", m_aSelected.Count());
		m_Field.SetText(label);
		string names;
		foreach (int playerId : m_aSelected)
		{
			if (!names.IsEmpty())
				names += ", ";
			names += GetGame().GetPlayerManager().GetPlayerName(playerId);
		}
		if (names.IsEmpty())
			names = "Browse all players or search by name";
		else if (m_aSelected.Count() == 1)
		{
			names = "1 player selected";
			Faction faction = m_Manager.GetPlayerFaction(m_aSelected[0]);
			if (faction)
				names = WidgetManager.Translate(faction.GetFactionName());
		}
		TextWidget.Cast(m_Field.GetRootWidget().FindAnyWidget("PlayerDetail")).SetText(names);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpen(SCR_ButtonBaseComponent button)
	{
		bool opening = !m_Menu.IsVisible();
		if (!opening)
		{
			CloseRoster();
			return;
		}
		m_Menu.SetVisible(opening);
		if (opening)
		{
			m_OnOpened.Invoke(this);
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			float x, y, width, height;
			m_wRoot.GetScreenPos(x, y);
			m_wRoot.GetScreenSize(width, height);
			float available = workspace.DPIUnscale(workspace.GetHeight() * 0.91 - y - height);
			FrameSlot.SetPos(m_Menu, workspace.DPIUnscale(x), workspace.DPIUnscale(y + height));
			FrameSlot.SetSize(m_Menu, workspace.DPIUnscale(width), Math.Max(0, available));
			m_Search.SetText("");
			Rebuild();
			GetGame().GetWorkspace().SetFocusedWidget(m_Search);
		}
	}

	//------------------------------------------------------------------------------------------------
	void CloseRoster()
	{
		m_Menu.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearchChanged(Widget w)
	{
		if (w == m_Search)
			Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	static int MatchRank(string name, string query)
	{
		name.ToLower();
		query.ToLower();
		if (query.IsEmpty() || name == query)
			return 0;
		if (name.StartsWith(query))
			return 1;
		if (name.Contains(query))
			return 2;
		int matched;
		for (int i; i < name.Length() && matched < query.Length(); i++)
		{
			if (name.Substring(i, 1) == query.Substring(matched, 1))
				matched++;
		}
		if (matched == query.Length())
			return 3;
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! One current display snapshot per refresh; selection still validates native identity.
	protected void ReadRoster(out array<int> players, out array<string> names)
	{
		players = {};
		names = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
			names.Insert(GetGame().GetPlayerManager().GetPlayerName(playerId));
	}

	//------------------------------------------------------------------------------------------------
	protected void Rebuild()
	{
		foreach (SCR_ButtonBaseComponent oldButton, int oldId : m_mRows)
			oldButton.GetRootWidget().RemoveFromHierarchy();
		m_mRows.Clear();
		m_mRowIdentities.Clear();
		if (!m_Manager)
			return;
		array<int> players;
		array<string> names;
		ReadRoster(players, names);
		string query = m_Search.GetText();
		int shown;
		// Four stable relevance buckets avoid rebuilding or querying the server per key.
		for (int rank; rank < 4; rank++)
		{
			foreach (int index, int playerId : players)
			{
				string name = names[index];
				if (MatchRank(name, query) != rank)
					continue;
				Widget row = GetGame().GetWorkspace().CreateWidgets("{7C41F8A018B24C11}UI/layouts/Admin/Lobby_AdminPlayerRow.layout", m_Menu.FindAnyWidget("PickerRows"));
				if (!row)
					continue;
				row.SetName("AdminPlayer_" + playerId.ToString());
				SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(row.FindHandler(SCR_ButtonTextComponent));
				Faction faction = m_Manager.GetPlayerFaction(playerId);
				string detail = "Unassigned";
				if (faction)
					detail = WidgetManager.Translate(faction.GetFactionName());
				SCR_AIGroup group = m_Manager.GetPlayerGroup(playerId);
				if (group)
					detail += " / " + GC_GroupCallsignHelper.GetDisplayName(group);
				TextWidget.Cast(row.FindAnyWidget("PlayerDetail")).SetText(detail);
				bool selected = m_aSelected.Contains(playerId);
				row.FindAnyWidget("PlayerCheckBox").SetVisible(m_bMultiple);
				row.FindAnyWidget("PlayerCheckFill").SetVisible(selected);
				row.FindAnyWidget("PlayerCheck").SetVisible(selected);
				button.SetText(name);
				button.SetToggled(selected, false, false);
				button.m_OnClicked.Insert(OnSelect);
				m_mRows.Set(button, playerId);
				m_mRowIdentities.Set(button, m_Manager.GetPlayerVoiceIdentity(playerId));
				shown++;
			}
		}
		TextWidget.Cast(m_Menu.FindAnyWidget("PickerEmpty")).SetVisible(shown == 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelect(SCR_ButtonBaseComponent button)
	{
		int playerId;
		if (!m_mRows.Find(button, playerId))
			return;
		string identity;
		if (!m_mRowIdentities.Find(button, identity) || identity.IsEmpty()
			|| m_Manager.GetPlayerVoiceIdentity(playerId) != identity)
		{
			Rebuild();
			return;
		}
		SelectPlayer(playerId, true);
		if (m_bMultiple)
			Rebuild();
		else
			CloseRoster();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRosterChanged(int playerId)
	{
		int index = m_aSelected.Find(playerId);
		if (index >= 0 && (!GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			|| m_Manager.GetPlayerVoiceIdentity(playerId) != m_aIdentities[index]))
		{
			m_aSelected.RemoveOrdered(index);
			m_aIdentities.RemoveOrdered(index);
			UpdateLabel();
			m_OnChanged.Invoke();
		}
		if (m_Menu.IsVisible())
			Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		// The workspace popup is separately owned and survives field-child teardown.
		if (m_SearchEvents)
			m_SearchEvents.GetOnChange().Remove(OnSearchChanged);
		if (m_Menu)
			m_Menu.RemoveFromHierarchy();
		m_Menu = null;
		m_SearchEvents = null;
		if (m_Manager)
		{
			m_Manager.m_OnPlayerConnected.Remove(OnRosterChanged);
			m_Manager.m_OnPlayerDisconnected.Remove(OnRosterChanged);
		}
		m_mRows.Clear();
		m_mRowIdentities.Clear();
		m_Manager = null;
		super.HandlerDeattached(w);
	}
}
