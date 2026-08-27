//! One client-local projectile trace created by a shot event.
class GC_SpectatorProjectileTrace
{
	protected IEntity m_Projectile;
	protected ProjectileMoveComponent m_ProjectileMove;
	protected vector m_vSpawnPosition;
	protected vector m_vStartPosition;
	protected vector m_vEndPosition;
	protected float m_fInitialSpeed;
	protected float m_fCurrentSpeed;
	protected float m_fTrailDuration;
	protected float m_fFinishedElapsedTime;
	protected bool m_bHasSegment;
	protected bool m_bFinished;

	void GC_SpectatorProjectileTrace(IEntity projectile, vector startPosition, float trailDuration)
	{
		m_Projectile = projectile;
		m_vSpawnPosition = startPosition;
		m_vStartPosition = startPosition;
		m_vEndPosition = startPosition;
		m_fTrailDuration = trailDuration;
		if (m_Projectile)
		{
			m_ProjectileMove = ProjectileMoveComponent.Cast(m_Projectile.FindComponent(ProjectileMoveComponent));
			if (m_ProjectileMove)
				m_fInitialSpeed = m_ProjectileMove.GetVelocity().Length();
			m_fCurrentSpeed = m_fInitialSpeed;
		}
	}

	bool IsValid()
	{
		return m_Projectile != null && m_ProjectileMove != null;
	}

	bool IsProjectile(IEntity projectile)
	{
		return m_Projectile && m_Projectile == projectile;
	}

	//! A shot event owns one trace. While its projectile exists, derive one
	//! bounded segment directly from live position and velocity; retain only the
	//! final segment for the configured presentation duration after deletion.
	bool Update(float timeSlice)
	{
		if (!m_bFinished && m_Projectile && !m_Projectile.IsDeleted())
		{
			m_vEndPosition = m_Projectile.GetOrigin();
			vector velocity = m_ProjectileMove.GetVelocity();
			m_fCurrentSpeed = velocity.Length();
			if (m_fInitialSpeed <= 0.0 && m_fCurrentSpeed > 0.0)
				m_fInitialSpeed = m_fCurrentSpeed;

			if (m_fCurrentSpeed > 0.001)
			{
				vector direction = velocity;
				direction.Normalize();
				float trailLength = m_fCurrentSpeed * m_fTrailDuration;
				float traveledDistance = vector.Distance(m_vSpawnPosition, m_vEndPosition);
				if (traveledDistance <= trailLength)
					m_vStartPosition = m_vSpawnPosition;
				else
					m_vStartPosition = m_vEndPosition - direction * trailLength;

				m_bHasSegment = vector.DistanceSq(m_vStartPosition, m_vEndPosition) > 0.0001;
			}
			return true;
		}

		if (!m_bFinished)
		{
			m_Projectile = null;
			m_ProjectileMove = null;
			m_bFinished = true;
		}

		m_fFinishedElapsedTime += timeSlice;
		return m_bHasSegment && m_fFinishedElapsedTime <= m_fTrailDuration;
	}

	bool HasSegment()
	{
		return m_bHasSegment;
	}

	bool IsFinished()
	{
		return m_bFinished;
	}

	vector GetStartPosition()
	{
		return m_vStartPosition;
	}

	vector GetEndPosition()
	{
		return m_vEndPosition;
	}

	float GetCurrentSpeed()
	{
		return m_fCurrentSpeed;
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
	protected static const int MAX_RETAINED_TRACES = 64;

	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected float m_fTrailDuration;
	protected bool m_bEnabled;
	protected ref map<IEntity, EventHandlerManagerComponent> m_mGrenadeEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref map<IEntity, EventHandlerManagerComponent> m_mProjectileEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref map<IEntity, SCR_MuzzleEffectComponent> m_mMuzzleEffects = new map<IEntity, SCR_MuzzleEffectComponent>();
	protected ref map<IEntity, int> m_mShooterReferences = new map<IEntity, int>();
	protected ref array<ref GC_SpectatorProjectileTrace> m_aTraces = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world, float trailDuration, bool enabled)
	{
		if (!canvas || !workspace || !world)
			return false;

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_fTrailDuration = trailDuration;
		if (m_fTrailDuration <= 0.0)
			m_fTrailDuration = 0.01;
		SetEnabled(enabled);
		return true;
	}

