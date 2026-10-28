//! Explicit test fixtures configure and invoke the production TilW instructions.
class GC_TilWTestFlagInstruction : TILW_SetFlagInstruction
{
	//------------------------------------------------------------------------------------------------
	void Configure(string flagName)
	{
		m_flagName = flagName;
		m_executionDelay = 0;
	}
}

class GC_TilWTestTicketInstruction : TILW_EditRespawnTicketsInstruction
{
	//------------------------------------------------------------------------------------------------
	void Configure(string factionKey, int operation, int amount, bool respawn)
	{
		m_factionKey = factionKey;
		m_operation = operation;
		m_amount = amount;
		m_allowSpecRespawn = respawn;
	}
}

modded class GC_LobbyManager
{
	protected vector m_vGC_TilWStartPosition;
	protected vector m_vGC_TilWDeathPosition;
	protected int m_iGC_TilWSlotId = -1;

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWDefine()
	{
		#ifdef LOBBY2
			return "LOBBY2=1";
		#else
			return "LOBBY2=0";
		#endif
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWSetup()
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() != SCR_EGameModeState.PREGAME)
			return "UNAVAILABLE: authority PREGAME required";
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		if (!framework)
			return "FAIL: missing authored framework";
		if (!framework.m_missionEvents.IsEmpty())
			return "UNAVAILABLE: fixture requires empty event list";

		GC_TilWTestFlagInstruction instruction = new GC_TilWTestFlagInstruction();
		instruction.Configure("GC_TEST_TILW_GAME");
		TILW_MissionEvent missionEvent = new TILW_MissionEvent();
		missionEvent.SetName("Lobby2 integration GAME gate");
		TILW_ConjunctionTerm condition = new TILW_ConjunctionTerm();
		condition.m_operands = {};
		missionEvent.SetCondition(condition);
		missionEvent.SetInstructions({instruction});
		framework.SetMissionEvents({missionEvent});
		framework.AdjustMissionFlag("GC_TEST_TILW_CLEAR", true);
		framework.AdjustMissionFlag("GC_TEST_TILW_CLEAR", false);
		framework.RecheckConditions();
		return GC_TestTilWSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWSnapshot()
	{
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		if (!framework)
			return "UNAVAILABLE: framework";
		PlayerController controller = GetGame().GetPlayerController();
		bool spectator;
		if (controller)
			spectator = TILW_Lobby.IsSpectator(controller.GetControlledEntity());
		return string.Format("phase=%1 mode=%2 invoker=%3 gameFlag=%4 clearFlag=%5 spectator=%6 US=%7 USSR=%8 endFlag=%9",
			GetPhaseName(GetLobbyPhase()), TILW_Lobby.GetGameMode() == GetGame().GetGameMode(),
			TILW_Lobby.GetOnGameStateChange() != null, framework.IsMissionFlag("GC_TEST_TILW_GAME"),
			framework.IsMissionFlag("GC_TEST_TILW_CLEAR"), spectator, GC_GetFactionRespawnTickets("US"),
			GC_GetFactionRespawnTickets("USSR"), framework.IsMissionFlag("GC_TEST_TILW_AFTER_END"));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWTickets(string factionKey, int operation, int amount, bool respawn)
	{
		GC_TilWTestTicketInstruction instruction = new GC_TilWTestTicketInstruction();
		instruction.Configure(factionKey, operation, amount, respawn);
		instruction.Execute();
		return GC_TestTilWSnapshot();
	}

	//------------------------------------------------------------------------------------------------
	//! Move a live claimed slot away from its start, then arrange native death.
	string GC_TestTilWDeath(int playerId)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() != SCR_EGameModeState.GAME)
			return "UNAVAILABLE: authority GAME required";
		GC_Slot slot = GetSlotByPlayerId(playerId);
		if (!slot || slot.IsKIA() || !slot.GetEntity())
			return "UNAVAILABLE: live claimed slot";
		IEntity character = slot.GetEntity();
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "UNAVAILABLE: damage manager";
		m_iGC_TilWSlotId = slot.GetID();
		m_vGC_TilWStartPosition = character.GetOrigin();
		m_vGC_TilWDeathPosition = m_vGC_TilWStartPosition + "20 0 0";
		character.SetOrigin(m_vGC_TilWDeathPosition);
		damage.SetHealthScaled(0);
		return GC_TestTilWRespawnResult();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWRespawnResult()
	{
		GC_Slot slot = GetSlot(m_iGC_TilWSlotId);
		if (!slot || !slot.GetEntity())
			return "UNAVAILABLE: remembered slot/body";
		IEntity body = slot.GetEntity();
		return string.Format("slot=%1 kia=%2 fromStart=%3 fromDeath=%4 controlled=%5 missionAI=%6",
			slot.GetID(), slot.IsKIA(), vector.Distance(body.GetOrigin(), m_vGC_TilWStartPosition),
			vector.Distance(body.GetOrigin(), m_vGC_TilWDeathPosition),
			GetGame().GetPlayerManager().GetPlayerControlledEntity(slot.GetPlayerId()) == body,
			TILW_Lobby.IsMissionAI(body));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestTilWEnd()
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE: authority";
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		GC_TilWTestFlagInstruction delayed = new GC_TilWTestFlagInstruction();
		delayed.Configure("GC_TEST_TILW_AFTER_END");
		delayed.SetExecutionDelay(2);
		TILW_ConjunctionTerm condition = new TILW_ConjunctionTerm();
		condition.m_operands = {};
		TILW_MissionEvent missionEvent = new TILW_MissionEvent();
		missionEvent.SetName("Delayed end probe");
		missionEvent.SetCondition(condition);
		missionEvent.SetInstructions({delayed});
		framework.m_missionEvents.Insert(missionEvent);
		missionEvent.EvalExpression();
		TILW_EndGameInstruction instruction = new TILW_EndGameInstruction();
		instruction.SetKey("US");
		instruction.SetGameOverType(EGameOverTypes.EDITOR_FACTION_VICTORY);
		instruction.Execute();
		return GC_TestTilWSnapshot();
	}
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestTilWInactive()
	{
		IEntity character = GetGame().GetWorld().FindEntityByName("IsolationCharacter");
		if (!character || GC_LobbyGameMode.GetGameMode())
			return "UNAVAILABLE: unrelated isolation world required";
		TILW_Lobby.SetFactionTickets("US", 10);
		TILW_Lobby.RespawnDeadPlayers("US");
		return string.Format("mode=%1 invoker=%2 tickets=%3 spectator=%4 unplayed=%5 missionAI=%6 framework=%7",
			TILW_Lobby.GetGameMode() != null, TILW_Lobby.GetOnGameStateChange() != null,
			TILW_Lobby.GetFactionTickets("US"), TILW_Lobby.IsSpectator(character),
			TILW_Lobby.IsUnplayedSlotCharacter(character), TILW_Lobby.IsMissionAI(character),
			TILW_MissionFrameworkEntity.Exists());
	}
}
