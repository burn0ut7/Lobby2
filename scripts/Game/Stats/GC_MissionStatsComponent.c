class GC_MissionStatsComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Authority owns collection and the disk outbox. No network request runs on the game thread.
class GC_MissionStatsComponent : SCR_BaseGameModeComponent
{
	protected ref GC_MissionStatsBatch m_Batch;
	protected ref map<string, ref GC_StatsPlayer> m_mPlayers = new map<string, ref GC_StatsPlayer>();
	protected ref map<string, ref GC_StatsCharacterTracker> m_mTrackers = new map<string, ref GC_StatsCharacterTracker>();
	protected ref map<EntityID, ref GC_StatsAIDamageTracker> m_mDamageObservers = new map<EntityID, ref GC_StatsAIDamageTracker>();
	protected ref set<EntityID> m_RecordedDeaths = new set<EntityID>();
	protected ref map<EntityID, string> m_mProjectileWeapons = new map<EntityID, string>();
	protected ref array<EntityID> m_aProjectileOrder = {};
	protected int m_iProjectileCursor;
	protected GC_LobbyManager m_Manager;
	protected float m_fLastSample;
	protected float m_fStarted;
	protected float m_fLastFlush;
	protected int m_iDeathSequence;
	protected bool m_bCollecting;
	protected bool m_bWriteFailureLogged;
	protected string m_sPendingPayload;
	protected int m_iPendingDeathCount;
	protected bool m_bPendingFinal;
	protected bool m_bAwaitingFinal;
	protected int m_iAssignmentCount;
	protected int m_iAmmoTypeCount;
	protected ref map<string, ref GC_StatsDebrief> m_mPersonalReports = new map<string, ref GC_StatsDebrief>();
	protected ref map<string, ref GC_StatsDebrief> m_mFactionReports = new map<string, ref GC_StatsDebrief>();
	static const string OUTBOX = "$profile:Lobby2Stats";
	static const int MAX_PENDING_DEATHS = 256;

	//------------------------------------------------------------------------------------------------
	//! Diagnostic preview of the next batch, including only death events not yet persisted.
	string GetPendingBatchPreview()
	{
		if (!m_Batch || !Replication.IsServer())
			return string.Empty;
		m_Batch.m_fDuration = GetDuration();
		SyncAssignments();
		return m_Batch.Encode();
	}

	//------------------------------------------------------------------------------------------------
	float GetDuration()
	{
		if (!m_Batch)
			return 0;
		if (IsCollecting())
			return GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted;
		return m_Batch.m_fDuration;
	}

