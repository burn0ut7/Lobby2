//! One client-local projectile path sampled for spectator presentation.
class GC_SpectatorProjectileTrace
{
	protected static const float MIN_SAMPLE_DISTANCE_SQ = 0.01;

	protected IEntity m_Projectile;
	protected ProjectileMoveComponent m_ProjectileMove;
	protected ref array<vector> m_aPositions = {};
	protected ref array<float> m_aSampleTimes = {};
	protected float m_fInitialSpeed;
	protected float m_fCurrentSpeed;
	protected float m_fCollapseDuration;
	protected float m_fElapsedTime;
	protected float m_fCollapseElapsedTime;
	protected bool m_bFinished;

	void GC_SpectatorProjectileTrace(IEntity projectile, float collapseDuration)
	{
		m_Projectile = projectile;
		m_fCollapseDuration = collapseDuration;
		if (m_fCollapseDuration <= 0.0)
			m_fCollapseDuration = 0.01;
		if (m_Projectile)
		{
			m_ProjectileMove = ProjectileMoveComponent.Cast(m_Projectile.FindComponent(ProjectileMoveComponent));
			m_aPositions.Insert(m_Projectile.GetOrigin());
			m_aSampleTimes.Insert(0.0);
			if (m_ProjectileMove)
			{
				m_fInitialSpeed = m_ProjectileMove.GetVelocity().Length();
				m_fCurrentSpeed = m_fInitialSpeed;
			}
		}
	}

	bool IsValid()
	{
		return m_Projectile != null && m_ProjectileMove != null;
	}

	//! Records the complete path while the projectile exists. Once the projectile
	//! is destroyed, the tail advances along that path over the configured duration.
	bool Update(float timeSlice)
	{
		m_fElapsedTime += timeSlice;
		if (!m_bFinished && m_Projectile && !m_Projectile.IsDeleted())
		{
			vector position = m_Projectile.GetOrigin();
			vector previousPosition = m_aPositions[m_aPositions.Count() - 1];
			float distanceSq = vector.DistanceSq(previousPosition, position);
			m_fCurrentSpeed = m_ProjectileMove.GetVelocity().Length();
			if (m_fInitialSpeed <= 0.0 && m_fCurrentSpeed > 0.0)
				m_fInitialSpeed = m_fCurrentSpeed;
			if (distanceSq >= MIN_SAMPLE_DISTANCE_SQ)
			{
				m_aPositions.Insert(position);
				m_aSampleTimes.Insert(m_fElapsedTime);
			}
		}
		else if (!m_bFinished)
		{
			m_Projectile = null;
			m_ProjectileMove = null;
			m_fCurrentSpeed = 0.0;
			m_bFinished = true;
		}

		if (!m_bFinished)
			return true;

		m_fCollapseElapsedTime += timeSlice;
		float tailTime = GetTailTime();
		while (m_aPositions.Count() > 2 && m_aSampleTimes[1] <= tailTime)
		{
			m_aPositions.Remove(0);
			m_aSampleTimes.Remove(0);
		}

		return m_fCollapseElapsedTime < m_fCollapseDuration;
	}

	int GetSegmentCount()
	{
		return m_aPositions.Count() - 1;
	}

	vector GetPosition(int index)
	{
		return m_aPositions[index];
	}

	float GetCurrentSpeed()
	{
		return m_fCurrentSpeed;
	}

	float GetInitialSpeed()
	{
		return m_fInitialSpeed;
	}

	float GetTailFraction()
	{
		if (!m_bFinished || m_aSampleTimes.Count() < 2)
			return 0.0;

		float tailTime = GetTailTime();
		if (tailTime <= m_aSampleTimes[0])
			return 0.0;

		float segmentDuration = m_aSampleTimes[1] - m_aSampleTimes[0];
		if (segmentDuration <= 0.0)
			return 1.0;

		return Math.Clamp((tailTime - m_aSampleTimes[0]) / segmentDuration, 0.0, 1.0);
	}

	protected float GetTailTime()
	{
		if (m_aSampleTimes.IsEmpty())
			return 0.0;

		float collapseProgress = Math.Clamp(m_fCollapseElapsedTime / m_fCollapseDuration, 0.0, 1.0);
		return m_aSampleTimes[m_aSampleTimes.Count() - 1] * collapseProgress;
	}
}

//! Spectator-only projectile event adapter and CanvasWidget renderer.
//! It owns presentation state only; projectile simulation remains base-game owned.
class GC_SpectatorProjectileTracer
{
	protected static const float LINE_WIDTH = 3.0;

	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected float m_fCollapseDuration;
	protected ref map<IEntity, EventHandlerManagerComponent> m_mShooterEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref array<ref GC_SpectatorProjectileTrace> m_aTraces = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world, float collapseDuration)
	{
		if (!canvas || !workspace || !world)
			return false;

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
		m_fCollapseDuration = collapseDuration;
		if (m_fCollapseDuration <= 0.0)
			m_fCollapseDuration = 0.01;
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
		m_fCollapseDuration = 0.0;
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

		GC_SpectatorProjectileTrace trace = new GC_SpectatorProjectileTrace(projectile, m_fCollapseDuration);
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

		float tailFraction = trace.GetTailFraction();
		vector startPosition = trace.GetPosition(0);
		vector firstEndPosition = trace.GetPosition(1);
		if (tailFraction > 0.0)
			startPosition = vector.Lerp(startPosition, firstEndPosition, tailFraction);

		vector screenPosition = m_Workspace.ProjWorldToScreen(startPosition, m_World);
		if (screenPosition[2] <= 0.0)
			return;

		LineDrawCommand line = new LineDrawCommand();
		line.m_iColor = GetSpeedColor(trace.GetCurrentSpeed(), trace.GetInitialSpeed());
		line.m_fWidth = LINE_WIDTH;
		line.m_Vertices = {};
		line.m_Vertices.Insert(screenPosition[0]);
		line.m_Vertices.Insert(screenPosition[1]);

		for (int positionIndex = 1; positionIndex <= segmentCount; positionIndex++)
		{
			screenPosition = m_Workspace.ProjWorldToScreen(trace.GetPosition(positionIndex), m_World);
			if (screenPosition[2] <= 0.0)
				return;

			line.m_Vertices.Insert(screenPosition[0]);
			line.m_Vertices.Insert(screenPosition[1]);
		}

		m_aDrawCommands.Insert(line);
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
