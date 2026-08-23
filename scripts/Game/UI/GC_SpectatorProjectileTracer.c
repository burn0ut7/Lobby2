//! One client-local projectile path sampled for spectator presentation.
class GC_SpectatorProjectileTrace
{
	protected IEntity m_Projectile;
	protected ProjectileMoveComponent m_ProjectileMove;
	protected ref array<vector> m_aPositions = {};
	protected ref array<float> m_aSampleTimes = {};
	protected ref array<float> m_aSpeeds = {};
	protected float m_fInitialSpeed;
	protected float m_fTrailDuration;
	protected float m_fElapsedTime;
	protected bool m_bFinished;

	void GC_SpectatorProjectileTrace(IEntity projectile, vector startPosition, float trailDuration)
	{
		m_Projectile = projectile;
		m_fTrailDuration = trailDuration;
		if (m_fTrailDuration <= 0.0)
			m_fTrailDuration = 0.01;
		if (m_Projectile)
		{
			m_ProjectileMove = ProjectileMoveComponent.Cast(m_Projectile.FindComponent(ProjectileMoveComponent));
			m_aPositions.Insert(startPosition);
			m_aSampleTimes.Insert(0.0);
			if (m_ProjectileMove)
				m_fInitialSpeed = m_ProjectileMove.GetVelocity().Length();
			m_aSpeeds.Insert(m_fInitialSpeed);
		}
	}

	bool IsValid()
	{
		return m_Projectile != null && m_ProjectileMove != null;
	}

	bool IsProjectile(IEntity projectile)
	{
		return m_Projectile == projectile;
	}

	//! Samples one point every presentation frame. After the configured delay,
	//! one oldest point is removed per frame so the tail follows the projectile.
	bool Update(float timeSlice)
	{
		m_fElapsedTime += timeSlice;
		if (!m_bFinished && m_Projectile && !m_Projectile.IsDeleted())
		{
			float speed = m_ProjectileMove.GetVelocity().Length();
			if (m_fInitialSpeed <= 0.0 && speed > 0.0)
				m_fInitialSpeed = speed;

			m_aPositions.Insert(m_Projectile.GetOrigin());
			m_aSampleTimes.Insert(m_fElapsedTime);
			m_aSpeeds.Insert(speed);
		}
		else if (!m_bFinished)
		{
			m_Projectile = null;
			m_ProjectileMove = null;
			m_bFinished = true;
		}

		int minimumPointCount = 2;
		if (m_bFinished)
			minimumPointCount = 1;

		float tailTime = m_fElapsedTime - m_fTrailDuration;
		if (m_aPositions.Count() > minimumPointCount && m_aSampleTimes[0] <= tailTime)
		{
			m_aPositions.RemoveOrdered(0);
			m_aSampleTimes.RemoveOrdered(0);
			m_aSpeeds.RemoveOrdered(0);
		}

		return !m_bFinished || m_aPositions.Count() > 1;
	}

	int GetSegmentCount()
	{
		return m_aPositions.Count() - 1;
	}

	vector GetPosition(int index)
	{
		return m_aPositions[index];
	}

	float GetSpeed(int index)
	{
		return m_aSpeeds[index];
	}

	float GetInitialSpeed()
	{
		return m_fInitialSpeed;
	}

}

