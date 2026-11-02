//! Side selection is presentation only. Every changed value is authorized again
//! by the manager; pending drafts survive until acknowledgement AND replication.
class GC_LobbyParametersComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName ROW_LAYOUT = "{6A4D000000000008}UI/layouts/Briefing/Lobby_Briefing_Parameter_Row.layout";
	protected static const ResourceName SIDE_LAYOUT = "{A592C710D3314B80}UI/layouts/Admin/Lobby_AdminIconButton.layout";
	protected Widget m_wRows;
	protected Widget m_wSides;
	protected ScrollLayoutWidget m_wScroll;
	protected TextWidget m_wStatus;
	protected TextWidget m_wLock;
	protected TextWidget m_wPermission;
	protected SCR_ButtonBaseComponent m_ApplyButton;
	protected SCR_ButtonBaseComponent m_RevertButton;
	protected GC_LobbyManager m_LobbyManager;
	protected GC_LobbyPlayerController m_Controller;
	protected ref array<GC_BriefingParameterRowComponent> m_aRows = {};
	protected ref array<SCR_ButtonTextComponent> m_aSideButtons = {};
	protected ref array<string> m_aSideKeys = {};
	protected FactionKey m_SelectedFactionKey;
	protected int m_iBaseRevision;
	protected bool m_bDirty;
	protected bool m_bStale;
	protected bool m_bSubmitting;
	protected int m_iAcceptedRevision = -1;
	protected int m_iInitializeAttempts;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRows = w.FindAnyWidget("ScenarioParametersRows");
		m_wSides = w.FindAnyWidget("ParametersSides");
		m_wScroll = ScrollLayoutWidget.Cast(w.FindAnyWidget("ScenarioParametersScroll"));
		m_wStatus = TextWidget.Cast(w.FindAnyWidget("ScenarioParametersStatus"));
		m_wLock = TextWidget.Cast(w.FindAnyWidget("ParametersLock"));
		m_wPermission = TextWidget.Cast(w.FindAnyWidget("ParametersPermission"));
		m_ApplyButton = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("ScenarioParametersApply").FindHandler(SCR_ButtonBaseComponent));
		m_RevertButton = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("ScenarioParametersRevert").FindHandler(SCR_ButtonBaseComponent));
		m_ApplyButton.m_OnClicked.Insert(OnApplyClicked);
		m_RevertButton.m_OnClicked.Insert(OnRevertClicked);
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnScenarioParametersChanged.Insert(OnParametersChanged);
			m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnFactionsChanged);
		}
		GetGame().GetCallqueue().CallLater(InitializeView, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(InitializeView);
		GetGame().GetCallqueue().Remove(UpdateAccess);
		if (m_Controller)
			m_Controller.m_OnScenarioParameterResult.Remove(OnSubmissionResult);
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnScenarioParametersChanged.Remove(OnParametersChanged);
			m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnFactionsChanged);
		}
		if (m_ApplyButton)
			m_ApplyButton.m_OnClicked.Remove(OnApplyClicked);
		if (m_RevertButton)
			m_RevertButton.m_OnClicked.Remove(OnRevertClicked);
		ClearRows();
		ClearSides();
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializeView()
	{
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!m_Controller || !m_LobbyManager || !m_LobbyManager.IsScenarioParameterCatalogValid())
		{
			m_iInitializeAttempts++;
			if (m_iInitializeAttempts < 20)
				GetGame().GetCallqueue().CallLater(InitializeView, 250);
			else
				SetStatus("Parameters unavailable.");
			return;
		}
		m_Controller.m_OnScenarioParameterResult.Insert(OnSubmissionResult);
		Faction faction = m_LobbyManager.GetPlayerFaction(m_Controller.GetPlayerId());
		if (faction)
			m_SelectedFactionKey = faction.GetFactionKey();
		BuildSides();
		LoadCurrentValues();
		GetGame().GetCallqueue().CallLater(UpdateAccess, 500, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearSides()
	{
		foreach (SCR_ButtonTextComponent button : m_aSideButtons)
		{
			if (!button)
				continue;
			button.m_OnClicked.Remove(OnSideClicked);
			if (button.GetRootWidget())
				button.GetRootWidget().RemoveFromHierarchy();
		}
		m_aSideButtons.Clear();
		m_aSideKeys.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildSides()
	{
		ClearSides();
		foreach (Faction faction : m_LobbyManager.GetRegisteredFactions())
		{
			if (!faction)
				continue;
			string key = faction.GetFactionKey();
			Widget root = GetGame().GetWorkspace().CreateWidgets(SIDE_LAYOUT, m_wSides);
			root.SetName("ParameterSide" + key);
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(root.FindHandler(SCR_ButtonTextComponent));
			button.SetText(faction.GetFactionName());
			button.m_OnClicked.Insert(OnSideClicked);
			m_aSideButtons.Insert(button);
			m_aSideKeys.Insert(key);
		}
		if (!m_aSideKeys.Contains(m_SelectedFactionKey) && !m_aSideKeys.IsEmpty())
			m_SelectedFactionKey = m_aSideKeys[0];
		UpdateSideSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSideSelection()
	{
		foreach (int index, SCR_ButtonTextComponent button : m_aSideButtons)
			button.SetToggled(m_aSideKeys[index] == m_SelectedFactionKey, false, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionsChanged(Faction faction)
	{
		if (!m_Controller)
			return;
		BuildSides();
		if (!m_bDirty && !m_bSubmitting)
			LoadCurrentValues();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSideClicked(SCR_ButtonBaseComponent button)
	{
		int index = m_aSideButtons.Find(SCR_ButtonTextComponent.Cast(button));
		if (index < 0)
			return;
		if (m_bDirty || m_bSubmitting)
			SetStatus("Apply or revert before switching sides.");
		else if (m_SelectedFactionKey != m_aSideKeys[index])
		{
			m_SelectedFactionKey = m_aSideKeys[index];
			LoadCurrentValues();
			m_wScroll.SetSliderPos(0, 0);
		}
		UpdateSideSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			if (!row)
				continue;
			row.m_OnChanged.Remove(OnRowChanged);
			if (row.GetWidgetRoot())
				row.GetWidgetRoot().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadCurrentValues()
	{
		ClearRows();
		m_bDirty = false;
		m_bStale = false;
		m_iBaseRevision = m_LobbyManager.GetScenarioParameterRevision();
		if (!m_wRows)
			return;
		foreach (int index, GC_LobbyParameterDefinition definition : m_LobbyManager.GetScenarioParameterDefinitions())
		{
			if (definition.GetOwnerFactionKey() != m_SelectedFactionKey)
				continue;
			GC_LobbyParameterValue value = m_LobbyManager.GetScenarioParameterCatalog().FindValue(index);
			Widget root = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wRows);
			GC_BriefingParameterRowComponent row = GC_BriefingParameterRowComponent.Cast(root.FindHandler(GC_BriefingParameterRowComponent));
			row.Configure(definition, value, false, index);
			row.m_OnChanged.Insert(OnRowChanged);
			m_aRows.Insert(row);
		}
		SetStatus("");
		if (m_aRows.IsEmpty())
			SetStatus("No parameters for this side.");
		UpdateAccess();
	}

	//------------------------------------------------------------------------------------------------
	//! Updates permissions without rebuilding rows or discarding a focused draft.
	protected void UpdateAccess()
	{
		if (!m_Controller || !m_LobbyManager || GC_LobbyManager.GetManager() != m_LobbyManager)
			return;
		int playerId = m_Controller.GetPlayerId();
		int editable;
		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			bool allowed = !m_bSubmitting && !m_bStale && m_LobbyManager.CanModifyScenarioParameter(playerId, row.GetDefinitionIndex());
			if (row.IsEditable() != allowed)
				row.SetEditable(allowed);
			if (allowed)
				editable++;
		}
		int remaining = m_LobbyManager.GC_GetParameterLockRemaining();
		string lockText = "CO choices lock during briefing";
		if (remaining > 0)
			lockText = string.Format("CO choices lock in %1:%2", remaining / 60, (remaining % 60).ToString(2));
		else if (remaining == 0)
			lockText = "Locked · admins only";
		else if (remaining == -2)
			lockText = "Synchronizing deadline…";
		if (m_LobbyManager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			lockText = "Mission ended";
		m_wLock.SetText(lockText);
		if (remaining > 0 && remaining <= 30)
			m_wLock.SetColor(Color.FromRGBA(242, 186, 77, 255));
		else
			m_wLock.SetColor(Color.FromRGBA(220, 220, 220, 255));
		string permission = "View only";
		if (SCR_Global.IsAdmin(playerId) && m_LobbyManager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME)
			permission = "Admin · edit either side";
		else if (editable > 0)
			permission = "CO · edit your side";
		m_wPermission.SetText(permission);
		m_ApplyButton.SetEnabled(editable > 0 && m_bDirty && !m_bSubmitting && !m_bStale);
		m_RevertButton.SetEnabled((m_bDirty || m_bStale) && !m_bSubmitting);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowChanged(GC_BriefingParameterRowComponent row)
	{
		if (!row || !row.IsEditable())
			return;
		m_bDirty = true;
		SetStatus("Unsaved changes");
		UpdateAccess();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRevertClicked(SCR_ButtonBaseComponent button)
	{
		if (!m_bSubmitting)
			LoadCurrentValues();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnParametersChanged(int revision)
	{
		if (m_bSubmitting)
		{
			if (m_iAcceptedRevision >= 0 && revision >= m_iAcceptedRevision)
				CompleteSubmission();
			return;
		}
		if (revision != m_iBaseRevision && m_bDirty)
		{
			m_bStale = true;
			SetStatus("Parameters changed. Revert to load the latest values.");
			UpdateAccess();
		}
		else if (!m_bDirty)
			LoadCurrentValues();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSubmissionResult(bool accepted, int revision)
	{
		if (!m_bSubmitting)
			return;
		if (accepted)
		{
			m_iAcceptedRevision = revision;
			if (m_LobbyManager.GetScenarioParameterRevision() >= revision)
				CompleteSubmission();
			return;
		}
		m_bSubmitting = false;
		m_bStale = revision != m_iBaseRevision;
		SetStatus("Not applied. Check access or revert to the latest values.");
		UpdateAccess();
	}

	//------------------------------------------------------------------------------------------------
	protected void CompleteSubmission()
	{
		m_bSubmitting = false;
		m_iAcceptedRevision = -1;
		LoadCurrentValues();
		SetStatus("Applied");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnApplyClicked(SCR_ButtonBaseComponent button)
	{
		if (!m_Controller || !m_bDirty || m_bStale || m_bSubmitting)
			return;
		array<ref GC_LobbyParameterValue> values = {};
		foreach (GC_LobbyParameterValue current : m_LobbyManager.GetScenarioParameterValues())
		{
			GC_LobbyParameterValue copy = new GC_LobbyParameterValue();
			copy.CopyFrom(current);
			values.Insert(copy);
		}
		foreach (GC_BriefingParameterRowComponent row : m_aRows)
		{
			GC_LobbyParameterValue draft = row.BuildValue();
			if (!draft)
			{
				SetStatus("Check the entered value.");
				return;
			}
			values[draft.GetDefinitionIndex()] = draft;
		}
		if (!m_LobbyManager.GetScenarioParameterCatalog().ValidateValues(values))
		{
			SetStatus("Check the values and allowed ranges.");
			return;
		}
		m_bSubmitting = true;
		m_iAcceptedRevision = -1;
		SetStatus("Applying…");
		UpdateAccess();
		m_Controller.RequestApplyScenarioParameters(m_iBaseRevision, values);
	}
}
