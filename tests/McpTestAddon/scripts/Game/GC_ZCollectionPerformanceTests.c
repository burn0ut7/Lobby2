#ifdef WORKBENCH
//! Explicit synthetic serializer benchmark; never publishes into production outboxes.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestDamageLinkRetention()
	{
		GC_TelemetryActor first = new GC_TelemetryActor();
		GC_TelemetryActor second = new GC_TelemetryActor();
		second.RememberDamage(77, 88);
		for (int sequence = 1; sequence <= 10000; sequence++)
			first.RememberDamage(sequence, sequence + 100);
		if (first.GetDamageSequence(10000) != 10100 || first.GetDamageSequence(8192) != 0
			|| first.GetDamageSequence(0) != 0 || second.GetDamageSequence(77) != 88 || second.GetDamageSequence(10000) != 0)
			return "FAIL lifetime damage links";
		return "PASS 10000 retained-hit updates; latest link and life isolation";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsPerformance(int assignments)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (GetManager() != this || !Replication.IsServer() || !controller || assignments < 1 || assignments > 32)
			return "UNAVAILABLE";
		if (!AuthorizeAdminAction(controller.GetPlayerId(), "stats-performance-test", assignments.ToString()))
			return "DENIED";
		GC_MissionStatsBatch batch = new GC_MissionStatsBatch();
		batch.m_sMissionId = "00000000-0000-0000-0000-000000000001";
		batch.m_sWorld = "synthetic-performance-fixture";
		batch.m_fDuration = 3600;
		for (int i; i < 128; i++)
		{
			GC_StatsPlayer player = new GC_StatsPlayer();
			player.m_sIdentity = "synthetic-player-" + i;
			player.m_sName = "Performance fixture " + i;
			player.m_fPlaySeconds = 3600;
			player.m_iShots = 600;
			for (int j; j < assignments; j++)
			{
				GC_StatsAssignment segment = new GC_StatsAssignment();
				segment.m_sFaction = "US";
				segment.m_sGroup = "1";
				segment.m_sGroupName = "Alpha 1";
				segment.m_sLeader = "synthetic-player-0";
				segment.m_sRole = "{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et";
				segment.m_iSlotId = i;
				segment.m_fFrom = j * 3600.0 / assignments;
				segment.m_fUntil = (j + 1) * 3600.0 / assignments;
				segment.m_fPlaySeconds = segment.m_fUntil - segment.m_fFrom;
				for (int k; k < 2; k++)
				{
					GC_StatsAmmo ammo = new GC_StatsAmmo();
					ammo.m_sProjectile = "Prefabs/Weapons/Ammo/Ammo_556x45_Ball_M855.et";
					ammo.m_sWeapon = "Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et";
					ammo.m_sMagazine = "synthetic-magazine-" + k;
					ammo.m_iRounds = 300;
					segment.m_aAmmo.Insert(ammo);
				}
				player.m_aAssignments.Insert(segment);
			}
			batch.m_aPlayers.Insert(player);
		}
		int started = System.GetTickCount();
		string payload = batch.Encode();
		int encodeMs = System.GetTickCount(started);
		FileIO.MakeDirectory("$profile:CollectionPerformanceBench");
		started = System.GetTickCount();
		bool saved = GC_StatsOutbox.WriteVerified("$profile:CollectionPerformanceBench/stats.json", payload);
		return string.Format("players=128 assignments=%1 encodeMs=%2 writeVerifyMs=%3 bytes=%4 verified=%5", assignments, encodeMs, System.GetTickCount(started), payload.Length(), saved);
	}
}
//! Synchronous isolated queue probe: restore production references before returning.
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryMotion(bool enabled)
	{
		PlayerController controller = GetGame().GetPlayerController();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (GetInstance() != this || !Replication.IsServer() || !controller || !manager)
			return "UNAVAILABLE";
		if (!manager.AuthorizeAdminAction(controller.GetPlayerId(), "telemetry-motion-test", enabled.ToString()))
			return "DENIED";
		GetGame().GetCallqueue().Remove(GC_MoveTelemetryFixtures);
		if (enabled)
			GetGame().GetCallqueue().CallLater(GC_MoveTelemetryFixtures, 200, true);
		return GetTelemetryReport();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_MoveTelemetryFixtures()
	{
		if (GetInstance() != this || !m_Telemetry || !m_Telemetry.IsRunning())
		{
			GetGame().GetCallqueue().Remove(GC_MoveTelemetryFixtures);
			return;
		}
		foreach (IEntity actor : m_GC_TestTelemetryActors)
		{
			if (!actor)
				continue;
			actor.SetOrigin(actor.GetOrigin() + Vector(0.02, 0, 0.01));
			actor.Update();
		}
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSessionDrain(bool legacy, int pending)
	{
		PlayerController controller = GetGame().GetPlayerController();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (GetInstance() != this || !Replication.IsServer() || !controller || !manager || pending < 0 || pending > 8192)
			return "UNAVAILABLE";
		if (!manager.AuthorizeAdminAction(controller.GetPlayerId(), "session-drain-test", pending.ToString()))
			return "DENIED";
		ref array<ref GC_SessionEvent> original = m_aQueued;
		int originalPending = m_iPendingCount;
		m_aQueued = {};
		for (int index; index < 8192; index++)
		{
			GC_SessionEvent entry = new GC_SessionEvent();
			entry.m_iSequence = index;
			m_aQueued.Insert(entry);
		}
		m_iPendingCount = pending;
		int started = System.GetTickCount();
		if (legacy)
		{
			for (int removed; removed < pending; removed++)
				m_aQueued.RemoveOrdered(0);
		}
		else
			DrainPublishedEvents();
		int elapsed = System.GetTickCount(started);
		bool valid = m_aQueued.Count() == 8192 - pending;
		foreach (int index, GC_SessionEvent entry : m_aQueued)
			valid = valid && entry.m_iSequence == index + pending;
		m_aQueued = original;
		m_iPendingCount = originalPending;
		return string.Format("legacy=%1 pending=%2 drainMs=%3 valid=%4", legacy, pending, elapsed, valid);
	}
}
#endif
