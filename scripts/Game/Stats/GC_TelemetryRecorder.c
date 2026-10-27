//! Optional authority recorder. Native observations only; never drives animation.
//! Session-log ownership keeps the exact same clock and mission ID as audit events.
class GC_TelemetryRecorder
{
	protected GC_SessionLogComponent m_Logs;
	protected string m_sRecordingId;
	protected string m_sRecords;
	protected string m_sPending;
	protected string m_sStopReason;
	protected int m_iSequence;
	protected int m_iFirstSequence = 1;
	protected int m_iRevision;
	protected int m_iRecordCount;
	protected int m_iDropped;
	protected float m_fBytes;
	protected float m_fByteLimit;
	protected int m_iActorSequence;
	protected int m_iRetiredCursor;
	protected ref array<ref GC_TelemetryActor> m_aRetired = {};
	protected int m_iCursor;
	protected int m_iSteps;
	protected int m_iCaptureMs;
	protected int m_iMaxCaptureMs;
	protected int m_iWriteMs;
	protected int m_iMaxWriteMs;
	protected float m_fFrom;
	protected float m_fLastDiscovery = -1000;
	protected float m_fLastFlush;
	protected bool m_bRunning;
	protected bool m_bSealed;
	protected bool m_bPendingClosed;
	protected ref array<int> m_aPlayers = {};
	protected int m_iPlayerCursor;
	protected ref array<ref GC_TelemetryActor> m_aActors = {};
	protected ref map<EntityID, GC_TelemetryActor> m_mActors = new map<EntityID, GC_TelemetryActor>();
	protected ref map<EntityID, ref GC_TelemetryProjectile> m_mProjectiles = new map<EntityID, ref GC_TelemetryProjectile>();
	protected ref array<ref GC_TelemetryProjectile> m_aProjectileOrder = {};
	protected int m_iProjectileCursor;
	protected int m_iProjectileSequence;
	protected ref map<int, int> m_mDamageSequences = new map<int, int>();
	static const string OUTBOX = "$profile:Lobby2Telemetry";
	static const int MAX_QUEUED_BYTES = 131072;

