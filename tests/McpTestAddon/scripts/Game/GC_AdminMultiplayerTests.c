//! Optional test-addon transport. No automatic execution or permission grants.
//! Owner-side commands call production requests; the normal server gates decide.
modded class GC_LobbyPlayerController
{
	protected string m_sGCReservationPeerToken;
	protected int m_iGCReservationPeerRequest = -1;

	protected string m_sGCTestNearbyCharacters;
	protected int m_iGCTestNearbyCharacters;
	//------------------------------------------------------------------------------------------------
	protected bool GC_TestNearbyCharacter(IEntity entity)
	{
		if (!ChimeraCharacter.Cast(entity))
			return true;
		m_iGCTestNearbyCharacters++;
		m_sGCTestNearbyCharacters += string.Format(" [controlled=%1 spectator=%2 flags=%3 position=%4]",
			entity == GetControlledEntity(), entity.FindComponent(GC_LobbySpectatorBodyComponent) != null, entity.GetFlags(), entity.GetOrigin());
		return m_iGCTestNearbyCharacters < 6;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSpectatorPresentation()
	{
		IEntity entity = GetControlledEntity();
		if (!entity)
			return "controlled=none";
		int visibleChildren;
		int children;
		IEntity child = entity.GetChildren();
		while (child)
		{
			children++;
			if ((child.GetFlags() & EntityFlags.VISIBLE) != 0)
				visibleChildren++;
			child = child.GetSibling();
		}
		bool currentCamera;
		if (m_SpectatorCamera && GetGame().GetCameraManager())
			currentCamera = GetGame().GetCameraManager().CurrentCamera() == m_SpectatorCamera;
		string result = string.Format("state=%1 requested=%2 spectatorBody=%3 bodyVisible=%4 children=%5 visibleChildren=%6 cameraExists=%7 cameraCurrent=%8",
			m_eLobbyPlayerState, m_bSpectatorPresentationRequested, entity.FindComponent(GC_LobbySpectatorBodyComponent) != null,
			(entity.GetFlags() & EntityFlags.VISIBLE) != 0, children, visibleChildren, m_SpectatorCamera != null, currentCamera);
		if (m_SpectatorCamera)
		{
			m_sGCTestNearbyCharacters = string.Empty;
			m_iGCTestNearbyCharacters = 0;
			GetWorld().QueryEntitiesBySphere(m_SpectatorCamera.GetOrigin(), 3, GC_TestNearbyCharacter);
			result += string.Format(" bodyPosition=%1 cameraPosition=%2 nearby=%3", entity.GetOrigin(), m_SpectatorCamera.GetOrigin(), m_sGCTestNearbyCharacters);
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestDispatchPeer(string token, string action, string message)
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager())
			Rpc(GC_TestRpcPeerCommand, token, action, message);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_TestRpcPeerCommand(string token, string action, string message)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || GetGame().GetPlayerController() != this)
			return;
		if (action == "stats-open")
		{
			OpenSlottingFromPause();
			Rpc(GC_TestRpcPeerReport, token, "dispatched production lobby/debrief entry");
			return;
		}
		if (action == "stats-read")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestDebriefRead());
			return;
		}
		if (action == "stats-select")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestDebriefSelect(message));
			return;
		}
		if (action == "mission-framework-snapshot")
		{
			bool flag = manager.GC_IsMissionFlagSet("Interaction complete");
			GC_Briefing briefing = manager.GetBriefing(32);
			bool hidden = briefing && !briefing.IsVisible();
			Rpc(GC_TestRpcPeerReport, token, string.Format("flag=%1 briefingHidden=%2 authority=%3", flag, hidden, Replication.IsServer()));
			return;
		}
		if (action == "presentation-report" || action == "presentation-reset" || action == "presentation-map-open" || action == "presentation-map-close" || action == "presentation-setup")
		{
			string presentation;
			if (action == "presentation-reset")
				presentation = manager.GC_TestPresentationReset();
			else if (action == "presentation-setup")
				presentation = manager.GC_TestSetupSnapshot();
			else if (action == "presentation-map-open" || action == "presentation-map-close")
				presentation = manager.GC_TestAOMapSurface("gameplay", action == "presentation-map-open");
			else
				presentation = manager.GC_TestPresentationReport();
			Rpc(GC_TestRpcPeerReport, token, presentation);
			return;
		}
		if (action == "dedicated-action")
		{
			GC_LobbyPlayerController.Cast(this).GC_TestDedicatedAction(token, message);
			return;
		}
		if (action == "ao-snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAOSnapshot());
			return;
		}
		if (action == "ao-map")
		{
			Rpc(GC_TestRpcPeerReport, token, GC_TestAOMapLocal());
			return;
		}
		if (action == "map-shape-view")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAOMapView(1.5, message.ToVector()));
			return;
		}

		if (action == "scope-move")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminScopeMove(message.ToVector()));
			return;
		}
		if (action == "scope-snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminScopeSnapshot(message.ToInt()));
			return;
		}
		if (action == "scope-picker")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminScopePicker(message.ToInt()));
			return;
		}
		if (action == "scope-ui")
		{
			array<string> uiFields = {};
			message.Split("|", uiFields, false);
			if (uiFields.Count() != 2)
				return;
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminScopeUi(uiFields[0], uiFields[1]));
			return;
		}
		if (action == "scope-request")
		{
			array<string> fields = {};
			message.Split("|", fields, false);
			if (fields.Count() != 3)
			{
				Rpc(GC_TestRpcPeerReport, token, "REJECTED expected action|faction|targetId");
				return;
			}
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminScopeRequest(fields[0], fields[1], fields[2].ToInt()));
			return;
		}

		if (action == "lottery-snapshot" || action == "lottery-start")
		{
			if (action == "lottery-start")
				RequestStartSlotLottery("", RplId.Invalid(), message.ToInt());
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestLotterySnapshot());
			return;
		}

		if (action == "phase-clock")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestPhaseClockSnapshot());
			return;
		}

		if (action.StartsWith("readiness-"))
		{
			if (action == "readiness-tile-click")
			{
				Rpc(GC_TestRpcPeerReport, token, manager.GC_TestReadinessSurface(message, "ReadinessCommander", true));
				return;
			}
			if (action == "readiness-open")
				OpenBriefingFromPause();
			else if (action == "readiness-long-name")
				manager.GC_TestReadinessLongName();
			else if (action == "readiness-focus")
			{
				MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
				if (menu && menu.GetRootWidget())
					GetGame().GetWorkspace().SetFocusedWidget(menu.GetRootWidget().FindAnyWidget("ReadinessAction_" + message));
			}
			else if (action == "readiness-join")
				RequestLobbySlot(message.ToInt());
			else if (action == "readiness-ready" || action == "readiness-unready")
			{
				GC_Slot slot = manager.GetFactionCommanderSlot(message);
				int slotId = -1;
				if (slot)
					slotId = slot.GetID();
				RequestFactionReadiness(message, slotId, manager.GetFactionReadinessRevision(), action == "readiness-ready");
			}
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestReadinessSnapshot());
			return;
		}

		if (action.StartsWith("reservation-"))
		{
			GC_TestReservationPeer(token, action, message);
			return;
		}

		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		GC_VoiceChannelAssignment voice = manager.GetPlayerVoiceAssignment(GetPlayerId());
		array<string> recipients = {};
		if (action == "pause-admin")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestPauseAction(message));
			return;
		}
		if (action == "roster")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestRosterLocks());
			return;
		}
		if (action == "message-all")
			RequestAdminMessage(GC_EAdminMessageAudience.ALL_PLAYERS, message, recipients);
		else if (action == "heal-self")
		{
			array<int> healPlayers = {GetPlayerId()};
			array<string> healIdentities = {manager.GetPlayerVoiceIdentity(GetPlayerId())};
			RequestAdminHeal(GC_EAdminHealScope.SELECTED, healPlayers, healIdentities, ReserveAdminPlayerToolRequest());
		}
		else if (action == "heal-nearby")
		{
			array<int> nearbyPlayers = {};
			array<string> nearbyIdentities = {};
			int nearbyRequest = ReserveAdminPlayerToolRequest();
			RequestAdminHeal(GC_EAdminHealScope.NEARBY, nearbyPlayers, nearbyIdentities, nearbyRequest, message.ToInt());
			Rpc(GC_TestRpcPeerReport, token, string.Format("DISPATCHED request=%1 radius=%2; inspect server audit and health", nearbyRequest, message));
			return;
		}
		else if (action == "teleport-to-player")
		{
			int destination = message.ToInt();
			GC_RequestAdminTeleport(GetPlayerId(), manager.GetPlayerVoiceIdentity(GetPlayerId()),
				destination, manager.GetPlayerVoiceIdentity(destination), ReserveAdminPlayerToolRequest());
		}
		else if (action == "vehicle-health")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestHealVehicleByRpl(message.ToInt()));
			return;
		}
		else if (action == "health-snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestAdminHealthSnapshot(GetPlayerId()));
			return;
		}
		else if (action == "voice-grant" || action == "voice-release")
		{
			if (m_bLobbyVoiceHeld || m_LobbyCapture)
			{
				Rpc(GC_TestRpcPeerReport, token, "REFUSED active user capture");
				return;
			}
			if (action == "voice-grant")
				Rpc(RpcAsk_LobbyVoice, GC_LobbyRadio.CHAT, m_iLobbyVoiceRequest);
			else
				Rpc(RpcAsk_ReleaseLobbyVoice);
		}
		else if (action == "admin-slot-self")
		{
			GC_Slot ownSlot = manager.GetSlotByPlayerId(GetPlayerId());
			if (!ownSlot)
			{
				Rpc(GC_TestRpcPeerReport, token, "UNAVAILABLE assigned slot");
				return;
			}
			RequestAdminSlot(ownSlot.GetID(), ownSlot.GetUUID(), message.ToInt());
		}
		else if (action == "message-admins")
			RequestAdminMessage(GC_EAdminMessageAudience.ADMINS, message, recipients);
		else if (action == "mute-self" && voice)
			RequestAdminVoice(GetPlayerId(), voice.GetPlayerIdentity(), false, true);
		else if (action == "priority-self" && voice)
			RequestAdminVoice(GetPlayerId(), voice.GetPlayerIdentity(), true, true);
		else if (action == "slot-request")
			RequestLobbySlot(message.ToInt());
		else if (action == "faction-rules-unlock")
			RequestAdminFactionRules(manager.GetFactionRulesRevision(), 2, false);
		else if (action == "freeze-move-inside" && freeze)
			freeze.GC_TestFreezeMove("145 1 140");
		else if (action == "freeze-move-outside" && freeze)
			freeze.GC_TestFreezeMove("210 1 130");
		else if (action == "freeze-end" && freeze)
			RequestAdminFreeze(freeze.GetRevision(), "end", 0);
		else if (action == "freeze-raise" && freeze)
			freeze.GC_TestFreezeWeapon("raise");
		else if (action == "freeze-fire" && freeze)
			freeze.GC_TestFreezeWeapon("fire");
		else if (action == "combat-equip-grenade" && freeze)
		{
			Rpc(GC_TestRpcPeerReport, token, freeze.GC_TestEquipGrenade());
			return;
		}
		else if (action == "combat-raise" && freeze)
			freeze.GC_TestFreezeCombat("raise");
		else if (action == "combat-throw" && freeze)
			freeze.GC_TestFreezeCombat("CharacterThrowGrenade");
		else if (action == "combat-melee" && freeze)
			freeze.GC_TestFreezeCombat("CharacterMelee");
		else if (action == "follow-player")
		{
			int targetId = message.ToInt();
			RequestAdminFollowPlayer(targetId, manager.GetPlayerVoiceIdentity(targetId));
		}
		else if (action == "follow-snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, GC_TestFollowSnapshot());
			return;
		}
		else if (action == "message-queue-snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, manager.GC_TestMessageQueueSnapshot());
			return;
		}
		else if ((action == "pilot-fire" || action == "pilot-snapshot") && freeze)
		{
			string pilotAction = "snapshot";
			if (action == "pilot-fire")
				pilotAction = "fire";
			Rpc(GC_TestRpcPeerReport, token, freeze.GC_TestPilotWeapon(pilotAction));
			return;
		}
		else if ((action == "turret-fire" || action == "turret-snapshot") && freeze)
		{
			string turretAction = "snapshot";
			if (action == "turret-fire")
				turretAction = "fire";
			Rpc(GC_TestRpcPeerReport, token, freeze.GC_TestTurret(turretAction));
			return;
		}
		else if (action != "snapshot")
		{
			Rpc(GC_TestRpcPeerReport, token, "UNAVAILABLE action or voice assignment");
			return;
		}

		// This is an observation at dispatch time. Read again after a mutation;
		// neither this response nor a successful RPC proves the action succeeded.
		string report = string.Format("player=%1 admin=%2 messages=%3", GetPlayerId(), SCR_Global.IsAdmin(GetPlayerId()), m_aAdminMessages.Count());
		report += " presentation=" + GC_TestSpectatorPresentation();
		report += string.Format(" rulesRevision=%1 factionLock=%2 balance=%3", manager.GetFactionRulesRevision(), manager.GetFactionLockOverride(), manager.IsFactionBalanceEnforced());
		Faction faction = manager.GetPlayerFaction(GetPlayerId());
		if (faction)
			report += " faction=" + faction.GetFactionKey();
		if (freeze)
		{
			report += " freezeState=" + freeze.GC_TestFreezeSnapshot() + " hud=" + freeze.GC_TestFreezeHud() + " weapon=" + freeze.GC_TestFreezeWeapon("snapshot");
			report += " combat=" + freeze.GC_TestFreezeCombat("snapshot");
		}
		GC_Slot slot = manager.GetSlotByPlayerId(GetPlayerId());
		if (slot)
		{
			report += string.Format(" slot=%1 entity=%2", slot.GetID(), slot.GetEntityRplId());
			if (GetControlledEntity())
				report += string.Format(" controlledPosition=%1", GetControlledEntity().GetOrigin());
		}
		else
			report += " slot=none";
		if (voice)
		{
			report += string.Format(" priority=%1 muted=%2 transmitting=%3 eligible=%4", voice.IsPrioritySpeaker(), voice.IsAdminMuted(), voice.IsPriorityTransmitting(), manager.CanTransmitLobbyVoice(GetPlayerId()));
		}
		if (!m_aAdminMessages.IsEmpty())
		{
			GC_AdminMessage latest = m_aAdminMessages[m_aAdminMessages.Count() - 1];
			report += string.Format(" latestId=%1 audience=%2 text=%3", latest.m_iMessageId, latest.m_eAudience, latest.m_sText);
		}
		Rpc(GC_TestRpcPeerReport, token, report);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_TestRpcPeerReport(string token, string report)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager && Replication.IsServer())
			manager.GC_TestRecordPeerReport(GetPlayerId(), token, report);
	}
	//------------------------------------------------------------------------------------------------
	protected void GC_TestReservationPeer(string token, string action, string message)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_Slot slot = manager.GetSlot(message.ToInt());
		if (action == "reservation-snapshot")
		{
			if (!slot)
			{
				Rpc(GC_TestRpcPeerReport, token, "UNAVAILABLE slot");
				return;
			}
			SCR_AIGroup group = manager.GetPlayerGroup(GetPlayerId());
			Rpc(GC_TestRpcPeerReport, token, string.Format("slot=%1 uid=%2 player=%3 initial=%4 name=%5 nativeGroupMatches=%6",
				slot.GetID(), slot.GetUUID(), slot.GetPlayerId(), slot.IsAwaitingInitialJoin(),
				slot.GetCachedPlayerName(), group && group == slot.GetGroup()));
			return;
		}
		if (action != "reservation-attempt" || !slot || m_iGCReservationPeerRequest >= 0)
		{
			Rpc(GC_TestRpcPeerReport, token, "REJECTED fixture action");
			return;
		}
		m_sGCReservationPeerToken = token;
		m_iGCReservationPeerRequest = ReserveAdminPlayerToolRequest();
		m_OnSlotReservationResult.Insert(GC_TestReservationPeerResult);
		RequestSlotReservation("reserve", slot.GetID(), slot.GetUUID(), manager.GetPlayerVoiceIdentity(GetPlayerId()), "", "", m_iGCReservationPeerRequest);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestReservationPeerResult(int requestId, string outcome, string output, string currentClaim)
	{
		if (requestId != m_iGCReservationPeerRequest)
			return;
		m_iGCReservationPeerRequest = -1;
		m_OnSlotReservationResult.Remove(GC_TestReservationPeerResult);
		Rpc(GC_TestRpcPeerReport, m_sGCReservationPeerToken, outcome);
	}
}

