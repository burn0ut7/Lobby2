//! Opt-in tests exercise production serialization and eligibility policy without changing gameplay.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStatsContract()
	{
		GC_MissionStatsBatch batch = new GC_MissionStatsBatch();
		batch.m_sMissionId = PersistenceIdUtils.Generate();
		batch.m_aPopulation.Insert(new GC_StatsPopulation());
		GC_StatsPlayer player = new GC_StatsPlayer();
		player.m_sIdentity = "fixture-player";
		player.m_sName = "Quote \" slash \\ Unicode é";
		player.m_iShots = 7;
		GC_StatsRole role = new GC_StatsRole();
		role.m_sName = "Medic";
		role.m_iSelections = 1;
		player.m_aRoles.Insert(role);
		batch.m_aPlayers.Insert(player);
		GC_StatsDeath death = new GC_StatsDeath();
		death.m_iSequence = 1;
		death.m_sVictim = player.m_sIdentity;
		batch.m_aDeaths.Insert(death);
		string first = batch.Encode();
		if (first != batch.Encode())
			return "FAIL: serialization is not repeatable";
		player.m_iShots = 8;
		if (first == batch.Encode())
			return "FAIL: serialization retained stale counters";
		JsonLoadContext read = new JsonLoadContext();
		int version;
		if (!read.LoadFromString(first) || !read.ReadValue("schemaVersion", version) || version != 3)
			return "FAIL: invalid batch JSON";
		return first;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPlaytimeIntervals()
	{
		GC_StatsPlayer player = new GC_StatsPlayer();
		GC_StatsAssignment first = new GC_StatsAssignment();
		first.m_Start = new GC_StatsCounters();
		player.m_aAssignments.Insert(first);
		player.m_ActiveAssignment = first;
		player.SyncAssignment(10);
		player.SyncAssignment(10);
		player.SyncAssignment(5);
		player.m_ActiveAssignment = null;
		player.SyncAssignment(20);
		GC_StatsAssignment second = new GC_StatsAssignment();
		second.m_fFrom = 20;
		second.m_fUntil = 20;
		second.m_Start = new GC_StatsCounters();
		second.m_Start.Add(player);
		player.m_aAssignments.Insert(second);
		player.m_ActiveAssignment = second;
		player.SyncAssignment(25);
		if (player.m_fPlaySeconds != 15 || first.m_fPlaySeconds != 10 || second.m_fPlaySeconds != 5)
			return "FAIL: repeated sync or disconnected gap changed raw time";
		return "PASS: 15 seconds connected, 10-second gap excluded; repeated and older sync idempotent";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsSnapshot()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!stats)
			return "unavailable: no stats component";
		return stats.GetPendingBatchPreview();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDebriefRead()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		Widget root;
		if (menu)
			root = menu.GetRootWidget();
		if (!root || !root.FindAnyWidget("DebriefStatus"))
			return "unavailable: debrief not open";
		string result = TextWidget.Cast(root.FindAnyWidget("DebriefPlayer")).GetText();
		result += "\nstatus=" + TextWidget.Cast(root.FindAnyWidget("DebriefStatus")).GetText();
		for (int i = 0; i < 15; i++)
			result += "\n" + TextWidget.Cast(root.FindAnyWidget("StatLabel" + i)).GetText() + "=" + TextWidget.Cast(root.FindAnyWidget("StatValue" + i)).GetText();
		return result + "\nammo=" + TextWidget.Cast(root.FindAnyWidget("DebriefAmmo")).GetText();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDebriefCache(string action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "unavailable: local controller";
		return controller.GC_TestDebriefCache(action);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDebriefSelect(string factionKey)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!GC_Lobby_DebriefMenu.Cast(menu))
			return "unavailable: debrief not open";
		Widget target = menu.GetRootWidget().FindAnyWidget("DebriefSelection_" + factionKey);
		if (!target || !target.IsVisibleInHierarchy() || !target.IsEnabled())
			return "unavailable: selection";
		SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
		if (!button)
			return "unavailable: button";
		button.OnClick(target, 0, 0, 0);
		return "dispatched production selection";
	}
}