	void SetEnabled(bool enabled)
	{
		if (m_bEnabled == enabled)
			return;

		m_bEnabled = enabled;
		GetGame().GetCallqueue().Remove(Update);
		if (m_bEnabled)
		{
			GetGame().GetCallqueue().CallLater(Update, 0, true);
			return;
		}

		m_aTraces.Clear();
		m_aDrawCommands = {};
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);
		ClearShooters();
	}

	bool IsEnabled()
	{
		return m_bEnabled;
	}

	void Destroy()
	{
		GetGame().GetCallqueue().Remove(Update);
		ClearShooters();
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
		if (!m_bEnabled || !shooter)
			return false;

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
			return false;

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
	}

	protected void OnMuzzleWeaponFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectile)
	{
		CreateProjectileTrace(projectile);
	}

	protected void OnWeaponProjectileCreated(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		CreateProjectileTrace(projectile);
	}

	protected void OnGrenadeCreated(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		CreateProjectileTrace(projectile);
	}

	protected void CreateProjectileTrace(IEntity projectile)
	{
		if (!m_bEnabled || !projectile || !m_wCanvas)
			return;

		foreach (GC_SpectatorProjectileTrace existingTrace : m_aTraces)
		{
			if (existingTrace.IsProjectile(projectile))
				return;
		}

		if (m_aTraces.Count() >= MAX_RETAINED_TRACES)
		{
			for (int traceIndex; traceIndex < m_aTraces.Count(); traceIndex++)
			{
				if (!m_aTraces[traceIndex].IsFinished())
					continue;

				m_aTraces.Remove(traceIndex);
				break;
			}
		}

		GC_SpectatorProjectileTrace trace = new GC_SpectatorProjectileTrace(projectile, projectile.GetOrigin(), m_fTrailDuration);
		if (trace.IsValid())
			m_aTraces.Insert(trace);
	}

	protected void Update()
	{
		if (!m_wCanvas || !m_Workspace || !m_World)
			return;

		float timeSlice = m_World.GetTimeSlice();
		for (int traceIndex = m_aTraces.Count() - 1; traceIndex >= 0; traceIndex--)
		{
			GC_SpectatorProjectileTrace trace = m_aTraces[traceIndex];
			bool keepTrace = trace.Update(timeSlice);
			if (!keepTrace)
				m_aTraces.Remove(traceIndex);
		}

		m_aDrawCommands.Clear();
		foreach (GC_SpectatorProjectileTrace trace : m_aTraces)
			DrawTrace(trace);
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected void ClearShooters()
	{
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
	}

	protected void DrawTrace(GC_SpectatorProjectileTrace trace)
	{
		if (!trace.HasSegment())
			return;

		vector startScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetStartPosition(), m_World);
		startScreenPosition[0] = m_Workspace.DPIUnscale(startScreenPosition[0]);
		startScreenPosition[1] = m_Workspace.DPIUnscale(startScreenPosition[1]);
		vector endScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetEndPosition(), m_World);
		endScreenPosition[0] = m_Workspace.DPIUnscale(endScreenPosition[0]);
		endScreenPosition[1] = m_Workspace.DPIUnscale(endScreenPosition[1]);
		if (startScreenPosition[2] > 0.0 && endScreenPosition[2] > 0.0)
		{
			float screenDeltaX = endScreenPosition[0] - startScreenPosition[0];
			float screenDeltaY = endScreenPosition[1] - startScreenPosition[1];
			if (screenDeltaX * screenDeltaX + screenDeltaY * screenDeltaY > MIN_DRAWABLE_SEGMENT_LENGTH_SQ)
			{
				LineDrawCommand line = new LineDrawCommand();
				line.m_iColor = GetSpeedColor(trace.GetCurrentSpeed(), trace.GetInitialSpeed());
				line.m_fWidth = LINE_WIDTH;
				line.m_bShouldEnclose = false;
				line.m_Vertices = {startScreenPosition[0], startScreenPosition[1], endScreenPosition[0], endScreenPosition[1]};
				m_aDrawCommands.Insert(line);
			}
		}
	}

	//! Projects the same retained world-space traces used by the 3D overlay onto
	//! the spectator map. Capture, lifetime, speed colors, and enable state remain
	//! owned by this single tracer.
	void AppendMapDrawCommands(SCR_MapEntity mapEntity, array<ref CanvasWidgetCommand> drawCommands)
	{
		if (!m_bEnabled || !mapEntity || !mapEntity.IsOpen() || !drawCommands)
			return;

		foreach (GC_SpectatorProjectileTrace trace : m_aTraces)
		{
			if (!trace.HasSegment())
				continue;

			vector startPosition = trace.GetStartPosition();
			vector endPosition = trace.GetEndPosition();
			int startScreenX;
			int startScreenY;
			mapEntity.WorldToScreen(startPosition[0], startPosition[2], startScreenX, startScreenY, true);
			int endScreenX;
			int endScreenY;
			mapEntity.WorldToScreen(endPosition[0], endPosition[2], endScreenX, endScreenY, true);
			float screenDeltaX = endScreenX - startScreenX;
			float screenDeltaY = endScreenY - startScreenY;
			if (screenDeltaX * screenDeltaX + screenDeltaY * screenDeltaY > MIN_DRAWABLE_SEGMENT_LENGTH_SQ)
			{
				LineDrawCommand line = new LineDrawCommand();
				line.m_iColor = GetSpeedColor(trace.GetCurrentSpeed(), trace.GetInitialSpeed());
				line.m_fWidth = LINE_WIDTH;
				line.m_bShouldEnclose = false;
				line.m_Vertices = {startScreenX, startScreenY, endScreenX, endScreenY};
				drawCommands.Insert(line);
			}
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
