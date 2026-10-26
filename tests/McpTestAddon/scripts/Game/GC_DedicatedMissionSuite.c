//! Explicit native autotest; never runs merely because the test addon is loaded.
class GC_DedicatedMissionSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	override ResourceName GetWorldFile()
	{
		return string.Empty;
	}
}

[Test(suite: GC_DedicatedMissionSuite, timeoutMs: 240000)]
class GC_DedicatedMissionReplication : SCR_AutotestCaseBase
{
	protected GC_LobbyManager m_Manager;
	protected int m_iStage;
	protected int m_iStarted;
	protected int m_iNextCheck;
	protected ref array<int> m_aPlayers = {};

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.Setup)]
	void Prepare()
	{
		string requested;
		System.GetCLIParam("autotest", requested);
		AssertTrue(requested == "GC_DedicatedMissionSuite" && RplSession.Mode() == RplMode.Dedicated,
			"Requires explicit dedicated test selection");
		m_iStarted = System.GetTickCount();
	}

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.Main)]
	bool Execute()
	{
		if (GetFailure())
			return true;
		int now = System.GetTickCount();
		if (now < m_iNextCheck)
			return false;
		m_iNextCheck = now + 1000;
		if (now - m_iStarted > 180000)
		{
			SetFailure("Mission replication timeout at stage " + m_iStage);
			return true;
		}
		if (m_iStage == 0)
		{
			m_Manager = GC_LobbyManager.GetManager();
			if (!m_Manager || !m_Manager.GC_GetMissionRuntime())
				return false;
			array<int> alreadyJoined = {};
			GetGame().GetPlayerManager().GetPlayers(alreadyJoined);
			AssertTrue(alreadyJoined.IsEmpty(), "Set flag before any clients join for snapshot coverage");
			GC_LobbyMissionContext context = new GC_LobbyMissionContext(m_Manager);
			GC_SetFlagAction setFlag = new GC_SetFlagAction();
			setFlag.m_sFlag = "Interaction complete";
			setFlag.m_bSet = true;
			context.Bind(m_Manager.GC_GetMissionRuntime());
			AssertTrue(setFlag.Execute(context), "Set initial replicated flag");
			m_Manager.SetBriefing(32, "Network test", "Snapshot", null, SCR_EGameModeState.PREGAME, false, false);
			Print("MISSION NETWORK: initial state set before joining");
			m_iStage = 1;
			return false;
		}
		if (m_iStage == 1)
		{
			GetGame().GetPlayerManager().GetPlayers(m_aPlayers);
			if (m_aPlayers.Count() != 2)
				return false;
			foreach (int id : m_aPlayers)
			{
				if (!GetGame().GetPlayerManager().GetPlayerControlledEntity(id))
					return false;
			}
			foreach (int id : m_aPlayers)
				AssertTrue(m_Manager.GC_TestPeerCommand(id, "mission-initial-" + id, "mission-framework-snapshot", "").StartsWith("DISPATCHED"), "Request joining client snapshot");
			m_iStage = 2;
			return false;
		}
		if (m_iStage == 2 || m_iStage == 4)
		{
			string prefix = "mission-initial-";
			string expected = "flag=1 briefingHidden=1 authority=0";
			if (m_iStage == 4)
			{
				prefix = "mission-update-";
				expected = "flag=0 briefingHidden=0 authority=0";
			}
			foreach (int id : m_aPlayers)
			{
				string report = m_Manager.GC_TestPeerReport(prefix + id);
				if (report.StartsWith("PENDING"))
					return false;
				AssertTrue(report.Contains(expected), "Remote state: " + report);
			}
			if (m_iStage == 4)
			{
				Print("MISSION NETWORK PASS: joining snapshots and live updates on two remote clients");
				return true;
			}
			m_Manager.GC_GetMissionRuntime().SetFlag("Interaction complete", false);
			GC_LobbyMissionContext context = new GC_LobbyMissionContext(m_Manager);
			GC_SetBriefingVisibilityAction visibility = new GC_SetBriefingVisibilityAction();
			visibility.m_iBriefing = 32;
			visibility.m_bVisible = true;
			AssertTrue(visibility.Execute(context), "Show briefing through production action");
			m_iStage = 3;
			m_iNextCheck = now + 3000;
			return false;
		}
		foreach (int id : m_aPlayers)
			AssertTrue(m_Manager.GC_TestPeerCommand(id, "mission-update-" + id, "mission-framework-snapshot", "").StartsWith("DISPATCHED"), "Request updated client snapshot");
		m_iStage = 4;
		return false;
	}
}