modded class GC_LobbyManager
{
	protected ref map<string, int> m_GC_PendingPeerReports = new map<string, int>();
	protected ref map<string, string> m_GC_PeerReports = new map<string, string>();

	//------------------------------------------------------------------------------------------------
	//! Compact observation fits the bounded peer report: id, group, lock, player.
	string GC_TestRosterLocks()
	{
		string result = string.Format("slots=%1 rows=id,group,locked,player", GetSlots().Count());
		foreach (GC_Slot slot : GetSlots())
		{
			result += string.Format(" | %1,%2,%3,%4", slot.GetID(), slot.GetGroupRplId(),
				slot.IsAdminLocked(), slot.GetPlayerId());
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPeerCommand(int playerId, string token, string action, string message)
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE authority";
		if (token.IsEmpty() || token.Length() > 64 || token.Contains("\n") || token.Contains("\r") || message.Length() > 512)
			return "REJECTED token or message";
		if (m_GC_PendingPeerReports.Contains(token) || m_GC_PeerReports.Contains(token))
			return "REJECTED token already used";
		if (m_GC_PendingPeerReports.Count() + m_GC_PeerReports.Count() >= 128)
			return "UNAVAILABLE report limit; start a fresh test session";
		array<string> allowedActions = {
			"stats-open",
			"stats-read",
			"stats-select",
			"mission-framework-snapshot",
			"ao-snapshot",
			"ao-map",
			"map-shape-view",
			"presentation-report",
			"presentation-reset",
			"presentation-map-open",
			"presentation-map-close",
			"presentation-setup",
			"dedicated-action",
			"scope-snapshot",
			"scope-move",
			"scope-request",
			"scope-picker",
			"scope-ui",
			"lottery-snapshot",
			"lottery-start",
			"phase-clock",
			"readiness-open",
			"readiness-tile-click",
			"readiness-long-name",
			"readiness-focus",
			"readiness-join",
			"readiness-ready",
			"readiness-unready",
			"readiness-snapshot",
			"snapshot",
			"reservation-snapshot",
			"reservation-attempt",
			"pause-admin",
			"heal-self",
			"heal-nearby",
			"teleport-to-player",
			"health-snapshot",
			"vehicle-health",
			"roster",
			"voice-grant",
			"voice-release",
			"message-all",
			"message-admins",
			"mute-self",
			"priority-self",
			"slot-request",
			"faction-rules-unlock",
			"freeze-end",
			"freeze-raise",
			"freeze-fire",
			"freeze-move-inside",
			"freeze-move-outside",
			"admin-slot-self",
			"combat-equip-grenade",
			"combat-raise",
			"combat-throw",
			"combat-melee",
			"pilot-fire",
			"pilot-snapshot",
			"turret-fire",
			"turret-snapshot",
			"follow-player",
			"follow-snapshot",
			"message-queue-snapshot"
		};
		if (allowedActions.Find(action) == -1)
			return "REJECTED unsupported test action";
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return "UNAVAILABLE connected controller";
		m_GC_PendingPeerReports.Set(token, playerId);
		controller.GC_TestDispatchPeer(token, action, message);
		return "DISPATCHED; read GC_TestPeerReport with the same token";
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestRecordPeerReport(int playerId, string token, string report)
	{
		int expectedPlayer;
		if (!Replication.IsServer() || !m_GC_PendingPeerReports.Find(token, expectedPlayer) || expectedPlayer != playerId || report.Length() > 2048)
			return;
		m_GC_PendingPeerReports.Remove(token);
		m_GC_PeerReports.Set(token, report);
		PrintFormat("GC | MCP Peer Observation | token=%1 player=%2 report=%3", token, playerId, report);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPeerReport(string token)
	{
		string report;
		if (m_GC_PeerReports.Find(token, report))
			return "CLIENT-REPORTED " + report;
		if (m_GC_PendingPeerReports.Contains(token))
			return "PENDING; inspect connection and logs before issuing another command";
		return "UNKNOWN token";
	}
}