	//------------------------------------------------------------------------------------------------
	//! Small personal report, independent of drained persistent death-event batches.
	GC_StatsDebrief GetDebrief(int playerId, string factionKey)
	{
		if (!Replication.IsServer() || !m_Batch || !m_Batch.m_bFinal)
			return null;
		if (!factionKey.IsEmpty())
			return m_mFactionReports.Get(factionKey);
		return m_mPersonalReports.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId));
	}

	//------------------------------------------------------------------------------------------------
	bool IsFinalized()
	{
		return m_Batch && m_Batch.m_bFinal;
	}

	//------------------------------------------------------------------------------------------------
	bool IsHistoryComplete()
	{
		return m_Batch && m_Batch.m_bComplete;
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerCharacterChanged(int playerId, IEntity character)
	{
		GC_StatsPlayer record = GetPlayer(playerId);
		if (!record || !m_Manager)
			return;
		GC_StatsCharacterTracker tracker = m_mTrackers.Get(record.m_sIdentity);
		if (!tracker)
		{
			tracker = new GC_StatsCharacterTracker();
			m_mTrackers.Set(record.m_sIdentity, tracker);
		}
		tracker.Start(this, playerId, record);
		tracker.Sample(character, 0, m_Manager);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildDebriefs()
	{
		foreach (GC_StatsPlayer player : m_Batch.m_aPlayers)
		{
			GC_StatsDebrief personal = new GC_StatsDebrief();
			personal.UsePersonalCounters(player);
			foreach (GC_StatsAssignment assignment : player.m_aAssignments)
			{
				personal.AddAssignment(assignment, player.m_sIdentity, false);
				if (assignment.m_sFaction.IsEmpty())
					continue;
				GC_StatsDebrief faction = m_mFactionReports.Get(assignment.m_sFaction);
				if (!faction)
				{
					faction = new GC_StatsDebrief();
					m_mFactionReports.Set(assignment.m_sFaction, faction);
				}
				faction.AddAssignment(assignment, player.m_sIdentity, true);
			}
			personal.Seal(true);
			m_mPersonalReports.Set(player.m_sIdentity, personal);
		}
		foreach (string key, GC_StatsDebrief report : m_mFactionReports)
			report.Seal(false);
	}

	//------------------------------------------------------------------------------------------------
	bool AcceptsRadioReport()
	{
		return Replication.IsServer() && GetInstance() == this && (m_bCollecting || m_bAwaitingFinal);
	}

	//------------------------------------------------------------------------------------------------
	//! Client-estimated speech is apportioned across overlapping membership intervals.
	void CreditRadio(GC_StatsPlayer player, float seconds, float serverFrom, float serverUntil)
	{
		if (!AcceptsRadioReport() || !player || seconds <= 0)
			return;
		float from = Math.Max(0, serverFrom - m_fStarted);
		float until = Math.Min(GetDuration(), serverUntil - m_fStarted);
		player.SyncAssignment(until);
		float covered;
		foreach (GC_StatsAssignment assignment : player.m_aAssignments)
			covered += Math.Max(0, Math.Min(until, assignment.m_fUntil) - Math.Max(from, assignment.m_fFrom));
		if (covered <= 0)
			return;
		seconds = Math.Min(seconds, covered);
		player.m_fRadioSecondsReported += seconds;
		foreach (GC_StatsAssignment segment : player.m_aAssignments)
		{
			float overlap = Math.Max(0, Math.Min(until, segment.m_fUntil) - Math.Max(from, segment.m_fFrom));
			segment.m_fRadioSecondsReported += seconds * overlap / covered;
		}
	}

	//------------------------------------------------------------------------------------------------
	GC_StatsPlayer GetRecordedPlayer(int playerId)
	{
		return m_mPlayers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId));
	}

	//------------------------------------------------------------------------------------------------
	void MarkIncomplete()
	{
		if (m_Batch)
			m_Batch.m_bComplete = false;
	}

	//------------------------------------------------------------------------------------------------
	bool AllocateAmmoType()
	{
		if (m_iAmmoTypeCount >= 8192)
		{
			MarkIncomplete();
			return false;
		}
		m_iAmmoTypeCount++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateAssignment(int playerId, GC_StatsPlayer player)
	{
		if (!IsCollecting() || !m_Manager || !player || playerId <= 0
			|| !GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			|| SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId) != player.m_sIdentity)
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted;
		GC_Slot slot = m_Manager.GetSlotByPlayerId(playerId);
		SCR_AIGroup group = m_Manager.GetPlayerGroup(playerId);
		Faction faction = m_Manager.GetPlayerFaction(playerId);
		string factionKey;
		string groupId;
		string groupName;
		string leader;
		string role;
		int slotId = -1;
		if (faction)
			factionKey = faction.GetFactionKey();
		if (group)
		{
			groupId = group.GetGroupID().ToString();
			groupName = GC_GroupCallsignHelper.GetDisplayName(group);
			if (group.GetLeaderID() > 0)
				leader = SCR_PlayerIdentityUtils.GetPlayerIdentityId(group.GetLeaderID());
		}
		if (slot)
		{
			slotId = slot.GetID();
			role = slot.GetAuthoredPrefab();
		}
		player.SyncAssignment(now);
		GC_StatsAssignment current = player.m_ActiveAssignment;
		if (current && current.m_sFaction == factionKey && current.m_sGroup == groupId
			&& current.m_sLeader == leader && current.m_sRole == role && current.m_iSlotId == slotId)
			return;
		player.m_ActiveAssignment = null;
		if (player.m_aAssignments.Count() >= 256 || m_iAssignmentCount >= 4096)
		{
			MarkIncomplete();
			return;
		}
		GC_StatsAssignment assignment = new GC_StatsAssignment();
		m_iAssignmentCount++;
		assignment.m_sFaction = factionKey;
		assignment.m_sGroup = groupId;
		assignment.m_sGroupName = groupName;
		assignment.m_sLeader = leader;
		assignment.m_sRole = role;
		assignment.m_iSlotId = slotId;
		assignment.m_fFrom = now;
		assignment.m_fUntil = now;
		assignment.m_Start = new GC_StatsCounters();
		assignment.m_Start.Add(player);
		player.m_aAssignments.Insert(assignment);
		player.m_ActiveAssignment = assignment;
	}

	//------------------------------------------------------------------------------------------------
	void ObserveProjectile(BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!IsCollecting() || !weapon || !projectile || !weapon.GetOwner())
			return;
		EntityPrefabData prefab = weapon.GetOwner().GetPrefabData();
		if (!prefab)
			return;
		EntityID id = projectile.GetID();
		if (m_mProjectileWeapons.Contains(id))
			return;
		if (m_aProjectileOrder.Count() < 4096)
			m_aProjectileOrder.Insert(id);
		else
		{
			m_mProjectileWeapons.Remove(m_aProjectileOrder[m_iProjectileCursor]);
			m_aProjectileOrder[m_iProjectileCursor] = id;
			m_iProjectileCursor = (m_iProjectileCursor + 1) % 4096;
		}
		m_mProjectileWeapons.Set(id, prefab.GetPrefabName());
	}

	//------------------------------------------------------------------------------------------------
	string GetProjectileWeapon(IEntity projectile)
	{
		if (!projectile)
			return string.Empty;
		// Melee contexts carry the weapon itself instead of a projectile.
		if (projectile.FindComponent(BaseWeaponComponent) && projectile.GetPrefabData())
			return projectile.GetPrefabData().GetPrefabName();
		return m_mProjectileWeapons.Get(projectile.GetID());
	}

	//------------------------------------------------------------------------------------------------
	static GC_MissionStatsComponent GetInstance()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode)
			return null;
		return GC_MissionStatsComponent.Cast(mode.FindComponent(GC_MissionStatsComponent));
	}

	//------------------------------------------------------------------------------------------------
	bool IsCollecting()
	{
		return m_bCollecting && Replication.IsServer() && GetInstance() == this;
	}

	//------------------------------------------------------------------------------------------------
	GC_StatsPlayer GetPlayer(int playerId)
	{
		if (!IsCollecting() || playerId <= 0)
			return null;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (identity.IsEmpty())
			return null;
		GC_StatsPlayer player = m_mPlayers.Get(identity);
		if (!player)
		{
			if (m_mPlayers.Count() >= 1024)
			{
				m_Batch.m_bComplete = false;
				return null;
			}
			player = new GC_StatsPlayer();
			player.m_sIdentity = identity;
			m_mPlayers.Set(identity, player);
			m_Batch.m_aPlayers.Insert(player);
		}
		player.m_sName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		return player;
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		if (!Replication.IsServer() || GetInstance() != this || m_Batch)
			return;
		m_Manager = GC_LobbyManager.GetManager();
		if (!m_Manager)
			return;
		m_Batch = new GC_MissionStatsBatch();
		m_Batch.m_sMissionId = PersistenceIdUtils.Generate();
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs && !logs.GetSessionId().IsEmpty())
			m_Batch.m_sMissionId = logs.GetSessionId();
		m_Batch.m_sWorld = GetGame().GetWorldFile();
		m_Batch.m_iStartedAt = System.GetUnixTime();
		m_fStarted = GetGame().GetWorld().GetWorldTime() * 0.001;
		m_fLastSample = m_fStarted;
		m_fLastFlush = m_fStarted;
		m_bCollecting = true;
		m_Manager.m_OnSlotPresentationChanged.Insert(OnSlotChanged);
		Sample();
		GetGame().GetCallqueue().CallLater(Sample, 1000, true);
		PrintFormat("GC | Stats | started mission=%1", m_Batch.m_sMissionId);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlotChanged(GC_Slot slot)
	{
		if (!IsCollecting() || !slot)
			return;
		GC_StatsCharacterTracker tracker = m_mTrackers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(slot.GetPlayerId()));
		if (tracker)
			tracker.UpdateRole(m_Manager);
	}

	//------------------------------------------------------------------------------------------------
	protected void Sample()
	{
		if (!IsCollecting())
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		float elapsed = Math.Max(0, now - m_fLastSample);
		m_fLastSample = now;
		m_Batch.m_fDuration = now - m_fStarted;
		array<int> connected = {};
		PlayerManager players = GetGame().GetPlayerManager();
		players.GetPlayers(connected);
		RecordPopulation(connected.Count());
		foreach (int playerId : connected)
		{
			GC_StatsPlayer player = GetPlayer(playerId);
			if (!player)
				continue;
			GC_StatsCharacterTracker tracker = m_mTrackers.Get(player.m_sIdentity);
			if (!tracker)
			{
				tracker = new GC_StatsCharacterTracker();
				tracker.Start(this, playerId, player);
				m_mTrackers.Set(player.m_sIdentity, tracker);
				tracker.Sample(players.GetPlayerControlledEntity(playerId), 0, m_Manager);
				continue;
			}
			if (tracker.GetPlayerId() != playerId)
			{
				tracker.Start(this, playerId, player);
				tracker.Sample(players.GetPlayerControlledEntity(playerId), 0, m_Manager);
				continue;
			}
			tracker.Sample(players.GetPlayerControlledEntity(playerId), elapsed, m_Manager);
		}
		for (int i = m_mTrackers.Count() - 1; i >= 0; i--)
		{
			GC_StatsCharacterTracker tracked = m_mTrackers.GetElement(i);
			int id = tracked.GetPlayerId();
			if (id > 0 && (!connected.Contains(id) || SCR_PlayerIdentityUtils.GetPlayerIdentityId(id) != m_mTrackers.GetKey(i)))
				tracked.Disconnect(GetDuration());
		}
		if (now - m_fLastFlush >= 30)
		{
			m_fLastFlush = now;
			Flush();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Store only changes, not one row per tick. Bounds protect the cumulative outbox.
	protected void RecordPopulation(int count)
	{
		int size = m_Batch.m_aPopulation.Count();
		if (size > 0 && m_Batch.m_aPopulation[size - 1].m_iPlayers == count)
			return;
		if (size >= 4096)
		{
			MarkIncomplete();
			return;
		}
		GC_StatsPopulation point = new GC_StatsPopulation();
		if (size > 0)
			point.m_fFrom = GetDuration();
		point.m_iPlayers = count;
		m_Batch.m_aPopulation.Insert(point);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (!IsCollecting())
			return;
		array<int> connected = {};
		GetGame().GetPlayerManager().GetPlayers(connected);
		if (!connected.Contains(playerId))
			connected.Insert(playerId);
		RecordPopulation(connected.Count());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);
		if (IsCollecting())
			OnPlayerCharacterChanged(playerId, GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		foreach (string identity, GC_StatsCharacterTracker tracker : m_mTrackers)
		{
			if (tracker.GetPlayerId() == playerId)
				tracker.Disconnect(GetDuration());
		}
		if (IsCollecting())
		{
			array<int> connected = {};
			GetGame().GetPlayerManager().GetPlayers(connected);
			connected.RemoveItem(playerId);
			RecordPopulation(connected.Count());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Called only by the owning respawn component on authority, immediately before ForceDeath.
	void MarkRespawn(int playerId, IEntity victim)
	{
		GC_StatsCharacterTracker tracker = m_mTrackers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId));
		if (IsCollecting() && tracker)
			tracker.MarkRespawn(victim);
	}

	//------------------------------------------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);
		if (!IsCollecting())
			return;
		IEntity victim = instigatorContextData.GetVictimEntity();
		if (!victim || !ChimeraCharacter.Cast(victim) || victim.FindComponent(GC_LobbySpectatorBodyComponent)
			|| m_RecordedDeaths.Contains(victim.GetID()))
			return;
		GC_StatsPlayer victimRecord = GetPlayer(instigatorContextData.GetVictimPlayerID());
		GC_StatsCharacterTracker tracker;
		foreach (string identity, GC_StatsCharacterTracker candidate : m_mTrackers)
		{
			if (candidate.TracksLife(victim))
			{
				tracker = candidate;
				victimRecord = candidate.GetRecord();
				break;
			}
		}
		GC_StatsPlayer killerRecord = GetPlayer(instigatorContextData.GetKillerPlayerID());
		UpdateAssignment(instigatorContextData.GetVictimPlayerID(), victimRecord);
		UpdateAssignment(instigatorContextData.GetKillerPlayerID(), killerRecord);
		GC_StatsDeath death = new GC_StatsDeath();
		death.m_iRelation = instigatorContextData.GetVictimKillerRelation();
		IEntity killer = instigatorContextData.GetKillerEntity();
		if (killer)
			death.m_fDistance = vector.Distance(victim.GetOrigin(), killer.GetOrigin());
		if (tracker)
		{
			death.m_sWeapon = tracker.GetDamageWeapon(instigatorContextData.GetKillerPlayerID(), victim);
			death.m_sCause = tracker.GetDeathCause(victim);
			if (death.m_sCause == "respawn" || death.m_sCause == "bleeding")
			{
				GC_StatsPlayer attacker = tracker.ApplyDelayedCredit(victim, death);
				if (attacker)
					killerRecord = attacker;
			}
		}
		// A disconnected player's tracked body remains a player victim, not an AI kill.
		if (victimRecord || instigatorContextData.GetVictimPlayerID() > 0)
			death.m_sVictimKind = "player";
		else if (instigatorContextData.GetVictimCharacterControlType() == SCR_ECharacterControlType.AI)
			death.m_sVictimKind = "ai";
		if (killerRecord)
			death.m_sKillerKind = "player";
		else if (instigatorContextData.GetKillerCharacterControlType() == SCR_ECharacterControlType.AI)
			death.m_sKillerKind = "ai";
		if (victim.GetPrefabData())
			death.m_sVictimPrefab = victim.GetPrefabData().GetPrefabName();
		if (killer && killer.GetPrefabData() && death.m_sCreditSource != "lastPlayerDamage")
			death.m_sKillerPrefab = killer.GetPrefabData().GetPrefabName();
		if (death.m_sCreditSource.IsEmpty() && ((killerRecord && killerRecord != victimRecord) || death.m_sKillerKind == "ai"))
			death.m_sCreditSource = "engine";
		bool teamKill = (death.m_iRelation & SCR_ECharacterDeathStatusRelations.KILLED_BY_FRIENDLY_PLAYER) != 0;
		if (!victimRecord && !killerRecord && death.m_sVictimKind != "ai")
			return;
		if (m_RecordedDeaths.Count() >= 16384)
		{
			MarkIncomplete();
			return;
		}
		m_RecordedDeaths.Insert(victim.GetID());
		if (victimRecord)
		{
			bool byPlayer = killerRecord && killerRecord != victimRecord;
			bool byAI = death.m_sKillerKind == "ai";
			victimRecord.RecordDeath(byPlayer, byAI);
			if (!victimRecord.m_ActiveAssignment && !victimRecord.m_aAssignments.IsEmpty())
				victimRecord.m_aAssignments[victimRecord.m_aAssignments.Count() - 1].RecordDeath(byPlayer, byAI);
		}
		if (killerRecord && killerRecord != victimRecord && death.m_sVictimKind != "other")
		{
			bool aiVictim = death.m_sVictimKind == "ai";
			killerRecord.RecordKill(aiVictim, teamKill);
			// A disconnected attacker has no open interval to receive later counter differences.
			if (!killerRecord.m_ActiveAssignment && !killerRecord.m_aAssignments.IsEmpty())
			{
				GC_StatsAssignment lastAssignment = killerRecord.m_aAssignments[killerRecord.m_aAssignments.Count() - 1];
				lastAssignment.RecordKill(aiVictim, teamKill);
			}
		}
		death.m_iSequence = ++m_iDeathSequence;
		death.m_fMissionSeconds = GetGame().GetWorld().GetWorldTime() * 0.001 - m_fStarted;
		if (victimRecord)
		{
			death.m_sVictim = victimRecord.m_sIdentity;
			death.m_sVictimName = victimRecord.m_sName;
		}
		if (killerRecord)
		{
			death.m_sKiller = killerRecord.m_sIdentity;
			death.m_sKillerName = killerRecord.m_sName;
		}
		if (death.m_sVictimKind == "ai")
		{
			death.m_sVictim = "ai:" + victim.GetID().ToString();
			death.m_sVictimName = "AI";
		}
		if (death.m_sKillerKind == "ai")
		{
			death.m_sKillerName = "AI";
			if (killer)
				death.m_sKiller = "ai:" + killer.GetID().ToString();
		}
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
		{
			string type = "death";
			if (killerRecord && killerRecord != victimRecord)
			{
				type = "kill";
				if (victimRecord && teamKill)
					type = "teamKill";
				else if (death.m_sVictimKind == "ai")
					type = "aiKill";
			}
			GC_SessionEvent entry = logs.CreateEvent(type, instigatorContextData.GetKillerPlayerID(), instigatorContextData.GetVictimPlayerID());
			entry.m_sTargetEntity = victim.GetID().ToString();
			if (killer && death.m_sCreditSource != "lastPlayerDamage")
				entry.m_sActorEntity = killer.GetID().ToString();
			entry.m_sActor = death.m_sKiller;
			entry.m_sActorName = death.m_sKillerName;
			entry.m_sTarget = death.m_sVictim;
			entry.m_sTargetName = death.m_sVictimName;
			entry.m_sActorKind = death.m_sKillerKind;
			entry.m_sTargetKind = death.m_sVictimKind;
			entry.m_sActorPrefab = death.m_sKillerPrefab;
			entry.m_sTargetPrefab = death.m_sVictimPrefab;
			entry.m_sWeapon = death.m_sWeapon;
			entry.m_fDistance = death.m_fDistance;
			entry.m_sDeathCause = death.m_sCause;
			entry.m_sCreditSource = death.m_sCreditSource;
			entry.m_iDamageEventSequence = death.m_iDamageEventSequence;
			if (death.m_sCause == "respawn")
				entry.m_sDetail = "Respawn selected";
			else if (death.m_sCause == "bleeding")
				entry.m_sDetail = "Bled to death";
			if (death.m_sCreditSource == "lastPlayerDamage")
				entry.m_sDetail += " | last damaging player credited";
			logs.RecordDeath(entry, victim, killer);
		}
		// AI-only deaths belong in the session event feed, not player statistics.
		if (!victimRecord && !killerRecord)
			return;
		if (m_Batch.m_aDeaths.Count() >= MAX_PENDING_DEATHS)
		{
			if (m_Batch.m_bComplete)
				Print("GC | Stats | death queue full; history marked incomplete", LogLevel.ERROR);
			m_Batch.m_bComplete = false;
			return;
		}
		m_Batch.m_aDeaths.Insert(death);
	}

	//------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		if (!Replication.IsServer() || GetInstance() != this || !ChimeraCharacter.Cast(entity)
			|| entity.FindComponent(GC_LobbySpectatorBodyComponent) || (m_Batch && !m_bCollecting)
			|| m_mDamageObservers.Contains(entity.GetID()))
			return;
		GC_StatsAIDamageTracker observer = new GC_StatsAIDamageTracker();
		observer.Start(this, entity);
		m_mDamageObservers.Set(entity.GetID(), observer);
	}

	//------------------------------------------------------------------------------------------------
	//! Native possession and retained player lives take precedence over AI classification.
	bool IsPlayerCharacter(IEntity entity)
	{
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity) > 0)
			return true;
		foreach (string identity, GC_StatsCharacterTracker tracker : m_mTrackers)
		{
			if (tracker.TracksLife(entity))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnControllableDeleted(IEntity entity)
	{
		if (entity)
		{
			GC_StatsAIDamageTracker observer = m_mDamageObservers.Get(entity.GetID());
			if (observer)
				observer.Detach();
			m_mDamageObservers.Remove(entity.GetID());
			m_RecordedDeaths.RemoveItem(entity.GetID());
			for (int i = m_mTrackers.Count() - 1; i >= 0; i--)
			{
				GC_StatsCharacterTracker tracker = m_mTrackers.GetElement(i);
				if (tracker.TracksLife(entity))
				{
					tracker.Detach();
					// Connected membership outlives a body, including deletion before disconnect.
					if (tracker.GetPlayerId() <= 0)
						m_mTrackers.Remove(m_mTrackers.GetKey(i));
				}
			}
		}
		super.OnControllableDeleted(entity);
	}

	//------------------------------------------------------------------------------------------------
	//! The marker is created only after payload write/readback. Consumers read marked batches only.
	protected bool Flush()
	{
		if (!m_Batch)
			return false;
		FileIO.MakeDirectory(OUTBOX);
		if (m_sPendingPayload.IsEmpty())
		{
			m_Batch.m_fDuration = GetDuration();
			SyncAssignments();
			m_sPendingPayload = m_Batch.Encode();
			if (m_sPendingPayload.IsEmpty())
			{
				Print("GC | Stats | serialization failed; retaining all pending events", LogLevel.ERROR);
				return false;
			}
			m_iPendingDeathCount = m_Batch.m_aDeaths.Count();
			m_bPendingFinal = m_Batch.m_bFinal;
		}
		string path = string.Format("%1/%2-%3.json", OUTBOX, m_Batch.m_sMissionId, m_Batch.m_iRevision);
		bool written = GC_StatsOutbox.WriteVerified(path, m_sPendingPayload) && GC_StatsOutbox.WriteReady(path + ".ready");
		if (!written)
		{
			if (!m_bWriteFailureLogged)
				PrintFormat("GC | Stats | outbox write failed path=%1; retaining pending events", path, level: LogLevel.ERROR);
			m_bWriteFailureLogged = true;
			return false;
		}
		m_bWriteFailureLogged = false;
		m_Batch.m_iRevision++;
		for (int i = 0; i < m_iPendingDeathCount; i++)
			m_Batch.m_aDeaths.RemoveOrdered(0);
		m_sPendingPayload = string.Empty;
		if (m_Batch.m_bFinal && !m_bPendingFinal)
			return Flush();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		if (!IsCollecting())
			return;
		Sample();
		Stop();
		m_bAwaitingFinal = true;
		// Give owners a bounded window to submit their last radio sample; gameplay counters are frozen.
		GetGame().GetCallqueue().CallLater(Finalize, 2000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void Finalize()
	{
		m_bAwaitingFinal = false;
		SyncAssignments();
		m_Batch.m_bFinal = true;
		BuildDebriefs();
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller)
				controller.GC_SendDebrief();
		}
		if (!Flush())
			GetGame().GetCallqueue().CallLater(RetryFinal, 30000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void RetryFinal()
	{
		if (GetInstance() != this || Flush())
			GetGame().GetCallqueue().Remove(RetryFinal);
	}

	//------------------------------------------------------------------------------------------------
	protected void SyncAssignments()
	{
		foreach (GC_StatsPlayer player : m_Batch.m_aPlayers)
			player.SyncAssignment(m_Batch.m_fDuration);
	}

	//------------------------------------------------------------------------------------------------
	protected void Stop()
	{
		m_bCollecting = false;
		if (GetGame())
			GetGame().GetCallqueue().Remove(Sample);
		if (m_Manager)
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlotChanged);
		foreach (string identity, GC_StatsCharacterTracker tracker : m_mTrackers)
			tracker.Detach();
		m_mTrackers.Clear();
		foreach (EntityID entityId, GC_StatsAIDamageTracker observer : m_mDamageObservers)
			observer.Detach();
		m_mDamageObservers.Clear();
		m_Manager = null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		if (IsCollecting())
			Sample();
		if (m_bAwaitingFinal)
			Finalize();
		else if (IsCollecting() || !m_sPendingPayload.IsEmpty())
			Flush();
		Stop();
		GetGame().GetCallqueue().Remove(Finalize);
		GetGame().GetCallqueue().Remove(RetryFinal);
		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MissionStatsComponent()
	{
		Stop();
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(RetryFinal);
			GetGame().GetCallqueue().Remove(Finalize);
		}
	}
}
