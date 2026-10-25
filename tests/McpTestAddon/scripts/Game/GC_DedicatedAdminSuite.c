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

[Test(suite: GC_DedicatedAdminSuite, timeoutMs: 420000)]
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
	protected int m_iFailures;
	protected bool m_bRoleOwned;
	protected vector m_vAdminPosition;
	protected int m_iTargetAmmo;

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.Setup)]
	void Prepare()
	{
		m_iStageStarted = System.GetTickCount();
		string requestedSuite;
		System.GetCLIParam("autotest", requestedSuite);
		AssertTrue(requestedSuite == "GC_DedicatedAdminSuite" && RplSession.Mode() == RplMode.Dedicated,
			"Requires explicit native suite selection on a dedicated server");
		Print("Dedicated regression: real owner clients; identities are not fabricated.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(bool condition, string description)
	{
		if (condition)
			Print("PASS " + description);
		else
		{
			m_iFailures++;
			Print("FAIL " + description);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(int playerId, string token, string action, string message)
	{
		if (GetFailure())
			return;
		string result = m_Manager.GC_TestPeerCommand(playerId, token, action, message);
		AssertTrue(result.StartsWith("DISPATCHED"), "Peer dispatch: " + result);
	}

	//------------------------------------------------------------------------------------------------
	protected void Action(string token, string action)
	{
		// These cases test refusal only. Never turn them into real kicks/bans if
		// this fixture is reused with authenticated accounts or identities arrive late.
		if (action == "heal-selected" || action == "teleport" || action == "teleport-faction"
			|| action == "message-selected" || action == "player-kick"
			|| action == "player-ban" || action == "player-unban")
		{
			AssertTrue(SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iAdmin).IsEmpty()
				&& SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iTarget).IsEmpty(),
				"Identity-refusal segment requires empty identities; no action dispatched otherwise");
		}
		Send(m_iAdmin, token, "dedicated-action", action + "|" + m_iTarget.ToString());
	}

	//------------------------------------------------------------------------------------------------
	protected bool Report(string token, string expected, bool requireAbsent = false)
	{
		string report = m_Manager.GC_TestPeerReport(token);
		if (report.StartsWith("PENDING"))
			return false;
		Print(token + ": " + report);
		Check(report.StartsWith("CLIENT-REPORTED") && report.Contains(expected), token + " terminal result");
		if (requireAbsent)
			Check(report.Contains("server=0") && report.Contains("character=0 slotEntity=0 replicationItem=0"), token + " remote entity absent on client");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected float Health(int playerId)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return -1;
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return -1;
		return damage.GetHealthScaled();
	}

	//------------------------------------------------------------------------------------------------
	protected int Ammo()
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(m_iTarget));
		if (!character || !character.GetCharacterController())
			return -1;
		BaseWeaponManagerComponent weapons = character.GetCharacterController().GetWeaponManagerComponent();
		if (!weapons || !weapons.GetCurrentWeapon() || !weapons.GetCurrentWeapon().GetCurrentMagazine())
			return -1;
		return weapons.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount();
	}

	//------------------------------------------------------------------------------------------------
	protected void DamageBoth()
	{
		Print(m_Manager.GC_TestAdminScopeDamage(m_iAdmin));
		Print(m_Manager.GC_TestAdminScopeDamage(m_iTarget));
		AssertTrue(Health(m_iAdmin) == 0.5 && Health(m_iTarget) == 0.5, "Damage fixture prerequisite");
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
			AssertTrue(connected.Count() == 2 && m_Manager.GetLobbyPhase() == SCR_EGameModeState.PREGAME,
				"Requires fresh disposable two-client Lobby2 session");
			if (GetFailure())
				return true;
			connected.Sort();
			m_iAdmin = connected[0];
			m_iTarget = connected[1];
			m_AdminController = GC_LobbyPlayerController.Cast(players.GetPlayerController(m_iAdmin));
			m_TargetController = players.GetPlayerController(m_iTarget);
			if (!m_AdminController || !m_TargetController || !players.GetPlayerControlledEntity(m_iAdmin) || !players.GetPlayerControlledEntity(m_iTarget))
				return false;
			AssertTrue(!SCR_Global.IsAdmin(m_iAdmin), "Refuse to change a pre-existing admin role");
			if (GetFailure())
				return true;
			m_bRoleOwned = players.GivePlayerRole(m_iAdmin, EPlayerRole.SESSION_ADMINISTRATOR);
			GC_AdminActionLogger.Log(m_iAdmin, "test-dedicated-admin-role", "grant", "changed=" + m_bRoleOwned.ToString());
			AssertTrue(m_bRoleOwned && SCR_Global.IsAdmin(m_iAdmin), "Native session role fixture");
			Send(m_iAdmin, "ready", "snapshot", "");
		}
		else if (GC_LobbyManager.GetManager() != m_Manager || !players.IsPlayerConnected(m_iAdmin) || !players.IsPlayerConnected(m_iTarget)
			|| players.GetPlayerController(m_iAdmin) != m_AdminController || players.GetPlayerController(m_iTarget) != m_TargetController)
		{
			SetFailure("Participant connection or world changed");
			return true;
		}

		switch (m_iStage)
		{
			case 1:
				if (!Report("ready", "admin=1"))
					return false;
				Action("slotting", "phase-slotting");
				break;
			case 2:
				if (!Report("slotting", "dispatched-phase"))
					return false;
				AssertTrue(m_Manager.GetLobbyPhase() == SCR_EGameModeState.SLOTTING, "Client phase advance to slotting");
				Action("unlock", "group-unlock");
				break;
			case 3:
				if (!Report("unlock", "outcome=applied"))
					return false;
				foreach (GC_Slot unlocked : m_Manager.GetSlots())
					Check(!unlocked.IsAdminLocked(), "Slot " + unlocked.GetID().ToString() + " unlocked");
				Action("rules", "rules-unlock");
				break;
			case 4:
				if (!Report("rules", "outcome=applied"))
					return false;
				Check(m_Manager.GetFactionLockOverride() == 2 && !m_Manager.IsFactionBalanceEnforced(), "Faction rules changed on authority");
				int adminSlot = -1;
				int targetSlot = -1;
				foreach (GC_Slot slot : m_Manager.GetSlots())
				{
					if (!slot.GetGroup() || !slot.GetGroup().GetFaction())
						continue;
					string key = slot.GetGroup().GetFaction().GetFactionKey();
					if (key == "US" && adminSlot < 0)
						adminSlot = slot.GetID();
					if (key == "USSR" && targetSlot < 0)
						targetSlot = slot.GetID();
				}
				AssertTrue(adminSlot >= 0 && targetSlot >= 0, "Everon US and USSR fixture slots");
				Send(m_iAdmin, "join-admin", "slot-request", adminSlot.ToString());
				Send(m_iTarget, "join-target", "slot-request", targetSlot.ToString());
				break;
			case 5:
				if (!m_Manager.GetSlotByPlayerId(m_iAdmin) || !m_Manager.GetSlotByPlayerId(m_iTarget))
					return false;
				Faction adminFaction = SCR_FactionManager.SGetPlayerFaction(m_iAdmin);
				Faction targetFaction = SCR_FactionManager.SGetPlayerFaction(m_iTarget);
				AssertTrue(adminFaction && targetFaction, "Native faction prerequisite");
				if (GetFailure())
					return true;
				Check(adminFaction.GetFactionKey() == "US" && targetFaction.GetFactionKey() == "USSR",
					"Normal client slot requests establish native factions without backend identity");
				Action("briefing", "phase-briefing");
				break;
			case 6:
				if (!Report("briefing", "dispatched-phase"))
					return false;
				AssertTrue(m_Manager.GetLobbyPhase() == SCR_EGameModeState.BRIEFING, "Client phase advance to briefing");
				Action("game", "phase-game");
				break;
			case 7:
				if (!Report("game", "dispatched-phase"))
					return false;
				if (Health(m_iAdmin) < 0 || Health(m_iTarget) < 0)
					return false;
				AssertTrue(m_Manager.GetLobbyPhase() == SCR_EGameModeState.GAME, "Client phase advance to game");
				m_vAdminPosition = players.GetPlayerControlledEntity(m_iAdmin).GetOrigin();
				Send(m_iTarget, "move", "scope-move", "4500 0 8000");
				break;
			case 8:
				if (!Report("move", "fixtureMove=1"))
					return false;
				Send(m_iAdmin, "absence", "scope-snapshot", m_iTarget.ToString());
				break;
			case 9:
				if (!Report("absence", "listed=1", true))
					return false;
				AssertTrue(m_iFailures == 0, "Replication absence and setup prerequisites");
				Check(vector.Distance(m_vAdminPosition, players.GetPlayerControlledEntity(m_iTarget).GetOrigin()) > 5000, "Authority verifies separation over 5km");
				DamageBoth();
				Action("everyone", "ui-everyone");
				break;
			case 10:
				if (!Report("everyone", "outcome=Healed 2 player(s)", true))
					return false;
				Check(Health(m_iAdmin) == 1 && Health(m_iTarget) == 1, "Everyone UI heals both real players");
				Send(m_iTarget, "everyone-owner", "scope-snapshot", m_iTarget.ToString());
				break;
			case 11:
				if (!Report("everyone-owner", "health=1"))
					return false;
				DamageBoth();
				Action("faction", "ui-faction");
				break;
			case 12:
				if (!Report("faction", "outcome=Healed 1 player(s)", true))
					return false;
				Check(Health(m_iAdmin) == 0.5 && Health(m_iTarget) == 1, "Faction UI heals distant USSR and excludes US");
				Send(m_iTarget, "faction-owner", "scope-snapshot", m_iTarget.ToString());
				break;
			case 13:
				if (!Report("faction-owner", "health=1"))
					return false;
				DamageBoth();
				Action("nearby", "heal-nearby");
				break;
			case 14:
				if (!Report("nearby", "outcome=Within 50m: Healed 1 player(s)", true))
					return false;
				Check(Health(m_iAdmin) == 1 && Health(m_iTarget) == 0.5, "Nearby heals local admin and excludes distant player");
				Action("message-all", "message-everyone");
				break;
			case 15:
				if (!Report("message-all", "recipients=2 skipped=0", true))
					return false;
				Send(m_iAdmin, "message-all-admin", "snapshot", "");
				Send(m_iTarget, "message-all-target", "snapshot", "");
				break;
			case 16:
				if (!Report("message-all-admin", "messages=1") || !Report("message-all-target", "Dedicated message-everyone"))
					return false;
				Action("message-faction", "message-faction");
				break;
			case 17:
				if (!Report("message-faction", "recipients=1 skipped=0", true))
					return false;
				Send(m_iAdmin, "message-faction-admin", "snapshot", "");
				Send(m_iTarget, "message-faction-target", "snapshot", "");
				break;
			case 18:
				if (!Report("message-faction-admin", "messages=1") || !Report("message-faction-target", "Dedicated message-faction"))
					return false;
				Action("moderation", "moderation");
				break;
			case 19:
				if (!Report("moderation", "rows=2", true))
					return false;
				Send(m_iAdmin, "picker", "scope-picker", m_iTarget.ToString());
				break;
			case 20:
				if (!Report("picker", "listed=1 selectable=0 character=0"))
					return false;
				Print("LIMIT: empty account identity prevents selected-player operations; verifying rejection, not successful targeting.");
				Action("selected", "heal-selected");
				break;
			case 21:
				if (!Report("selected", "A selected player disconnected.", true))
					return false;
				Check(Health(m_iTarget) == 0.5, "Identity-less selected heal does not mutate target");
				Action("teleport", "teleport");
				break;
			case 22:
				if (!Report("teleport", "A selected player changed or disconnected.", true))
					return false;
				Check(vector.Distance(m_vAdminPosition, players.GetPlayerControlledEntity(m_iAdmin).GetOrigin()) < 3, "Rejected teleport does not move admin");
				Action("teleport-faction", "teleport-faction");
				break;
			case 23:
				if (!Report("teleport-faction", "The destination player must still have a living spawned character.", true))
					return false;
				Check(vector.Distance(m_vAdminPosition, players.GetPlayerControlledEntity(m_iAdmin).GetOrigin()) < 3, "Rejected faction teleport does not move admin");
				Action("selected-message", "message-selected");
				break;
			case 24:
				if (!Report("selected-message", "rejected-stale-recipient", true))
					return false;
				Action("kick", "player-kick");
				break;
			case 25:
				if (!Report("kick", "rejected-account-identity", true))
					return false;
				Check(players.IsPlayerConnected(m_iTarget), "Rejected kick keeps target connected");
				Action("ban", "player-ban");
				break;
			case 26:
				if (!Report("ban", "rejected-account-identity", true))
					return false;
				Action("unban", "player-unban");
				break;
			case 27:
				if (!Report("unban", "rejected-account-identity", true))
					return false;
				Send(m_iAdmin, "moderation-open", "scope-ui", "click|ToolModeration");
				break;
			case 28:
				Send(m_iAdmin, "moderation-ui", "scope-ui", "moderation|" + m_iTarget.ToString());
				break;
			case 29:
				if (!Report("moderation-ui", "rows=2 selected=1 identityReady=0"))
					return false;
				Print(m_Manager.GC_TestHealKill(m_iTarget));
				break;
			case 30:
				if (!m_Manager.GetSlotByPlayerId(m_iTarget).IsKIA())
					return false;
				Action("respawn", "heal-everyone");
				break;
			case 31:
				if (!Report("respawn", "Respawned 1."))
					return false;
				Check(Health(m_iTarget) == 1 && !m_Manager.GetSlotByPlayerId(m_iTarget).IsKIA(), "Everyone recovers a distant KIA slot");
				Send(m_iTarget, "respawn-owner", "scope-snapshot", m_iTarget.ToString());
				break;
			case 32:
				if (!Report("respawn-owner", "health=1"))
					return false;
				vector center = (players.GetPlayerControlledEntity(m_iAdmin).GetOrigin() + players.GetPlayerControlledEntity(m_iTarget).GetOrigin()) * 0.5;
				center[1] = GetGame().GetWorld().GetSurfaceY(center[0], center[2]) + 0.2;
				Print(GC_LobbyFreezeComponent.GetInstance().GC_TestStagingArea(center, 5000));
				AssertTrue(GC_LobbyFreezeComponent.GetInstance().HasStagingAreas(), "Temporary freeze area prerequisite");
				Action("freeze-start", "freeze-start");
				break;
			case 33:
				if (!Report("freeze-start", "outcome=applied", true))
					return false;
				Check(GC_LobbyFreezeComponent.GetInstance().IsFreezeActive(), "Freeze active on authority");
				Send(m_iTarget, "freeze-owner", "snapshot", "");
				break;
			case 34:
				if (!Report("freeze-owner", "active=1"))
					return false;
				m_iTargetAmmo = Ammo();
				AssertTrue(m_iTargetAmmo > 0, "Loaded remote weapon prerequisite");
				Send(m_iTarget, "freeze-raise", "freeze-raise", "");
				break;
			case 35:
				Send(m_iTarget, "freeze-fire", "freeze-fire", "");
				break;
			case 36:
				if (!Report("freeze-fire", "freeze=1"))
					return false;
				Check(Ammo() == m_iTargetAmmo,
					"Freeze suppresses remote client's fire input without ammo consumption");
				Action("freeze-extend", "freeze-extend");
				break;
			case 37:
				if (!Report("freeze-extend", "outcome=applied", true))
					return false;
				Check(GC_LobbyFreezeComponent.GetInstance().GetRemainingSeconds() > 60, "Freeze extension applied");
				Action("freeze-end", "freeze-end");
				break;
			case 38:
				if (!Report("freeze-end", "outcome=applied", true))
					return false;
				Check(!GC_LobbyFreezeComponent.GetInstance().IsFreezeActive(), "Freeze ended on authority");
				Send(m_iTarget, "freeze-ended-owner", "snapshot", "");
				Send(m_iTarget, "unfrozen-raise", "freeze-raise", "");
				Action("reservation-export", "reservation-export");
				break;
			case 39:
				if (!Report("freeze-ended-owner", "active=0") || !Report("reservation-export", "parsed=1 version=1 slots=26", true))
					return false;
				Action("map-save", "map-save");
				Send(m_iTarget, "unfrozen-fire", "freeze-fire", "");
				break;
			case 40:
				if (!Report("map-save", "Saved plan to $profile:Lobby2MapPlans/dedicated-regression.json", true))
					return false;
				if (!Report("unfrozen-fire", "freeze=0"))
					return false;
				Check(Ammo() >= 0 && Ammo() < m_iTargetAmmo,
					"Same remote fire input consumes ammo after freeze ends");
				Action("map-clear", "map-clear");
				break;
			case 41:
				if (!Report("map-clear", "Load and Clear are available before gameplay only.", true))
					return false;
				Send(m_iTarget, "admin-chat", "dedicated-action", "chat-send|" + m_iAdmin.ToString());
				break;
			case 42:
				Action("admin-chat-read", "chat-read");
				break;
			case 43:
				if (!Report("admin-chat-read", "Dedicated admin contact"))
					return false;
				Print("LIMIT ban-list not replayed: native crash on this unconfigured dedicated session; see AdminReplicationScope.md.");
				break;
			case 44:
				Send(m_iAdmin, "reservations-tab", "scope-ui", "click|ToolReservations");
				break;
			case 45:
				if (!Report("reservations-tab", "ReservationPanel=1 ReservationsSelectedBar=1"))
					return false;
				Send(m_iAdmin, "markers-tab", "scope-ui", "click|ToolMapPlans");
				break;
			case 46:
				if (!Report("markers-tab", "MapPlanPanel=1 MapPlansSelectedBar=1"))
					return false;
				Send(m_iAdmin, "heal-tab", "scope-ui", "click|ToolHeal");
				break;
			case 47:
				if (!Report("heal-tab", "ToolTitle=Heal"))
					return false;
				Send(m_iAdmin, "final-absence", "scope-snapshot", m_iTarget.ToString());
				break;
			case 48:
				if (!Report("final-absence", "listed=1", true))
					return false;
				break;
			case 49:
				if (!Report("respawn-owner", "health=1"))
					return false;
				DamageBoth();
				RevokeRole();
				Action("denied-heal", "heal-everyone");
				break;
			case 50:
				if (!Report("denied-heal", "outcome=Admin login required."))
					return false;
				Check(Health(m_iAdmin) == 0.5 && Health(m_iTarget) == 0.5, "Logged-out heal rejected without mutation");
				Action("denied-message", "message-everyone");
				break;
			case 51:
				if (!Report("denied-message", "rejected-not-logged-in"))
					return false;
				Send(m_iTarget, "denied-message-owner", "snapshot", "");
				Action("denied-lock", "group-lock");
				break;
			case 52:
				if (!Report("denied-message-owner", "messages=2") || !Report("denied-lock", "rejected-not-logged-in"))
					return false;
				foreach (GC_Slot unchanged : m_Manager.GetSlots())
					Check(!unchanged.IsAdminLocked(), "Logged-out group lock leaves slot " + unchanged.GetID().ToString() + " unchanged");
				Print("Dedicated regression finished; failures=" + m_iFailures.ToString() + "; identity-dependent successes remain untested.");
				AssertTrue(m_iFailures == 0, "Dedicated regression action checks");
				return true;
		}
		m_iStage++;
		m_iStageStarted = now;
		m_iNextCheck = now + 2000;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[TestStep(TestStage.TearDown)]
	void RevokeRole()
	{
		if (!m_bRoleOwned)
			return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (players.GetPlayerController(m_iAdmin) != m_AdminController)
		{
			Print("Owned connection departed; no other connection's role is changed.");
			return;
		}
		bool removed = players.ClearPlayerRole(m_iAdmin, EPlayerRole.SESSION_ADMINISTRATOR);
		GC_AdminActionLogger.Log(m_iAdmin, "test-dedicated-admin-role", "revoke", "changed=" + removed.ToString());
		AssertTrue(removed && !SCR_Global.IsAdmin(m_iAdmin), "Temporary role removed");
		if (removed)
			m_bRoleOwned = false;
	}
}