//! Test-only counters observe actual requests and deliveries, not timing assumptions.
modded class GC_LobbyPlayerController
{
	protected int m_iGC_TestDebriefRequests;
	protected int m_iGC_TestDebriefDeliveries;

	//------------------------------------------------------------------------------------------------
	override protected void GC_RpcAsk_Debrief()
	{
		m_iGC_TestDebriefRequests++;
		super.GC_RpcAsk_Debrief();
	}

	//------------------------------------------------------------------------------------------------
	override protected void GC_RpcDo_Debrief(string factionKey, string summary, string ammunition, bool complete)
	{
		m_iGC_TestDebriefDeliveries++;
		super.GC_RpcDo_Debrief(factionKey, summary, ammunition, complete);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDebriefCache(string action)
	{
		if (action == "clear")
			GC_ClearDebriefCache();
		else if (action == "cycle")
		{
			GC_LobbyManager manager = GC_LobbyManager.GetManager();
			if (!manager || m_mGC_DebriefCache.Count() < 3)
				return "unavailable: three cached views";
			array<string> keys = {"USSR", "US", ""};
			for (int i = 0; i < 12; i++)
			{
				string key = keys[i % keys.Count()];
				GC_StatsCachedReport cached = m_mGC_DebriefCache.Get(key);
				if (!cached)
					return "FAIL: fixture faction absent";
				manager.GC_TestDebriefSelect(key);
				Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget();
				string status = TextWidget.Cast(root.FindAnyWidget("DebriefStatus")).GetText();
				string ammo = TextWidget.Cast(root.FindAnyWidget("DebriefAmmo")).GetText();
				if (status == "Loading mission statistics…" || ammo != cached.m_sAmmunition)
					return string.Format("FAIL: selection %1 not rendered synchronously", key);
				array<string> values = {};
				cached.m_sSummary.Split("\n", values, false);
				if (values.Count() == 15)
				{
					for (int row = 0; row < 15; row++)
					{
						if (TextWidget.Cast(root.FindAnyWidget("StatValue" + row)).GetText() != values[row])
							return "FAIL: cached counter mismatch";
					}
				}
				else if (status != cached.m_sSummary)
					return "FAIL: cached empty report mismatch";
			}
		}
		else if (action != "snapshot")
			return "unavailable: expected snapshot, cycle or clear";
		return string.Format("cached=%1 requested=%2 serverRequests=%3 deliveries=%4", m_mGC_DebriefCache.Count(), m_bGC_DebriefRequested, m_iGC_TestDebriefRequests, m_iGC_TestDebriefDeliveries);
	}
}

//! Opt-in serialization/write benchmark uses a separate unmarked file, never the live outbox.
modded class GC_MissionStatsComponent
{
	//------------------------------------------------------------------------------------------------
	//! Exercise deletion then disconnect in one call, before the sampler can repair tracking.
	string GC_TestPlaytimeDeletionOrder()
	{
		if (!IsCollecting())
			return "unavailable: active collection required";
		GC_StatsPlayer player = new GC_StatsPlayer();
		GC_StatsAssignment assignment = new GC_StatsAssignment();
		assignment.m_Start = new GC_StatsCounters();
		player.m_ActiveAssignment = assignment;
		player.m_aAssignments.Insert(assignment);
		GC_StatsCharacterTracker tracker = new GC_StatsCharacterTracker();
		tracker.Start(this, 2147483647, player);
		tracker.GC_TestSetLife(GetOwner());
		m_mTrackers.Set("deletion-order-fixture", tracker);
		OnControllableDeleted(GetOwner());
		bool retained = m_mTrackers.Contains("deletion-order-fixture");
		OnPlayerDisconnected(2147483647, null, 0);
		float closedTime = player.m_fPlaySeconds;
		player.SyncAssignment(GetDuration() + 10);
		m_mTrackers.Remove("deletion-order-fixture");
		if (!retained || player.m_ActiveAssignment || player.m_fPlaySeconds != closedTime)
			return "FAIL: body deletion lost disconnect ownership";
		return "PASS: deletion retains connection; immediate disconnect closes time before next sample";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsRadioIntervals()
	{
		if (!IsCollecting() || GetDuration() < 12)
			return "unavailable: collect for at least 12 seconds";
		float until = GetDuration();
		GC_StatsPlayer player = new GC_StatsPlayer();
		GC_StatsAssignment first = new GC_StatsAssignment();
		first.m_fFrom = until - 10;
		first.m_fUntil = until - 6;
		player.m_aAssignments.Insert(first);
		GC_StatsAssignment second = new GC_StatsAssignment();
		second.m_fFrom = until - 6;
		second.m_fUntil = until;
		second.m_Start = new GC_StatsCounters();
		player.m_aAssignments.Insert(second);
		player.m_ActiveAssignment = second;
		CreditRadio(player, 10, m_fStarted + until - 10, m_fStarted + until);
		player.SyncAssignment(until);
		if (Math.AbsFloat(player.m_fRadioSecondsReported - 10) > 0.01
			|| Math.AbsFloat(first.m_fRadioSecondsReported - 4) > 0.01
			|| Math.AbsFloat(second.m_fRadioSecondsReported - 6) > 0.01)
			return string.Format("FAIL total=%1 first=%2 second=%3", player.m_fRadioSecondsReported, first.m_fRadioSecondsReported, second.m_fRadioSecondsReported);
		return "PASS: ten seconds retained across 4/6 second membership intervals";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsLoad()
	{
		if (!Replication.IsServer() || GetInstance() != this)
			return "unavailable: active authority required";
		GC_MissionStatsBatch fixture = new GC_MissionStatsBatch();
		fixture.m_sMissionId = PersistenceIdUtils.Generate();
		fixture.m_fDuration = 1920;
		GC_StatsPopulation population = new GC_StatsPopulation();
		population.m_iPlayers = 128;
		fixture.m_aPopulation.Insert(population);
		for (int i = 0; i < 128; i++)
		{
			GC_StatsPlayer player = new GC_StatsPlayer();
			player.m_sIdentity = "load-player-" + i;
			player.m_sName = "Load test participant";
			for (int j = 0; j < 32; j++)
			{
				GC_StatsAssignment assignment = new GC_StatsAssignment();
				assignment.m_sFaction = "USSR";
				assignment.m_sGroup = (i / 8).ToString();
				assignment.m_sGroupName = "Fixture squad";
				assignment.m_sRole = "fixture-role";
				assignment.m_fFrom = j * 60;
				assignment.m_fUntil = (j + 1) * 60;
				assignment.m_fPlaySeconds = 60;
				player.m_fPlaySeconds += 60;
				for (int k = 0; k < 2; k++)
				{
					GC_StatsAmmo ammo = new GC_StatsAmmo();
					ammo.m_sProjectile = "{1D9DDE1632F33A9E}Prefabs/Weapons/Ammo/Ammo_545x39_Ball_7N6.et" + k;
					ammo.m_sMagazine = "{0A84AA5A3884176F}Prefabs/Weapons/Magazines/Magazine_545x39_AK_30rnd_Last_5Tracer.et";
					ammo.m_sWeapon = "{EB404DC9E1BCB750}Prefabs/Weapons/Rifles/AK74/Rifle_AK74N_1P29.et";
					ammo.m_iRounds = 30;
					assignment.m_aAmmo.Insert(ammo);
					assignment.m_iShots += ammo.m_iRounds;
					player.m_iShots += ammo.m_iRounds;
				}
				player.m_aAssignments.Insert(assignment);
			}
			fixture.m_aPlayers.Insert(player);
		}
		int start = System.GetTickCount();
		string payload = fixture.Encode();
		int encodeMs = System.GetTickCount(start);
		if (payload.Length() < 1000000)
			return "FAIL: load fixture did not serialize expected large payload";
		FileIO.MakeDirectory("$profile:Lobby2StatsTest");
		start = System.GetTickCount();
		bool saved = GC_StatsOutbox.WriteVerified("$profile:Lobby2StatsTest/load.json", payload);
		return string.Format("players=128 assignments=4096 ammoRows=8192 bytes=%1 encodeMs=%2 writeReadMs=%3 verified=%4", payload.Length(), encodeMs, System.GetTickCount(start), saved);
	}
}

modded class GC_StatsCharacterTracker
{
	//------------------------------------------------------------------------------------------------
	void GC_TestSetLife(IEntity entity)
	{
		m_LifeCharacterId = entity.GetID();
	}
}
