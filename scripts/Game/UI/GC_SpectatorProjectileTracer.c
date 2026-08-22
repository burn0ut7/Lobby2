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

	void GC_SpectatorProjectileTrace(IEntity projectile, float trailDuration)
	{
		m_Projectile = projectile;
		m_fTrailDuration = trailDuration;
		if (m_fTrailDuration <= 0.0)
			m_fTrailDuration = 0.01;
		if (m_Projectile)
		{
			m_ProjectileMove = ProjectileMoveComponent.Cast(m_Projectile.FindComponent(ProjectileMoveComponent));
			m_aPositions.Insert(m_Projectile.GetOrigin());
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
	protected static const float LINE_WIDTH = 1.0;

	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected float m_fTrailDuration;
	protected ref map<IEntity, EventHandlerManagerComponent> m_mShooterEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref array<ref GC_SpectatorProjectileTrace> m_aTraces = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

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
		return true;
	}

	void Destroy()
	{
		GetGame().GetCallqueue().Remove(Update);
		foreach (IEntity shooter, EventHandlerManagerComponent eventManager : m_mShooterEvents)
		{
			if (eventManager)
				eventManager.RemoveScriptHandler("OnProjectileShot", this, OnProjectileShot, false);
		}

		m_mShooterEvents.Clear();
		m_aTraces.Clear();
		m_aDrawCommands.Clear();
		if (m_wCanvas)
			m_wCanvas.SetDrawCommands(m_aDrawCommands);

		m_wCanvas = null;
		m_Workspace = null;
		m_World = null;
		m_fTrailDuration = 0.0;
	}

	void TrackShooter(IEntity shooter)
	{
		if (!shooter || m_mShooterEvents.Contains(shooter))
			return;

		EventHandlerManagerComponent eventManager = EventHandlerManagerComponent.Cast(shooter.FindComponent(EventHandlerManagerComponent));
		if (!eventManager)
			return;

		eventManager.RegisterScriptHandler("OnProjectileShot", this, OnProjectileShot, false);
		m_mShooterEvents.Insert(shooter, eventManager);
	}

	void UntrackShooter(IEntity shooter)
	{
		EventHandlerManagerComponent eventManager;
		if (!shooter || !m_mShooterEvents.Find(shooter, eventManager))
			return;

		if (eventManager)
			eventManager.RemoveScriptHandler("OnProjectileShot", this, OnProjectileShot, false);
		m_mShooterEvents.Remove(shooter);
	}

	protected void OnProjectileShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!projectile || !m_wCanvas)
			return;

		foreach (GC_SpectatorProjectileTrace existingTrace : m_aTraces)
		{
			if (existingTrace.IsProjectile(projectile))
				return;
		}

		GC_SpectatorProjectileTrace trace = new GC_SpectatorProjectileTrace(projectile, m_fTrailDuration);
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
			if (!m_aTraces[traceIndex].Update(timeSlice))
				m_aTraces.Remove(traceIndex);
		}

		m_aDrawCommands = {};
		foreach (GC_SpectatorProjectileTrace trace : m_aTraces)
			DrawTrace(trace);
		m_wCanvas.SetDrawCommands(m_aDrawCommands);
	}

	protected void DrawTrace(GC_SpectatorProjectileTrace trace)
	{
		int segmentCount = trace.GetSegmentCount();
		if (segmentCount < 1)
			return;

		for (int positionIndex = 0; positionIndex < segmentCount; positionIndex++)
		{
			vector startScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetPosition(positionIndex), m_World);
			vector endScreenPosition = m_Workspace.ProjWorldToScreen(trace.GetPosition(positionIndex + 1), m_World);
			if (startScreenPosition[2] <= 0.0 || endScreenPosition[2] <= 0.0)
				continue;

			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = GetSpeedColor(trace.GetSpeed(positionIndex + 1), trace.GetInitialSpeed());
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

	//! Uses Reforger's default red, yellow, green, and white color anchors.
	protected int GetSpeedColor(float speed, float initialSpeed)
	{
		if (initialSpeed <= 0.0)
			return Color.WHITE;

		float normalizedSpeed = Math.Clamp(speed / initialSpeed, 0.0, 1.0);
		int startColor;
		int endColor;
		float factor;
		if (normalizedSpeed >= 0.666667)
		{
			startColor = Color.YELLOW;
			endColor = Color.RED;
			factor = (normalizedSpeed - 0.666667) * 3.0;
		}
		else if (normalizedSpeed >= 0.333333)
		{
			startColor = Color.GREEN;
			endColor = Color.YELLOW;
			factor = (normalizedSpeed - 0.333333) * 3.0;
		}
		else
		{
			startColor = Color.WHITE;
			endColor = Color.GREEN;
			factor = normalizedSpeed * 3.0;
		}

		Color color = Color.FromInt(startColor);
		color.Lerp(Color.FromInt(endColor), factor);
		return color.PackToInt();
	}
}
