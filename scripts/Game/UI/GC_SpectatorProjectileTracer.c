//! One client-local projectile path sampled for spectator presentation.
class GC_SpectatorProjectileTrace
{
	protected static const float MIN_SAMPLE_DISTANCE_SQ = 0.01;
	protected static const float COLLAPSE_DURATION = 1.0;
	protected static const int MAX_SAMPLES = 128;

	protected IEntity m_Projectile;
	protected ref array<vector> m_aPositions = {};
	protected ref array<float> m_aSpeeds = {};
	protected float m_fInitialSpeed;
	protected float m_fCollapseProgress;
	protected bool m_bCollapsing;

	void GC_SpectatorProjectileTrace(IEntity projectile)
	{
		m_Projectile = projectile;
		if (m_Projectile)
		{
			m_aPositions.Insert(m_Projectile.GetOrigin());
			m_aSpeeds.Insert(0.0);
		}
	}

	//! Returns false once the collapsing tail reaches the final sample.
	bool Update(float timeSlice)
	{
		if (!m_bCollapsing && m_Projectile && !m_Projectile.IsDeleted())
		{
			vector position = m_Projectile.GetOrigin();
			vector previousPosition = m_aPositions[m_aPositions.Count() - 1];
			float distanceSq = vector.DistanceSq(previousPosition, position);
			if (distanceSq >= MIN_SAMPLE_DISTANCE_SQ)
			{
				float speed;
				if (timeSlice > 0.0)
					speed = Math.Sqrt(distanceSq) / timeSlice;

				if (m_fInitialSpeed <= 0.0 && speed > 0.0)
				{
					m_fInitialSpeed = speed;
					m_aSpeeds[0] = speed;
				}

				m_aPositions.Insert(position);
				m_aSpeeds.Insert(speed);
				if (m_aPositions.Count() > MAX_SAMPLES)
				{
					// Preserve the true start and endpoint while bounding long-flight memory.
					m_aPositions.Remove(1);
					m_aSpeeds.Remove(1);
				}
			}

			return true;
		}

		m_Projectile = null;
		m_bCollapsing = true;
		if (m_aPositions.Count() < 2)
			return false;

		m_fCollapseProgress += timeSlice / COLLAPSE_DURATION;
		return m_fCollapseProgress < 1.0;
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

	float GetCollapseProgress()
	{
		return Math.Clamp(m_fCollapseProgress, 0.0, 1.0);
	}
}

//! Spectator-only projectile event adapter and CanvasWidget renderer.
//! It owns presentation state only; projectile simulation remains base-game owned.
class GC_SpectatorProjectileTracer
{
	protected static const int MAX_ACTIVE_TRACES = 64;
	protected static const float LINE_WIDTH = 3.0;
	protected static const float LINE_OUTLINE_WIDTH = 1.0;

	protected CanvasWidget m_wCanvas;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected ref map<IEntity, EventHandlerManagerComponent> m_mShooterEvents = new map<IEntity, EventHandlerManagerComponent>();
	protected ref array<ref GC_SpectatorProjectileTrace> m_aTraces = {};
	protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

	bool Start(CanvasWidget canvas, WorkspaceWidget workspace, BaseWorld world)
	{
		if (!canvas || !workspace || !world)
			return false;

		m_wCanvas = canvas;
		m_Workspace = workspace;
		m_World = world;
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

		if (m_aTraces.Count() >= MAX_ACTIVE_TRACES)
			m_aTraces.Remove(0);
		m_aTraces.Insert(new GC_SpectatorProjectileTrace(projectile));
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

		float collapsedSegments = trace.GetCollapseProgress() * segmentCount;
		int firstSegment = Math.Floor(collapsedSegments);
		float firstSegmentFraction = collapsedSegments - firstSegment;
		if (firstSegment >= segmentCount)
			return;

		for (int segmentIndex = firstSegment; segmentIndex < segmentCount; segmentIndex++)
		{
			vector startPosition = trace.GetPosition(segmentIndex);
			vector endPosition = trace.GetPosition(segmentIndex + 1);
			if (segmentIndex == firstSegment && firstSegmentFraction > 0.0)
				startPosition = vector.Lerp(startPosition, endPosition, firstSegmentFraction);

			vector screenStart = m_Workspace.ProjWorldToScreen(startPosition, m_World);
			vector screenEnd = m_Workspace.ProjWorldToScreen(endPosition, m_World);
			if (screenStart[2] <= 0.0 || screenEnd[2] <= 0.0)
				continue;

			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = GetSpeedColor(trace.GetSpeed(segmentIndex + 1), trace.GetInitialSpeed());
			line.m_fWidth = LINE_WIDTH;
			line.m_fOutlineWidth = LINE_OUTLINE_WIDTH;
			line.m_iOutlineColor = 0x99000000;
			line.m_Vertices = {};
			line.m_Vertices.Insert(screenStart[0]);
			line.m_Vertices.Insert(screenStart[1]);
			line.m_Vertices.Insert(screenEnd[0]);
			line.m_Vertices.Insert(screenEnd[1]);
			m_aDrawCommands.Insert(line);
		}
	}

	//! Initial speed is red, then green, yellow, and finally white at rest.
	protected int GetSpeedColor(float speed, float initialSpeed)
	{
		if (initialSpeed <= 0.0)
			return Color.WHITE;

		float normalizedSpeed = Math.Clamp(speed / initialSpeed, 0.0, 1.0);
		float red;
		float green;
		float blue;
		if (normalizedSpeed >= 0.666667)
		{
			float factor = (normalizedSpeed - 0.666667) * 3.0;
			red = factor;
			green = 1.0 - factor;
		}
		else if (normalizedSpeed >= 0.333333)
		{
			red = (0.666667 - normalizedSpeed) * 3.0;
			green = 1.0;
		}
		else
		{
			float factor = normalizedSpeed * 3.0;
			red = 1.0;
			green = 1.0;
			blue = 1.0 - factor;
		}

		Color color = new Color(red, green, blue, 1.0);
		return color.PackToInt();
	}
}
