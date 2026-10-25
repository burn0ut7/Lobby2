//! Explicit native -autotest entry point. Loading the test addon alone runs nothing.
class GC_DedicatedAdminSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	override ResourceName GetWorldFile()
	{
		// Dedicated Server Tool owns loading the selected Everon scenario.
		return string.Empty;
	}
}

[Test(suite: GC_DedicatedAdminSuite, timeoutMs: 240000)]
class GC_DedicatedAdminRange : SCR_AutotestCaseBase
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_AdminController;
	protected PlayerController m_TargetController;
	protected int m_iAdmin;
	protected int m_iTarget;
	protected int m_iStage;
	protected int m_iNextCheck;
	protected int m_iStageStarted;
	protected bool m_bRoleOwned;
	protected string m_sAdminIdentity;
	protected string m_sTargetIdentity;

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.Setup)]
	void Prepare()
	{
		m_iStage = 0;
		m_iStageStarted = System.GetTickCount();
		m_iNextCheck = 0;
		m_bRoleOwned = false;
		string requestedSuite;
		System.GetCLIParam("autotest", requestedSuite);
		AssertTrue(requestedSuite == "GC_DedicatedAdminSuite" && RplSession.Mode() == RplMode.Dedicated,
			"Requires explicit native suite selection on a dedicated server");
		Print("GC dedicated range: native runner active; embedded Play is not used.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Next()
	{
		Print(string.Format("GC dedicated range: stage %1 verified", m_iStage));
		m_iStage++;
		m_iStageStarted = System.GetTickCount();
		m_iNextCheck = m_iStageStarted + 2000;
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(int playerId, string token, string action, string message)
	{
		string result = m_Manager.GC_TestPeerCommand(playerId, token, action, message);
		AssertTrue(result.StartsWith("DISPATCHED"), "Peer command dispatch: " + result);
	}

	//------------------------------------------------------------------------------------------------
	protected bool Report(string token, string expected)
	{
		string report = m_Manager.GC_TestPeerReport(token);
		if (report.StartsWith("PENDING"))
			return false;
		Print(token + ": " + report);
		AssertTrue(report.StartsWith("CLIENT-REPORTED") && report.Contains(expected), "Unexpected peer result: " + report);
		return !GetFailure();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Living(int playerId)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		return character && !character.FindComponent(GC_LobbySpectatorBodyComponent);
	}

	//------------------------------------------------------------------------------------------------
	protected float Health(int playerId)
	{
		if (!Living(playerId))
			return -1;
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return -1;
		return damage.GetHealthScaled();
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
		m_iNextCheck = now + 500;
		int deadline = 30000;
		if (m_iStage == 0)
			deadline = 120000;
		if (now - m_iStageStarted > deadline)
		{
			SetFailure("Stage timeout: " + m_iStage.ToString());
			return true;
		}

		PlayerManager players = GetGame().GetPlayerManager();
		if (m_iStage == 0)
		{
			m_Manager = GC_LobbyManager.GetManager();
			array<int> connected = {};
			players.GetPlayers(connected);
			if (!m_Manager || connected.Count() < 2)
				return false;
			if (connected.Count() != 2 || m_Manager.GetLobbyPhase() != SCR_EGameModeState.PREGAME)
			{
				SetFailure("Requires a fresh disposable Lobby2 server with exactly two clients");
				return true;
			}
			connected.Sort();
			m_iAdmin = connected[0];
			m_iTarget = connected[1];
			m_AdminController = GC_LobbyPlayerController.Cast(players.GetPlayerController(m_iAdmin));
			if (!m_AdminController || !players.GetPlayerController(m_iTarget) || !players.GetPlayerControlledEntity(m_iTarget))
				return false;
			if (SCR_Global.IsAdmin(m_iAdmin))
			{
				SetFailure("Refusing to modify an existing admin role");
				return true;
			}
			m_sAdminIdentity = m_Manager.GetPlayerVoiceIdentity(m_iAdmin);
			m_sTargetIdentity = m_Manager.GetPlayerVoiceIdentity(m_iTarget);
			if (m_sAdminIdentity.IsEmpty() || m_sTargetIdentity.IsEmpty())
				return false;
			m_TargetController = players.GetPlayerController(m_iTarget);
			m_bRoleOwned = players.GivePlayerRole(m_iAdmin, EPlayerRole.SESSION_ADMINISTRATOR);
			GC_AdminActionLogger.Log(m_iAdmin, "test-dedicated-admin-role", "grant", "changed=" + m_bRoleOwned.ToString());
			AssertTrue(m_bRoleOwned && SCR_Global.IsAdmin(m_iAdmin), "Native test role setup");
			AssertTrue(m_Manager.TryAdvanceLobbyPhase(m_iAdmin, SCR_EGameModeState.SLOTTING), "Slotting setup");
			Next();
			return false;
		}
		if (GC_LobbyManager.GetManager() != m_Manager || !players.IsPlayerConnected(m_iAdmin) || !players.IsPlayerConnected(m_iTarget)
			|| players.GetPlayerController(m_iAdmin) != m_AdminController || players.GetPlayerController(m_iTarget) != m_TargetController
			|| m_Manager.GetPlayerVoiceIdentity(m_iAdmin) != m_sAdminIdentity || m_Manager.GetPlayerVoiceIdentity(m_iTarget) != m_sTargetIdentity)
		{
			SetFailure("Test participant or world changed");
			return true;
		}

		switch (m_iStage)
		{
			case 1:
				AssertTrue(m_Manager.ExecuteAdminGroupLock(m_iAdmin, RplId.Invalid(), false).StartsWith("applied"), "Unlock fixture slots");
				AssertTrue(m_Manager.ExecuteAdminAssignSlot(m_iAdmin, m_iAdmin, m_sAdminIdentity, 5, "") == "applied", "Assign admin fixture");
				AssertTrue(m_Manager.ExecuteAdminAssignSlot(m_iAdmin, m_iTarget, m_Manager.GetPlayerVoiceIdentity(m_iTarget), 6, "") == "applied", "Assign target fixture");
				AssertTrue(m_Manager.TryAdvanceLobbyPhase(m_iAdmin, SCR_EGameModeState.BRIEFING), "Briefing setup");
				break;
			case 2:
				AssertTrue(m_Manager.TryAdvanceLobbyPhase(m_iAdmin, SCR_EGameModeState.GAME), "Game setup");
				break;
			case 3:
				if (!Living(m_iAdmin) || !Living(m_iTarget))
					return false;
				Send(m_iTarget, "ded-move", "scope-move", "4500 0 8000");
				break;
			case 4:
				if (!Report("ded-move", "fixtureMove=1"))
					return false;
				Send(m_iAdmin, "ded-absence", "scope-snapshot", m_iTarget.ToString());
				break;
			case 5:
				if (!Report("ded-absence", "server=0 connected=2 listed=1 identityReady=1 character=0 slotEntity=0 replicationItem=0 faction=USSR"))
					return false;
				Print(m_Manager.GC_TestAdminScopeDamage(m_iTarget));
				AssertTrue(Health(m_iTarget) == 0.5, "Damaged target fixture");
				Send(m_iAdmin, "ded-picker", "scope-picker", m_iTarget.ToString());
				break;
			case 6:
				if (!Report("ded-picker", "listed=1 selectable=1 character=0"))
					return false;
				Send(m_iAdmin, "ded-apply", "scope-ui", "click|ToolApply");
				break;
			case 7:
				if (Health(m_iTarget) != 1)
					return false;
				Print("PASS dedicated client selected Heal: target health restored to 1");
				Print(m_Manager.GC_TestAdminScopeDamage(m_iTarget));
				AssertTrue(Health(m_iTarget) == 0.5, "Everyone heal damage fixture");
				Send(m_iAdmin, "ded-everyone", "scope-heal", "everyone|" + m_iTarget.ToString());
				break;
			case 8:
				if (!Report("ded-everyone", "character=0 slotEntity=0 replicationItem=0") || !Report("ded-everyone", "outcome=Healed"))
					return false;
				if (Health(m_iTarget) != 1)
					return false;
				Print("PASS dedicated client Everyone Heal: target health restored to 1");
				Print(m_Manager.GC_TestAdminScopeDamage(m_iTarget));
				AssertTrue(Health(m_iTarget) == 0.5, "Faction heal damage fixture");
				Send(m_iAdmin, "ded-faction", "scope-heal", "faction|" + m_iTarget.ToString());
				break;
			case 9:
				if (!Report("ded-faction", "character=0 slotEntity=0 replicationItem=0") || !Report("ded-faction", "outcome=Healed"))
					return false;
				if (Health(m_iTarget) != 1)
					return false;
				Print("PASS dedicated client Faction Heal: target health restored to 1");
				Send(m_iAdmin, "ded-moderation-open", "scope-ui", "click|ToolModeration");
				break;
			case 10:
				Send(m_iAdmin, "ded-moderation-row", "scope-ui", "moderation|" + m_iTarget.ToString());
				break;
			case 11:
				if (!Report("ded-moderation-row", "rows=2 selected=1 identityReady=1 kickEnabled=1 banEnabled=1"))
					return false;
				Send(m_iAdmin, "ded-pre-teleport", "scope-snapshot", m_iTarget.ToString());
				break;
			case 12:
				if (!Report("ded-pre-teleport", "character=0 slotEntity=0 replicationItem=0"))
					return false;
				Send(m_iAdmin, "ded-teleport", "teleport-to-player", m_iTarget.ToString());
				break;
			case 13:
				if (vector.Distance(players.GetPlayerControlledEntity(m_iAdmin).GetOrigin(), players.GetPlayerControlledEntity(m_iTarget).GetOrigin()) > 6)
					return false;
				Print("PASS dedicated client teleport: authority verified arrival within 6m");
				Send(m_iAdmin, "ded-arrival", "scope-snapshot", m_iTarget.ToString());
				break;
			case 14:
				if (!Report("ded-arrival", "character=1 slotEntity=1 replicationItem=1"))
					return false;
				Print(m_Manager.GC_TestAdminScopeDamage(m_iTarget));
				RevokeRole();
				Send(m_iAdmin, "ded-denied", "scope-heal", "everyone|" + m_iTarget.ToString());
				break;
			case 15:
				if (!Report("ded-denied", "outcome=Admin login required."))
					return false;
				AssertTrue(!SCR_Global.IsAdmin(m_iAdmin) && Health(m_iTarget) == 0.5, "Logged-out client must not heal target");
				if (!GetFailure())
					Print("PASS dedicated admin range suite; inspect correlated admin audit for all outcomes");
				return true;
		}
		Next();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.TearDown)]
	void RevokeRole()
	{
		if (!m_bRoleOwned)
			return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (players.GetPlayerController(m_iAdmin) != m_AdminController || !m_Manager
			|| m_Manager.GetPlayerVoiceIdentity(m_iAdmin) != m_sAdminIdentity)
		{
			Print("Role fixture participant departed; refusing to modify another controller");
			return;
		}
		bool removed = players.ClearPlayerRole(m_iAdmin, EPlayerRole.SESSION_ADMINISTRATOR);
		GC_AdminActionLogger.Log(m_iAdmin, "test-dedicated-admin-role", "revoke", "changed=" + removed.ToString());
		AssertTrue(removed && !SCR_Global.IsAdmin(m_iAdmin), "Temporary native role revocation");
		if (removed)
			m_bRoleOwned = false;
	}
}
