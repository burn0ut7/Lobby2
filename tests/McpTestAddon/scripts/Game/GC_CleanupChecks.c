//! Opt-in checks loaded only through the separate Lobby2McpTests project.
//! Run GC_RunCleanupChecks on the example world's GC_LobbyManager in Play.
//! No camera/voice capture or persistent world mutation is performed.
#ifdef WORKBENCH
class GC_CleanupParameterChecks : GC_LobbyParametersComponent
{
	//------------------------------------------------------------------------------------------------
	bool Run(GC_LobbyManager manager)
	{
		m_LobbyManager = manager;
		m_iBaseRevision = manager.GetScenarioParameterRevision();
		m_bDirty = true;
		m_bSubmitting = true;
		OnSubmissionResult(false, m_iBaseRevision);
		bool rejectionRetainsDraft = m_bDirty && !m_bSubmitting && !m_bStale;
		PrintFormat("GC | Cleanup Test | rejected-draft-retained pass=%1", rejectionRetainsDraft);

		m_bSubmitting = true;
		OnSubmissionResult(false, m_iBaseRevision + 1);
		bool conflictRetainsDraft = m_bDirty && m_bStale && !m_bSubmitting;
		PrintFormat("GC | Cleanup Test | conflicting-draft-retained pass=%1", conflictRetainsDraft);

		m_bStale = false;
		m_bSubmitting = true;
		m_iAcceptedRevision = -1;
		OnParametersChanged(m_iBaseRevision);
		bool snapshotWaitsForResult = m_bSubmitting && m_bDirty && !m_bStale;
		OnSubmissionResult(true, m_iBaseRevision);
		bool snapshotBeforeResult = snapshotWaitsForResult && !m_bSubmitting && !m_bDirty;
		PrintFormat("GC | Cleanup Test | snapshot-before-result pass=%1", snapshotBeforeResult);

		m_bDirty = true;
		m_bSubmitting = true;
		OnSubmissionResult(true, m_iBaseRevision + 1);
		bool resultWaitsForSnapshot = m_bSubmitting && m_bDirty;
		PrintFormat("GC | Cleanup Test | result-waits-for-snapshot pass=%1", resultWaitsForSnapshot);
		m_bSubmitting = false;
		m_LobbyManager = null;
		return rejectionRetainsDraft && conflictRetainsDraft && snapshotBeforeResult && resultWaitsForSnapshot;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Diagnostic reproducer for the default end-data parameter's lifetime.
	string GC_TestEndDataLifetime()
	{
		return GC_TestCreateEndData(null);
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestCreateEndData(SCR_GameModeEndData endData)
	{
		endData = SCR_GameModeEndData.CreateSimple();
		bool parameterAlive = endData != null;
		ref SCR_GameModeEndData ownedData = SCR_GameModeEndData.CreateSimple();
		return string.Format("parameterAlive=%1 ownedDataAlive=%2", parameterAlive, ownedData != null);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_RunCleanupChecks()
	{
		array<SCR_AIGroup> groups = GetRegisteredGroups();
		if (groups.Count() < 2)
			return false;

		GC_LobbyRoster roster = new GC_LobbyRoster();
		roster.QueueJipGroup(Replication.FindItemId(groups[0]));
		roster.QueueJipGroup(Replication.FindItemId(groups[1]));
		bool wasEmpty;
		roster.AddGroup(groups[1], wasEmpty);
		bool partialAvailable = roster.GetGroups().Count() == 1 && roster.GetGroups()[0] == groups[1];
		roster.AddGroup(groups[0], wasEmpty);
		bool finalOrder = roster.GetGroups()[0] == groups[0] && roster.GetGroups()[1] == groups[1];
		PrintFormat("GC | Cleanup Test | reverse-arrival-roster-order pass=%1", partialAvailable && finalOrder);

		int groupCount = GetRegisteredGroups().Count();
		int slotCount = GetSlots().Count();
		int briefingCount = GetBriefings().Count();
		OnGameModeEnd(null);
		bool phaseRetainsData = !m_bShutdown && GetRegisteredGroups().Count() == groupCount
			&& GetSlots().Count() == slotCount && GetBriefings().Count() == briefingCount;
		PrintFormat("GC | Cleanup Test | mode-end-retains-data pass=%1 groups=%2 slots=%3 briefings=%4", phaseRetainsData, groupCount, slotCount, briefingCount);

		GC_CleanupParameterChecks parameterChecks = new GC_CleanupParameterChecks();
		return parameterChecks.Run(this) && partialAvailable && finalOrder && phaseRetainsData;
	}
}
#endif