//! Spectator-only projectile event adapter and CanvasWidget renderer.
//! It owns presentation state only; projectile simulation remains base-game owned.
class GC_SpectatorProjectileTracer
{
	protected static const float LINE_WIDTH = 1.5;
	protected static const float MIN_DRAWABLE_SEGMENT_LENGTH_SQ = 1.0;
	protected static const int SHOT_DETAIL_LIMIT = 12;

	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected float m_fTrailDuration;
	protected ref map<IEntity, EventHandlerManagerComponent> m_mGrenadeEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref map<IEntity, EventHandlerManagerComponent> m_mProjectileEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref map<IEntity, SCR_MuzzleEffectComponent> m_mMuzzleEffects = new map<IEntity, SCR_MuzzleEffectComponent>();
	protected ref map<IEntity, int> m_mShooterReferences = new map<IEntity, int>();
	protected ref set<IEntity> m_sReportedMissingFiringSources = new set<IEntity>();
	protected ref array<ref GC_SpectatorProjectileTrace> m_aTraces = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};
	protected int m_iTrackAttempts;
	protected int m_iTrackSuccesses;
	protected int m_iTrackFailures;
	protected int m_iUntracks;
	protected int m_iProjectileCallbacks;
	protected int m_iMuzzleCallbacks;
	protected int m_iAcceptedTraces;
	protected int m_iRejectedCallbacks;
	protected int m_iLastSegmentCount;
	protected int m_iLastProjectedSegmentCount;
	protected int m_iLastDrawCommandCount;

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world, float trailDuration)
	{
		if (!canvas || !workspace || !world)
			return false;

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_fTrailDuration = trailDuration;
		if (m_fTrailDuration <= 0.0)
			m_fTrailDuration = 0.01;
		GetGame().GetCallqueue().CallLater(Update, 0, true);
		PrintFormat("GC | Tracer Debug | renderer started: canvas=%1 workspace=%2 world=%3 trailDuration=%4", m_wCanvas, m_Workspace, m_World, m_fTrailDuration);
		return true;
	}

	void Destroy()
	{
		ReportDiagnostics("destroy");
		GetGame().GetCallqueue().Remove(Update);
		foreach (IEntity shooter, EventHandlerManagerComponent eventManager : m_mGrenadeEvents)
		{
			if (eventManager)
				eventManager.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeCreated, false);
		}
		foreach (IEntity shooter, EventHandlerManagerComponent eventManager : m_mProjectileEvents)
		{
			if (eventManager)
				eventManager.RemoveScriptHandler("OnProjectileShot", this, OnWeaponProjectileCreated, false);
		}
		foreach (IEntity shooter, SCR_MuzzleEffectComponent muzzleEffect : m_mMuzzleEffects)
		{
			if (muzzleEffect)
				muzzleEffect.GetOnWeaponFired().Remove(OnMuzzleWeaponFired);
		}

		m_mGrenadeEvents.Clear();
		m_mProjectileEvents.Clear();
		m_mMuzzleEffects.Clear();
		m_mShooterReferences.Clear();
		m_sReportedMissingFiringSources.Clear();
		m_aTraces.Clear();
		m_aDrawCommands.Clear();
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);

		m_wCanvas = null;
		m_Workspace = null;
		m_World = null;
		m_fTrailDuration = 0.0;
	}

	bool TrackShooter(IEntity shooter, bool trackGrenades, bool trackProjectileEvents)
	{
		m_iTrackAttempts++;
		if (!shooter)
		{
			m_iTrackFailures++;
			Print("GC | Tracer Debug | subscription rejected: shooter is null", LogLevel.WARNING);
			return false;
		}

		int referenceCount;
		if (m_mShooterReferences.Find(shooter, referenceCount))
		{
			m_mShooterReferences.Set(shooter, referenceCount + 1);
			return true;
		}

		EventHandlerManagerComponent eventManager = EventHandlerManagerComponent.Cast(shooter.FindComponent(EventHandlerManagerComponent));
		SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(shooter.FindComponent(SCR_MuzzleEffectComponent));
		bool useGrenadeEvent = trackGrenades && eventManager;
		bool useProjectileEvent = trackProjectileEvents && !muzzleEffect && eventManager;
		if (!useGrenadeEvent && !useProjectileEvent && !muzzleEffect)
		{
			m_iTrackFailures++;
			if (!m_sReportedMissingFiringSources.Contains(shooter))
			{
				m_sReportedMissingFiringSources.Insert(shooter);
				PrintFormat("GC | Tracer Debug | subscription rejected: shooter=%1 reason=no supported firing event source", shooter, level: LogLevel.WARNING);
			}
			return false;
		}

		if (useGrenadeEvent)
		{
			eventManager.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeCreated, false);
			m_mGrenadeEvents.Insert(shooter, eventManager);
		}
		if (useProjectileEvent)
		{
			eventManager.RegisterScriptHandler("OnProjectileShot", this, OnWeaponProjectileCreated, false);
			m_mProjectileEvents.Insert(shooter, eventManager);
		}
		if (muzzleEffect)
		{
			muzzleEffect.GetOnWeaponFired().Insert(OnMuzzleWeaponFired);
			m_mMuzzleEffects.Insert(shooter, muzzleEffect);
		}
		m_mShooterReferences.Insert(shooter, 1);
		m_iTrackSuccesses++;
		PrintFormat("GC | Tracer Debug | subscription active: shooter=%1 grenadeEvent=%2 projectileEvent=%3 muzzleEffect=%4 tracked=%5", shooter, useGrenadeEvent, useProjectileEvent, muzzleEffect, m_mShooterReferences.Count());
		return true;
	}

	void UntrackShooter(IEntity shooter)
	{
		int referenceCount;
		if (!shooter || !m_mShooterReferences.Find(shooter, referenceCount))
			return;

		if (referenceCount > 1)
		{
			m_mShooterReferences.Set(shooter, referenceCount - 1);
			return;
		}

		EventHandlerManagerComponent eventManager;
		if (m_mGrenadeEvents.Find(shooter, eventManager) && eventManager)
			eventManager.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeCreated, false);
		if (m_mProjectileEvents.Find(shooter, eventManager) && eventManager)
			eventManager.RemoveScriptHandler("OnProjectileShot", this, OnWeaponProjectileCreated, false);
		SCR_MuzzleEffectComponent muzzleEffect;
		if (m_mMuzzleEffects.Find(shooter, muzzleEffect) && muzzleEffect)
			muzzleEffect.GetOnWeaponFired().Remove(OnMuzzleWeaponFired);
		m_mGrenadeEvents.Remove(shooter);
		m_mProjectileEvents.Remove(shooter);
		m_mMuzzleEffects.Remove(shooter);
		m_mShooterReferences.Remove(shooter);
		m_iUntracks++;
		PrintFormat("GC | Tracer Debug | subscription removed: shooter=%1 tracked=%2", shooter, m_mShooterReferences.Count());
	}

	protected void OnMuzzleWeaponFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectile)
	{
		m_iMuzzleCallbacks++;
		if (!effectEntity)
		{
			m_iRejectedCallbacks++;
			PrintFormat("GC | Tracer Debug | muzzle callback rejected: callback=%1 muzzle=%2 projectile=%3 reason=no muzzle effect entity", m_iMuzzleCallbacks, muzzle, projectile, level: LogLevel.WARNING);
			return;
		}

		CreateProjectileTrace(0, null, projectile, effectEntity.GetOrigin(), "muzzle");
	}

	protected void OnWeaponProjectileCreated(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		IEntity muzzleParticle;
		if (weapon)
		{
			IEntity weaponEntity = weapon.GetOwner();
			MuzzleEffectComponent muzzleEffect;
			if (weaponEntity)
				muzzleEffect = MuzzleEffectComponent.Cast(weaponEntity.FindComponent(MuzzleEffectComponent));
			if (muzzleEffect)
				muzzleParticle = muzzleEffect.GetParticleEntity();
		}

		if (!muzzleParticle)
		{
			m_iRejectedCallbacks++;
			PrintFormat("GC | Tracer Debug | vehicle projectile callback rejected: player=%1 weapon=%2 projectile=%3 reason=no muzzle particle entity", playerId, weapon, projectile, level: LogLevel.WARNING);
			return;
		}

		CreateProjectileTrace(playerId, weapon, projectile, muzzleParticle.GetOrigin(), "vehicle-muzzle");
	}

	protected void OnGrenadeCreated(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		vector startPosition;
		if (projectile)
			startPosition = projectile.GetOrigin();
		CreateProjectileTrace(playerId, weapon, projectile, startPosition, "grenade");
	}

	protected void CreateProjectileTrace(int playerId, BaseWeaponComponent weapon, IEntity projectile, vector startPosition, string originSource)
	{
		m_iProjectileCallbacks++;
		if (!projectile || !m_wCanvas)
		{
			m_iRejectedCallbacks++;
			if (m_iProjectileCallbacks <= SHOT_DETAIL_LIMIT)
				PrintFormat("GC | Tracer Debug | projectile callback rejected: callback=%1 player=%2 weapon=%3 projectile=%4 canvas=%5", m_iProjectileCallbacks, playerId, weapon, projectile, m_wCanvas, level: LogLevel.WARNING);
			return;
		}

		foreach (GC_SpectatorProjectileTrace existingTrace : m_aTraces)
		{
			if (existingTrace.IsProjectile(projectile))
			{
				m_iRejectedCallbacks++;
				if (m_iProjectileCallbacks <= SHOT_DETAIL_LIMIT)
					PrintFormat("GC | Tracer Debug | projectile callback duplicate: callback=%1 player=%2 weapon=%3 projectile=%4", m_iProjectileCallbacks, playerId, weapon, projectile);
				return;
			}
		}

		GC_SpectatorProjectileTrace trace = new GC_SpectatorProjectileTrace(projectile, startPosition, m_fTrailDuration);
		if (trace.IsValid())
		{
			m_aTraces.Insert(trace);
			m_iAcceptedTraces++;
		}
		else
		{
			m_iRejectedCallbacks++;
		}

		if (m_iProjectileCallbacks <= SHOT_DETAIL_LIMIT)
			PrintFormat("GC | Tracer Debug | projectile callback: callback=%1 player=%2 weapon=%3 projectile=%4 originSource=%5 start=%6 projectilePosition=%7 traceValid=%8 activeTraces=%9", m_iProjectileCallbacks, playerId, weapon, projectile, originSource, startPosition, projectile.GetOrigin(), trace.IsValid(), m_aTraces.Count());
	}

	int GetTrackedShooterCount()
	{
		return m_mShooterReferences.Count();
	}

	void ReportDiagnostics(string source)
	{
		PrintFormat("GC | Tracer Debug | subscription report: source=%1 tracked=%2 grenadeSources=%3 projectileEventSources=%4 muzzleSources=%5 attempts=%6 successes=%7 failures=%8 untracks=%9", source, m_mShooterReferences.Count(), m_mGrenadeEvents.Count(), m_mProjectileEvents.Count(), m_mMuzzleEffects.Count(), m_iTrackAttempts, m_iTrackSuccesses, m_iTrackFailures, m_iUntracks);
		PrintFormat("GC | Tracer Debug | rendering report: callbacks=%1 muzzleCallbacks=%2 accepted=%3 rejected=%4 activeTraces=%5 segments=%6 projectedSegments=%7 drawCommands=%8", m_iProjectileCallbacks, m_iMuzzleCallbacks, m_iAcceptedTraces, m_iRejectedCallbacks, m_aTraces.Count(), m_iLastSegmentCount, m_iLastProjectedSegmentCount, m_iLastDrawCommandCount);
	}

	protected void Update()
	{
		if (!m_wCanvas || !m_Workspace || !m_World)
			return;

		float timeSlice = m_World.GetTimeSlice();
		for (int traceIndex = m_aTraces.Count() - 1; traceIndex >= 0; traceIndex--)
		{
			if (!m_aTraces[traceIndex].Update(timeSlice))
				m_aTraces.Remove(traceIndex);
		}

		m_aDrawCommands = {};
		m_iLastSegmentCount = 0;
		m_iLastProjectedSegmentCount = 0;
		foreach (GC_SpectatorProjectileTrace trace : m_aTraces)
			DrawTrace(trace);
		m_iLastDrawCommandCount = m_aDrawCommands.Count();
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected void DrawTrace(GC_SpectatorProjectileTrace trace)
	{
		int segmentCount = trace.GetSegmentCount();
		if (segmentCount < 1)
			return;

		for (int positionIndex = 0; positionIndex < segmentCount; positionIndex++)
		{
			m_iLastSegmentCount++;
			vector startScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetPosition(positionIndex), m_World);
			vector endScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetPosition(positionIndex + 1), m_World);
			if (startScreenPosition[2] <= 0.0 || endScreenPosition[2] <= 0.0)
				continue;

			float screenDeltaX = endScreenPosition[0] - startScreenPosition[0];
			float screenDeltaY = endScreenPosition[1] - startScreenPosition[1];
			if (screenDeltaX * screenDeltaX + screenDeltaY * screenDeltaY <= MIN_DRAWABLE_SEGMENT_LENGTH_SQ)
				continue;
			m_iLastProjectedSegmentCount++;

			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = GetSpeedColor(trace.GetSpeed(positionIndex), trace.GetInitialSpeed());
			line.m_fWidth = LINE_WIDTH;
			line.m_bShouldEnclose = false;
			line.m_Vertices = {};
			line.m_Vertices.Insert(startScreenPosition[0]);
			line.m_Vertices.Insert(startScreenPosition[1]);
			line.m_Vertices.Insert(endScreenPosition[0]);
			line.m_Vertices.Insert(endScreenPosition[1]);
			m_aDrawCommands.Insert(line);
		}
	}

	//! Holds red through 90% speed, blends to green by 10%, then to white at rest.
	protected int GetSpeedColor(float speed, float initialSpeed)
	{
		if (initialSpeed <= 0.0)
			return Color.WHITE;

		float speedRatio = Math.Clamp(speed / initialSpeed, 0.0, 1.0);
		if (speedRatio >= 0.9)
			return Color.RED;

		Color color;
		if (speedRatio >= 0.1)
		{
			color = Color.FromInt(Color.RED);
			color.Lerp(Color.FromInt(Color.GREEN), (0.9 - speedRatio) / 0.8);
		}
		else
		{
			color = Color.FromInt(Color.GREEN);
			color.Lerp(Color.FromInt(Color.WHITE), (0.1 - speedRatio) / 0.1);
		}

		return color.PackToInt();
	}
}
