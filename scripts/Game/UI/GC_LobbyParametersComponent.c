//! Standalone scenario-parameter view. Faction selection controls presentation;
//! GC_LobbyManager remains the sole authority for validation and mutation.
class GC_LobbyParametersComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName ROW_LAYOUT = "{6A4D000000000008}UI/layouts/Briefing/Lobby_Briefing_Parameter_Row.layout";
	protected static const int MAX_INITIALIZE_ATTEMPTS = 20;
	protected GC_FactionListComponent m_FactionList;
	protected Widget m_wRows;
	protected ScrollLayoutWidget m_wScroll;
	protected TextWidget m_wStatus;
	protected SCR_ButtonBaseComponent m_ApplyButton;
	protected GC_LobbyManager m_LobbyManager;
	protected GC_LobbyGameMode m_LobbyGameMode;
	protected ref array<GC_BriefingParameterRowComponent> m_aRows = {};
	protected FactionKey m_SelectedFactionKey;
	protected int m_iBaseRevision;
	protected bool m_bEditable;
	protected bool m_bDirty;
	protected bool m_bStale;
	protected int m_iEditableRowCount;
	protected int m_iInitializeAttempts;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRows = w.FindAnyWidget("ScenarioParametersRows");
		m_wScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("ScenarioParametersScroll"));
		m_wStatus = TextWidget.Cast(w.FindAnyWidget("ScenarioParametersStatus"));

		Widget factionListWidget = w.FindAnyWidget("ParametersFactionList");
		if (factionListWidget)
			m_FactionList = GC_FactionListComponent.Cast(factionListWidget.FindHandler(GC_FactionListComponent));
		if (m_FactionList)
			m_FactionList.m_OnFactionSelected.Insert(OnFactionSelected);

		Widget applyWidget = w.FindAnyWidget("ScenarioParametersApply");
		if (applyWidget)
			m_ApplyButton = SCR_ButtonBaseComponent.Cast(applyWidget.FindHandler(SCR_ButtonBaseComponent));
		if (m_ApplyButton)
			m_ApplyButton.m_OnClicked.Insert(OnApplyClicked);

		m_LobbyManager = GC_LobbyManager.GetManager();
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnScenarioParametersChanged.Insert(OnParametersChanged);
			m_LobbyManager.m_OnLobbyPhaseChanged.Insert(OnLobbyPhaseChanged);
		}

		m_LobbyGameMode = GC_LobbyGameMode.GetGameMode();
		if (m_LobbyGameMode)
			m_LobbyGameMode.GetOnPlayerRoleChange().Insert(OnPlayerRoleChanged);

		GetGame().GetCallqueue().CallLater(InitializeView, 0);
	}

	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(InitializeView);
		GetGame().GetCallqueue().Remove(RefreshAdminPresentation);
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnScenarioParametersChanged.Remove(OnParametersChanged);
			m_LobbyManager.m_OnLobbyPhaseChanged.Remove(OnLobbyPhaseChanged);
		}
		if (m_LobbyGameMode)
			m_LobbyGameMode.GetOnPlayerRoleChange().Remove(OnPlayerRoleChanged);
		if (m_FactionList)
			m_FactionList.m_OnFactionSelected.Remove(OnFactionSelected);
		if (m_ApplyButton)
			m_ApplyButton.m_OnClicked.Remove(OnApplyClicked);

		ClearRows();
		m_FactionList = null;
		m_LobbyManager = null;
		m_LobbyGameMode = null;
		m_wRows = null;
		m_wScroll = null;
		m_wStatus = null;
		m_ApplyButton = null;
		super.HandlerDeattached(w);
	}

	protected void InitializeView()
	{
		if (!m_LobbyManager || !m_LobbyManager.IsScenarioParameterCatalogValid())
		{
			if (m_wStatus)
				m_wStatus.SetText("Scenario parameters are unavailable.");
			return;
		}

		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
		{
			m_iInitializeAttempts++;
			if (m_iInitializeAttempts >= MAX_INITIALIZE_ATTEMPTS)
			{
				if (m_wStatus)
					m_wStatus.SetText("Unable to initialize the parameter view.");
				return;
			}

			GetGame().GetCallqueue().CallLater(InitializeView, 250);
			return;
		}

		m_iInitializeAttempts = 0;
		EPlayerRole roleFlags = GetGame().GetPlayerManager().GetPlayerRoles(playerController.GetPlayerId());
		bool isAdmin = SCR_Global.IsAdmin(playerController.GetPlayerId());
		SCR_EGameModeState phase = m_LobbyManager.GetLobbyPhase();
		m_bEditable = isAdmin && (phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING);
		PrintFormat("GC | Admin Trace | parameters initial permission: player=%1 roleFlags=%2 flagsAdmin=%3 queriedAdmin=%4 phase=%5 editable=%6", playerController.GetPlayerId(), roleFlags, SCR_Global.IsAdminRole(roleFlags), isAdmin, GC_LobbyManager.GetPhaseName(phase), m_bEditable);
		m_bDirty = false;
		m_bStale = false;
		m_iBaseRevision = m_LobbyManager.GetScenarioParameterRevision();

		Faction initialFaction = m_LobbyManager.GetPlayerFaction(playerController.GetPlayerId());
		if (!initialFaction)
		{
			foreach (Faction faction : m_LobbyManager.GetRegisteredFactions())
			{
				if (faction)
				{
					initialFaction = faction;
					break;
				}
			}
		}

		if (initialFaction)
			m_SelectedFactionKey = initialFaction.GetFactionKey();
		else
			m_SelectedFactionKey = string.Empty;

		if (m_FactionList && !m_SelectedFactionKey.IsEmpty())
			m_FactionList.SetSelectedFaction(m_SelectedFactionKey, false);

		RefreshRows();
		GetGame().GetCallqueue().CallLater(RefreshAdminPresentation, 250);
	}

	protected void RefreshAdminPresentation()
	{
		GetGame().GetCallqueue().Remove(RefreshAdminPresentation);
		if (m_LobbyManager && GetGame().GetPlayerController())
			OnLobbyPhaseChanged(m_LobbyManager.GetLobbyPhase());

		GetGame().GetCallqueue().CallLater(RefreshAdminPresentation, 250);
	}

	protected void OnFactionSelected(FactionKey factionKey)
	{
		if (factionKey == m_SelectedFactionKey)
			return;

		if (m_bDirty)
		{
			if (m_FactionList && !m_SelectedFactionKey.IsEmpty())
				m_FactionList.SetSelectedFaction(m_SelectedFactionKey, false);
			if (m_wStatus)
				m_wStatus.SetText("Apply your changes before selecting another faction.");
			return;
		}

		m_SelectedFactionKey = factionKey;
		m_bStale = false;
		if (m_LobbyManager)
			m_iBaseRevision = m_LobbyManager.GetScenarioParameterRevision();
		RefreshRows();
		if (m_wScroll)
			m_wScroll.SetSliderPos(0, 0);
	}

	protected void RefreshRows()
	{
		ClearRows();
		if (!m_wRows || !m_LobbyManager)
			return;

		Faction selectedFaction = m_LobbyManager.GetRegisteredFaction(m_SelectedFactionKey);
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = -1;
		if (playerController)
			playerId = playerController.GetPlayerId();

		array<ref GC_LobbyParameterDefinition> definitions = m_LobbyManager.GetScenarioParameterDefinitions();
		array<ref GC_LobbyParameterValue> values = m_LobbyManager.GetScenarioParameterValues();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int visibleCount;
		m_iEditableRowCount = 0;
		int definitionIndex;
		foreach (GC_LobbyParameterDefinition definition : definitions)
		{
			if (!selectedFaction || !definition.IsVisibleToFaction(selectedFaction))
			{
				definitionIndex++;
				continue;
			}

			GC_LobbyParameterValue value;
			foreach (GC_LobbyParameterValue candidate : values)
			{
				if (candidate && candidate.GetDefinitionIndex() == definitionIndex)
				{
					value = candidate;
					break;
				}
			}

			if (!value || !workspace)
			{
				definitionIndex++;
				continue;
			}

			Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wRows);
			GC_BriefingParameterRowComponent row;
			if (rowRoot)
				row = GC_BriefingParameterRowComponent.Cast(rowRoot.FindHandler(GC_BriefingParameterRowComponent));
			if (!row)
			{
				if (rowRoot)
					rowRoot.RemoveFromHierarchy();
				definitionIndex++;
				continue;
			}

			bool selectedFactionOwnsDefinition = definition.GetOwnerFactionKey() == m_SelectedFactionKey;
			bool rowEditable = selectedFactionOwnsDefinition && m_bEditable
				&& m_LobbyManager.CanModifyScenarioParameter(playerId, definitionIndex);
			row.Configure(definition, value, rowEditable, definitionIndex);
			row.m_OnChanged.Insert(OnRowChanged);
			m_aRows.Insert(row);
			if (rowEditable)
				m_iEditableRowCount++;
			visibleCount++;
			definitionIndex++;
		}

		if (m_wStatus)
		{
			if (m_bStale)
				m_wStatus.SetText("Parameters changed on the server. Leave and reopen this view to continue.");
			else if (m_LobbyManager.GetLobbyPhase() == SCR_EGameModeState.GAME || m_LobbyManager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
				m_wStatus.SetText("Parameters are locked after the briefing phase.");
			else if (!selectedFaction)
				m_wStatus.SetText("Select a faction to view its parameters.");
			else if (visibleCount == 0)
				m_wStatus.SetText("No parameters are available for the selected faction.");
			else if (playerController && SCR_Global.IsAdmin(playerId) && m_iEditableRowCount == 0)
				m_wStatus.SetText("No parameters can be modified for the selected faction.");
			else if (!m_bEditable)
				m_wStatus.SetText("Read-only parameter snapshot.");
			else
				m_wStatus.SetText(string.Empty);
		}

		if (m_ApplyButton)
			m_ApplyButton.SetEnabled(m_bEditable && m_bDirty && !m_bStale && m_iEditableRowCount > 0);
	}

	protected void ClearRows()
	{
		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			if (row)
				row.m_OnChanged.Remove(OnRowChanged);
			if (row && row.GetWidgetRoot())
				row.GetWidgetRoot().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}

	protected void OnRowChanged(GC_BriefingParameterRowComponent row)
	{
		if (!row || !row.IsEditable())
			return;

		m_bDirty = true;
		if (m_wStatus && m_LobbyManager && !m_bStale)
		{
			SCR_EGameModeState phase = m_LobbyManager.GetLobbyPhase();
			if (phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING)
				m_wStatus.SetText(string.Empty);
		}
		if (m_ApplyButton)
			m_ApplyButton.SetEnabled(m_bEditable && !m_bStale && m_iEditableRowCount > 0);
	}

	protected void OnParametersChanged(int revision)
	{
		if (revision != m_iBaseRevision && m_bDirty)
		{
			m_bStale = true;
			if (m_wStatus)
				m_wStatus.SetText("Parameters changed on the server. Leave and reopen this view to continue.");
			if (m_ApplyButton)
				m_ApplyButton.SetEnabled(false);
			return;
		}

		if (!m_bDirty)
		{
			m_iBaseRevision = revision;
			RefreshRows();
		}
	}

	protected void OnLobbyPhaseChanged(SCR_EGameModeState phase)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		int playerId = -1;
		if (playerController)
			playerId = playerController.GetPlayerId();
		m_bEditable = playerController && SCR_Global.IsAdmin(playerId)
			&& (phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING);
		m_iEditableRowCount = 0;

		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			if (!row)
				continue;

			GC_LobbyParameterDefinition definition = m_LobbyManager.GetScenarioParameterCatalog().FindDefinition(row.GetDefinitionIndex());
			bool selectedFactionOwnsDefinition = definition && definition.GetOwnerFactionKey() == m_SelectedFactionKey;
			bool rowEditable = selectedFactionOwnsDefinition && m_bEditable
				&& m_LobbyManager.CanModifyScenarioParameter(playerId, row.GetDefinitionIndex());
			row.SetEditable(rowEditable);
			if (rowEditable)
				m_iEditableRowCount++;
		}

		if (m_wStatus && !m_bStale)
		{
			if (phase != SCR_EGameModeState.PREGAME && phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
				m_wStatus.SetText("Parameters are locked after the briefing phase.");
			else if (!m_bEditable)
				m_wStatus.SetText("Read-only parameter snapshot.");
			else if (m_iEditableRowCount == 0)
				m_wStatus.SetText("No parameters can be modified for the selected faction.");
			else
				m_wStatus.SetText(string.Empty);
		}
		if (m_ApplyButton)
			m_ApplyButton.SetEnabled(m_bEditable && m_bDirty && !m_bStale && m_iEditableRowCount > 0);
	}

	protected void OnPlayerRoleChanged(int playerId, EPlayerRole roleFlags)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		int localPlayerId = -1;
		if (playerController)
			localPlayerId = playerController.GetPlayerId();

		bool queriedAdmin = playerController && SCR_Global.IsAdmin(localPlayerId);
		PrintFormat("GC | Admin Trace | parameters role event: changedPlayer=%1 localPlayer=%2 roleFlags=%3 flagsAdmin=%4 queriedAdmin=%5 isLocal=%6 manager=%7", playerId, localPlayerId, roleFlags, SCR_Global.IsAdminRole(roleFlags), queriedAdmin, localPlayerId == playerId, m_LobbyManager);
		if (!playerController || localPlayerId != playerId || !m_LobbyManager)
			return;

		OnLobbyPhaseChanged(m_LobbyManager.GetLobbyPhase());
		PrintFormat("GC | Admin Trace | parameters role applied: player=%1 admin=%2 phase=%3 editable=%4 editableRows=%5 dirty=%6 stale=%7", playerId, SCR_Global.IsAdmin(localPlayerId), GC_LobbyManager.GetPhaseName(m_LobbyManager.GetLobbyPhase()), m_bEditable, m_iEditableRowCount, m_bDirty, m_bStale);
	}

	protected void OnApplyClicked(SCR_ButtonBaseComponent button)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		bool isAdmin = playerController && SCR_Global.IsAdmin(playerController.GetPlayerId());
		PrintFormat("GC | Admin Trace | parameters apply dispatched: admin=%1 editable=%2 dirty=%3 stale=%4 editableRows=%5 manager=%6", isAdmin, m_bEditable, m_bDirty, m_bStale, m_iEditableRowCount, m_LobbyManager);
		if (!isAdmin || !m_bEditable || !m_bDirty || m_bStale || !m_LobbyManager)
			return;

		SCR_EGameModeState phase = m_LobbyManager.GetLobbyPhase();
		if (phase != SCR_EGameModeState.PREGAME && phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
			return;

		array<ref GC_LobbyParameterValue> values = {};
		foreach (GC_LobbyParameterValue currentValue : m_LobbyManager.GetScenarioParameterValues())
		{
			if (!currentValue)
				continue;

			GC_LobbyParameterValue draftValue = new GC_LobbyParameterValue();
			draftValue.CopyFrom(currentValue);
			values.Insert(draftValue);
		}

		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			GC_LobbyParameterValue value = row.BuildValue();
			if (!value)
			{
				if (m_wStatus)
					m_wStatus.SetText("Invalid parameter input. Correct it before applying.");
				if (m_ApplyButton)
					m_ApplyButton.SetEnabled(false);
				return;
			}

			bool replaced;
			for (int valueIndex; valueIndex < values.Count(); valueIndex++)
			{
				if (values[valueIndex].GetDefinitionIndex() != value.GetDefinitionIndex())
					continue;

				values[valueIndex] = value;
				replaced = true;
				break;
			}

			if (!replaced)
				values.Insert(value);
		}

		if (values.Count() != m_LobbyManager.GetScenarioParameterDefinitions().Count()
			|| !m_LobbyManager.GetScenarioParameterCatalog().ValidateValues(values))
		{
			if (m_wStatus)
				m_wStatus.SetText("Invalid parameter input. Correct it before applying.");
			if (m_ApplyButton)
				m_ApplyButton.SetEnabled(false);
			return;
		}

		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			if (m_wStatus)
				m_wStatus.SetText("Unable to submit parameters from this client.");
			return;
		}

		// Clear the local draft before dispatch. A listen-server RPC can publish
		// the new revision synchronously; leaving this true until after the call
		// incorrectly marks our own accepted update as a conflicting stale edit.
		m_bDirty = false;
		if (m_ApplyButton)
			m_ApplyButton.SetEnabled(false);
		if (m_wStatus)
			m_wStatus.SetText("Parameter update submitted.");
		controller.RequestApplyScenarioParameters(m_iBaseRevision, values);
	}
}