	//------------------------------------------------------------------------------------------------
	void Start(GC_SessionLogComponent logs, int quotaMiB)
	{
		m_Logs = logs;
		m_fByteLimit = quotaMiB * 1048576.0;
		m_sRecordingId = PersistenceIdUtils.Generate();
		m_fFrom = logs.GetSessionMilliseconds();
		m_fLastFlush = m_fFrom;
		m_bRunning = true;
		FileIO.MakeDirectory(OUTBOX);
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	bool IsSealed()
	{
		return m_bSealed;
	}

	//------------------------------------------------------------------------------------------------
	float Now()
	{
		return m_Logs.GetSessionMilliseconds();
	}

	//------------------------------------------------------------------------------------------------
	string Report()
	{
		return string.Format("recording=%1 running=%2 sealed=%3 records=%4 batches=%5 bytes=%6 dropped=%7 actors=%8", m_sRecordingId, m_bRunning, m_bSealed, m_iSequence, m_iRevision, m_fBytes, m_iDropped, m_aActors.Count())
			+ string.Format(" steps=%1 captureMs=%2 maxCaptureMs=%3 writeMs=%4 maxWriteMs=%5 stop=%6", m_iSteps, m_iCaptureMs, m_iMaxCaptureMs, m_iWriteMs, m_iMaxWriteMs, m_sStopReason)
			+ " quotaMiB=" + (m_fByteLimit / 1048576.0);
	}

	//------------------------------------------------------------------------------------------------
	void Stop(string reason)
	{
		if (!m_bRunning)
			return;
		m_bRunning = false;
		m_sStopReason = reason;
	}

	//------------------------------------------------------------------------------------------------
	//! Shutdown gets one bounded write attempt; failed publication remains explicit.
	void Close()
	{
		Stop("mission-end");
		GetGame().GetCallqueue().Remove(Tick);
		Flush();
		if (!m_bSealed && m_sPending.IsEmpty())
			Flush();
		Print("GC | Telemetry | " + Report());
	}

	//------------------------------------------------------------------------------------------------
	JsonSaveContext BeginRecord(string type)
	{
		JsonSaveContext context = new JsonSaveContext();
		context.SetMaxDecimalPlaces(6);
		context.WriteValue("sequence", m_iSequence + 1);
		context.WriteValue("timeMs", Now());
		context.WriteValue("type", type);
		context.StartObject("data");
		return context;
	}

	//------------------------------------------------------------------------------------------------
	int CommitRecord(JsonSaveContext context)
	{
		if (!m_bRunning)
			return 0;
		context.EndObject();
		string record = context.SaveToString();
		if (!context.IsValid())
		{
			Stop("overflow");
			return 0;
		}
		return CommitEncoded(record);
	}

	//------------------------------------------------------------------------------------------------
	int CommitData(string type, string data)
	{
		JsonSaveContext header = new JsonSaveContext();
		header.SetMaxDecimalPlaces(6);
		header.WriteValue("sequence", m_iSequence + 1);
		header.WriteValue("timeMs", Now());
		header.WriteValue("type", type);
		string encoded = header.SaveToString();
		return CommitEncoded(encoded.Substring(0, encoded.Length() - 1) + ",\"data\":" + data + "}");
	}

	//------------------------------------------------------------------------------------------------
	protected int CommitEncoded(string record)
	{
		if (!m_bRunning)
			return 0;
		if (record.Length() < 2 || m_sRecords.Length() + record.Length() + 1 > MAX_QUEUED_BYTES || m_iRecordCount >= 512)
		{
			m_iDropped++;
			Stop("overflow");
			return 0;
		}
		if (m_iRecordCount > 0)
			m_sRecords += ",";
		m_sRecords += record;
		m_iRecordCount++;
		return ++m_iSequence;
	}

	//------------------------------------------------------------------------------------------------
	static void WriteVector(JsonSaveContext context, string name, vector value)
	{
		array<float> values = {value[0], value[1], value[2]};
		context.WriteValue(name, values);
	}

	//------------------------------------------------------------------------------------------------
	GC_TelemetryActor Track(IEntity entity, bool wasPlayer = false, string identity = "", bool deathEvent = false)
	{
		if (!m_bRunning || !entity || !entity.GetPrefabData() || (!ChimeraCharacter.Cast(entity) && !Vehicle.Cast(entity)) || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return null;
		GC_TelemetryActor actor = m_mActors.Get(entity.GetID());
		if (actor && actor.GetEntity() == entity)
			return actor;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!deathEvent && character && character.GetCharacterController() && character.GetCharacterController().IsDead())
			return null;
		if (m_aActors.Count() >= 256)
		{
			m_iDropped++;
			Stop("overflow");
			return null;
		}
		actor = new GC_TelemetryActor();
		m_aActors.Insert(actor);
		m_mActors.Set(entity.GetID(), actor);
		actor.Start(this, entity, "entity-" + (++m_iActorSequence), wasPlayer, identity);
		return actor;
	}

	//------------------------------------------------------------------------------------------------
	string EntityId(IEntity entity)
	{
		GC_TelemetryActor actor = Track(entity);
		if (actor)
			return actor.GetId();
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	string ProjectileId(IEntity projectile, bool create)
	{
		if (!projectile)
			return string.Empty;
		GC_TelemetryProjectile known = m_mProjectiles.Get(projectile.GetID());
		if (known && known.m_Entity == projectile)
			return known.m_sId;
		if (!create)
			return string.Empty;
		GC_TelemetryProjectile entry = new GC_TelemetryProjectile();
		entry.m_Entity = projectile;
		entry.m_NativeId = projectile.GetID();
		entry.m_sId = "projectile-" + (++m_iProjectileSequence);
		if (m_aProjectileOrder.Count() < 4096)
			m_aProjectileOrder.Insert(entry);
		else
		{
			GC_TelemetryProjectile old = m_aProjectileOrder[m_iProjectileCursor];
			if (m_mProjectiles.Get(old.m_NativeId) == old)
				m_mProjectiles.Remove(old.m_NativeId);
			m_aProjectileOrder[m_iProjectileCursor] = entry;
			m_iProjectileCursor = (m_iProjectileCursor + 1) % 4096;
		}
		m_mProjectiles.Set(entry.m_NativeId, entry);
		return entry.m_sId;
	}

	//------------------------------------------------------------------------------------------------
	void RecordShot(string type, int playerId, BaseWeaponComponent weapon, IEntity projectile, int eventSequence)
	{
		if (!m_bRunning || !projectile || !projectile.GetPrefabData())
			return;
		string actorId = EntityId(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (actorId.IsEmpty())
			return;
		JsonSaveContext context = BeginRecord(type);
		context.WriteValue("entity", actorId);
		context.WriteValue("projectileId", ProjectileId(projectile, true));
		string prefab;
		if (weapon && weapon.GetOwner() && weapon.GetOwner().GetPrefabData())
			prefab = weapon.GetOwner().GetPrefabData().GetPrefabName();
		context.WriteValue("weaponPrefab", prefab);
		context.WriteValue("projectilePrefab", projectile.GetPrefabData().GetPrefabName());
		WriteVector(context, "origin", projectile.GetOrigin());
		vector velocity;
		ProjectileMoveComponent movement = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (movement)
			velocity = movement.GetVelocity();
		else if (projectile.GetPhysics())
			velocity = projectile.GetPhysics().GetVelocity();
		WriteVector(context, "velocity", velocity);
		context.WriteValue("sessionEventSequence", eventSequence);
		CommitRecord(context);
	}

	//------------------------------------------------------------------------------------------------
	void RecordDamage(BaseDamageContext damage, IEntity victim, GC_SessionEvent entry)
	{
		if (!m_bRunning)
			return;
		GC_TelemetryActor victimActor = Track(victim, entry.m_sTargetKind == "player", entry.m_sTarget);
		if (!victimActor)
			return;
		string victimId = victimActor.GetId();
		if (victimId.IsEmpty())
			return;
		IEntity attacker;
		if (damage.instigator)
			attacker = damage.instigator.GetInstigatorEntity();
		string attackerId = EntityId(attacker);
		JsonSaveContext context = BeginRecord("damage");
		context.WriteValue("entity", victimId);
		context.WriteValue("attacker", attackerId);
		context.WriteValue("projectileId", ProjectileId(damage.damageSource, false));
		context.WriteValue("reportedDamage", damage.damageValue);
		int damageType = damage.damageType;
		context.WriteValue("damageType", damageType);
		WriteVector(context, "position", damage.hitPosition);
		context.WriteValue("sessionEventSequence", entry.m_iSequence);
		int sequence = CommitRecord(context);
		if (m_mDamageSequences.Count() < 8192)
			m_mDamageSequences.Set(entry.m_iSequence, sequence);
	}

	//------------------------------------------------------------------------------------------------
	void RecordDeath(GC_SessionEvent entry, IEntity victim, IEntity killer)
	{
		if (!m_bRunning || (entry.m_sType != "death" && entry.m_sType != "kill" && entry.m_sType != "teamKill" && entry.m_sType != "aiKill"))
			return;
		GC_TelemetryActor victimActor = Track(victim, entry.m_sTargetKind == "player", entry.m_sTarget, true);
		if (!victimActor || victimActor.HasEnded())
			return;
		victimActor.Finish();
		string victimId = victimActor.GetId();
		string killerId;
		// Delayed statistical credit does not establish the current killer body.
		if (entry.m_sCreditSource != "lastPlayerDamage")
			killerId = EntityId(killer);
		if (victimId.IsEmpty())
			return;
		JsonSaveContext context = BeginRecord("death");
		context.WriteValue("entity", victimId);
		context.WriteValue("killer", killerId);
		context.WriteValue("damageSequence", m_mDamageSequences.Get(entry.m_iDamageEventSequence));
		context.WriteValue("sessionEventSequence", entry.m_iSequence);
		string cause = entry.m_sDeathCause;
		if (cause.IsEmpty())
			cause = "unknown";
		context.WriteValue("cause", cause);
		WriteVector(context, "position", victim.GetOrigin());
		WriteVector(context, "anglesDegrees", victim.GetAngles());
		CommitRecord(context);
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (GC_SessionLogComponent.GetInstance() != m_Logs || !Replication.IsServer())
		{
			Close();
			return;
		}
		int started = System.GetTickCount();
		float now = Now();
		if (m_bRunning && now - m_fLastDiscovery >= 1000 && m_iPlayerCursor >= m_aPlayers.Count())
		{
			m_fLastDiscovery = now;
			m_aPlayers.Clear();
			GetGame().GetPlayerManager().GetPlayers(m_aPlayers);
			m_iPlayerCursor = 0;
		}
		for (int discovered; m_bRunning && discovered < 8 && m_iPlayerCursor < m_aPlayers.Count(); discovered++)
			Track(GetGame().GetPlayerManager().GetPlayerControlledEntity(m_aPlayers[m_iPlayerCursor++]));
		int count = m_aActors.Count();
		for (int step; m_bRunning && step < count; step++)
		{
			if (m_iCursor >= count)
				m_iCursor = 0;
			GC_TelemetryActor actor = m_aActors[m_iCursor];
			// Successful observations are ordered by this cursor. Wait for the
			// next due actor instead of scanning every actor on every frame.
			if (!actor.IsDue(now))
				break;
			actor.Sample();
			if (actor.HasEnded())
			{
				// Bounded tombstones preserve late death/damage attribution without
				// sampling corpses or admitting them again during player discovery.
				if (m_aRetired.Count() < 256)
					m_aRetired.Insert(actor);
				else
				{
					GC_TelemetryActor old = m_aRetired[m_iRetiredCursor];
					if (m_mActors.Get(old.GetNativeId()) == old)
						m_mActors.Remove(old.GetNativeId());
					m_aRetired[m_iRetiredCursor] = actor;
					m_iRetiredCursor = (m_iRetiredCursor + 1) % 256;
				}
				m_aActors.Remove(m_iCursor);
				count--;
			}
			else
				m_iCursor++;
			if (System.GetTickCount(started) >= 2 || m_sRecords.Length() >= 65536)
				break;
		}
		int elapsed = System.GetTickCount(started);
		m_iSteps++;
		m_iCaptureMs += elapsed;
		m_iMaxCaptureMs = Math.Max(m_iMaxCaptureMs, elapsed);
		if (!m_bRunning || m_sRecords.Length() >= 65536 || m_iRecordCount >= 384 || now - m_fLastFlush >= 5000)
			Flush();
	}

	//------------------------------------------------------------------------------------------------
	protected void Flush()
	{
		if (m_bSealed || (m_bRunning && m_iRecordCount == 0))
			return;
		if (m_sPending.IsEmpty())
		{
			JsonSaveContext context = new JsonSaveContext();
			// Match record timestamp precision, including a start and event in one tick.
			context.SetMaxDecimalPlaces(6);
			context.WriteValue("kind", "telemetry");
			context.WriteValue("schemaVersion", 2);
			context.WriteValue("missionId", m_Logs.GetSessionId());
			context.WriteValue("recordingId", m_sRecordingId);
			context.WriteValue("revision", m_iRevision);
			context.WriteValue("startedAt", m_Logs.GetSessionStartedAt());
			context.WriteValue("world", GetGame().GetWorldFile());
			context.WriteValue("gameVersion", GetGame().GetBuildVersion());
			context.WriteValue("assetManifestId", "native-resources:" + m_sRecordingId);
			context.WriteValue("clock", "session-world-ms");
			context.WriteValue("coordinates", "enfusion-world-metres");
			context.WriteValue("sampleHz", 5);
			context.WriteValue("firstSequence", m_iFirstSequence);
			context.WriteValue("fromMs", m_fFrom);
			context.WriteValue("untilMs", Now());
			bool closed = !m_bRunning;
			m_bPendingClosed = closed;
			context.WriteValue("closed", closed);
			context.WriteValue("stopReason", m_sStopReason);
			context.WriteValue("droppedRecords", m_iDropped);
			context.StartObject("coverage");
			context.WriteValue("actors", "players");
			context.WriteValue("pose", "inferred");
			bool captured = true;
			bool unsupported = false;
			context.WriteValue("shots", captured);
			context.WriteValue("impacts", unsupported);
			context.WriteValue("damage", captured);
			context.WriteValue("grenades", captured);
			context.WriteValue("vehicles", captured);
			context.WriteValue("gear", captured);
			context.EndObject();
			string header = context.SaveToString();
			// Both parts were encoded by the native JSON serializer. Concatenate
			// already-encoded records without reparsing a growing history each frame.
			m_sPending = header.Substring(0, header.Length() - 1) + ",\"records\":[" + m_sRecords + "]}";
		}
		int started = System.GetTickCount();
		string path = string.Format("%1/%2-telemetry-%3.json", OUTBOX, m_sRecordingId, m_iRevision);
		bool saved = GC_StatsOutbox.WriteVerified(path, m_sPending) && GC_StatsOutbox.WriteReady(path + ".ready");
		int elapsed = System.GetTickCount(started);
		m_iWriteMs += elapsed;
		m_iMaxWriteMs = Math.Max(m_iMaxWriteMs, elapsed);
		if (!saved)
		{
			Stop("storage-error");
			GetGame().GetCallqueue().Remove(Tick);
			Print("GC | Telemetry | publication failed; retained pending payload; " + Report(), LogLevel.ERROR);
			return;
		}
		m_fBytes += m_sPending.Length();
		m_iRevision++;
		m_sPending = string.Empty;
		m_sRecords = string.Empty;
		m_iRecordCount = 0;
		m_iFirstSequence = m_iSequence + 1;
		m_fFrom = Now();
		m_fLastFlush = m_fFrom;
		if (m_bPendingClosed)
		{
			m_bSealed = true;
			GetGame().GetCallqueue().Remove(Tick);
			Print("GC | Telemetry | " + Report());
		}
		else if (m_fBytes >= m_fByteLimit)
			Stop("overflow");
	}
	//------------------------------------------------------------------------------------------------
	void ~GC_TelemetryRecorder()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
	}
}

//! Weak entity reference prevents native-ID reuse from linking different projectiles.
class GC_TelemetryProjectile
{
	IEntity m_Entity;
	EntityID m_NativeId;
	string m_sId;
}
