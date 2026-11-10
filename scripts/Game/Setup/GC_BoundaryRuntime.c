//! Clients discover spatial changes. Authority validates only the requesting body,
//! native vehicle occupants, active enforcement cases and a bounded audit slice.
class GC_BoundaryRuntime
{
	static const float APPROACH_METRES = 75;
	protected GC_LobbyGameMode m_Mode;
	protected ref array<GC_SetupZoneEntity> m_aSetups = {};
	protected ref array<GC_BoundaryWallComponent> m_aWalls = {};
	protected ref array<int> m_aActive = {};
	protected ref array<int> m_aAudit = {};
	protected int m_iAuditCursor;
	protected float m_fAuditCycleStarted;
	protected ref array<int> m_aCounts = {};
	protected ref array<vector> m_aPoints = {};
	protected bool m_bStopped;
	protected bool m_bActiveScheduled;
	protected int m_iRevision;

	void GC_BoundaryRuntime(GC_LobbyGameMode mode)
	{
		m_Mode = mode;
	}

	//------------------------------------------------------------------------------------------------
	static int ProximityBand(array<vector> polygon, vector position)
	{
		position[1] = 0;
		float nearest = APPROACH_METRES * APPROACH_METRES + 1;
		for (int i; i < polygon.Count(); i++)
		{
			vector first = polygon[i];
			vector second = polygon[(i + 1) % polygon.Count()];
			first[1] = 0;
			second[1] = 0;
			nearest = Math.Min(nearest, Math3D.PointLineSegmentDistanceSqr(position, first, second));
		}
		if (nearest <= 9)
			return 3;
		if (nearest <= 225)
			return 2;
		if (nearest <= APPROACH_METRES * APPROACH_METRES)
			return 1;
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	void RegisterSetup(GC_SetupZoneEntity zone)
	{
		if (!m_aSetups.Contains(zone))
			m_aSetups.Insert(zone);
		Changed();
	}

	void UnregisterSetup(GC_SetupZoneEntity zone)
	{
		m_aSetups.RemoveItem(zone);
		Changed();
	}

	//------------------------------------------------------------------------------------------------
	//! Walls register presentation only; setup and AO retain their enforcement policy.
	void RegisterWall(GC_BoundaryWallComponent wall)
	{
		if (m_bStopped || m_aWalls.Contains(wall))
			return;
		m_aWalls.Insert(wall);
		wall.RefreshAuthority();
	}

	void UnregisterWall(GC_BoundaryWallComponent wall)
	{
		m_aWalls.RemoveItem(wall);
	}

	//------------------------------------------------------------------------------------------------
	//! Coalesce authored mutations and phase changes, not recurring polling.
	void Changed()
	{
		if (m_bStopped)
			return;
		GetGame().GetCallqueue().Remove(Rebuild);
		GetGame().GetCallqueue().CallLater(Rebuild, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsActive()
	{
		return !m_bStopped && Replication.IsServer() && m_Mode == GC_LobbyGameMode.GetGameMode()
			&& m_Mode.GetState() == SCR_EGameModeState.GAME;
	}

	//------------------------------------------------------------------------------------------------
	protected void Rebuild()
	{
		if (m_bStopped || m_Mode != GC_LobbyGameMode.GetGameMode())
			return;
		GetGame().GetCallqueue().Remove(Audit);
		GetGame().GetCallqueue().Remove(ObserveActive);
		m_bActiveScheduled = false;
		m_aActive.Clear();
		m_aAudit.Clear();
		m_iAuditCursor = 0;
		m_aCounts.Clear();
		m_aPoints.Clear();
		m_iRevision++;
		m_Mode.GetAOLimits().OnPhaseChanged();
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		// Enrollment must be captured at match entry, before players can leave setup.
		foreach (GC_SetupZoneEntity zone : m_aSetups)
		{
			zone.EvaluatePlayers(players);
			if (!IsActive() || !zone.IsEvaluating())
				continue;
			array<vector> polygon = {};
			zone.GetBoundaryPolygon(polygon);
			m_aCounts.Insert(polygon.Count());
			m_aPoints.InsertAll(polygon);
		}
		if (IsActive())
			m_Mode.GetAOLimits().GetDetectionPolygons(m_aCounts, m_aPoints);
		foreach (int playerId : players)
		{
			SendSnapshot(playerId);
			if (!m_aCounts.IsEmpty())
				Observe(playerId);
		}
		foreach (GC_BoundaryWallComponent wall : m_aWalls)
			wall.RefreshAuthority();
		if (!m_aCounts.IsEmpty())
			GetGame().GetCallqueue().CallLater(Audit, 250);
	}

	//------------------------------------------------------------------------------------------------
	void SendSnapshot(int playerId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller && !m_bStopped)
			controller.GC_SendBoundaryDetection(m_iRevision, m_aCounts, m_aPoints);
	}

	//------------------------------------------------------------------------------------------------
	//! At most four roster entries per 250 ms: a 128-player cycle takes eight seconds.
	//! This backstop is independent of client reports. It never performs a fast roster sweep.
	protected void Audit()
	{
		if (!IsActive() || m_aCounts.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(Audit);
			return;
		}
		if (m_iAuditCursor >= m_aAudit.Count())
		{
			m_aAudit.Clear();
			GetGame().GetPlayerManager().GetPlayers(m_aAudit);
			m_iAuditCursor = 0;
			m_fAuditCycleStarted = GetGame().GetWorld().GetWorldTime();
		}
		for (int i; i < 4 && m_iAuditCursor < m_aAudit.Count(); i++)
		{
			int playerId = m_aAudit[m_iAuditCursor++];
			if (!m_aActive.Contains(playerId))
				Observe(playerId);
		}
		int delay = 250;
		if (m_iAuditCursor >= m_aAudit.Count())
			delay = Math.Max(250, 8000 - (GetGame().GetWorld().GetWorldTime() - m_fAuditCycleStarted));
		GetGame().GetCallqueue().CallLater(Audit, delay);
	}

	//------------------------------------------------------------------------------------------------
	protected void ObserveActive()
	{
		array<int> players = {};
		players.Copy(m_aActive);
		Process(players);
	}

	//------------------------------------------------------------------------------------------------
	void Observe(int playerId)
	{
		array<int> players = {playerId};
		Process(players);
	}

	//------------------------------------------------------------------------------------------------
	protected void Process(array<int> players)
	{
		if (!IsActive() || m_aCounts.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(ObserveActive);
			m_bActiveScheduled = false;
			m_aActive.Clear();
			return;
		}
		PlayerManager manager = GetGame().GetPlayerManager();
		// Read native compartments for the affected mover, not every player's possession.
		int supplied = players.Count();
		for (int i; i < supplied; i++)
		{
			IEntity character = manager.GetPlayerControlledEntity(players[i]);
			if (!character)
				continue;
			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
			if (!vehicle)
				continue;
			SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compartments)
				continue;
			array<IEntity> occupants = {};
			compartments.GetOccupants(occupants);
			foreach (IEntity occupant : occupants)
			{
				int occupantId = manager.GetPlayerIdFromControlledEntity(occupant);
				if (occupantId > 0 && !players.Contains(occupantId))
					players.Insert(occupantId);
			}
		}
		foreach (GC_SetupZoneEntity setup : m_aSetups)
			setup.EvaluatePlayers(players);
		m_Mode.GetAOLimits().EvaluatePlayers(players);
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(manager.GetPlayerController(playerId));
			if (controller)
				controller.GC_BindBoundaryEligibility();
			IEntity body = manager.GetPlayerControlledEntity(playerId);
			bool observe;
			if (controller && controller.GetLobbyPlayerState() == GC_ELobbyPlayerState.PLAYING && body && SCR_AIDamageHandling.IsAlive(body))
			{
				IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(body);
				IEntity mover = body;
				if (vehicle)
					mover = vehicle;
				observe = m_Mode.GetAOLimits().NeedsObservation(playerId, body, vehicle);
				foreach (GC_SetupZoneEntity zone : m_aSetups)
					observe = observe || zone.NeedsObservation(playerId, mover);
			}
			if (observe && !m_aActive.Contains(playerId))
				m_aActive.Insert(playerId);
			else if (!observe)
				m_aActive.RemoveItem(playerId);
			foreach (GC_BoundaryWallComponent wall : m_aWalls)
				wall.RefreshPlayer(playerId);
		}
		if (m_aActive.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(ObserveActive);
			m_bActiveScheduled = false;
		}
		else if (!m_bActiveScheduled)
		{
			m_bActiveScheduled = true;
			GetGame().GetCallqueue().CallLater(ObserveActive, 100, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ForgetPlayer(int playerId)
	{
		m_aActive.RemoveItem(playerId);
		m_Mode.GetAOLimits().ForgetPlayer(playerId);
		foreach (GC_BoundaryWallComponent wall : m_aWalls)
			wall.ForgetPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bStopped = true;
		GetGame().GetCallqueue().Remove(Rebuild);
		GetGame().GetCallqueue().Remove(Audit);
		GetGame().GetCallqueue().Remove(ObserveActive);
		m_aActive.Clear();
		m_aSetups.Clear();
		m_aWalls.Clear();
	}
}
