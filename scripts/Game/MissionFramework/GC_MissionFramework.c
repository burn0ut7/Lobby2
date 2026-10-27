//! Lobby-component-owned lifecycle. The component supplies authored events; this
//! module owns initialization, its only timer, execution and teardown.
class GC_MissionFramework
{
	protected GC_LobbyManager m_Manager;
	protected ref array<ref GC_MissionEvent> m_aEvents;
	protected ref GC_MissionRuntime m_Runtime;
	protected ref array<ref GC_MissionFlag> m_aFlags;

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyManager manager, array<ref GC_MissionEvent> events, array<ref GC_MissionFlag> flags)
	{
		Stop();
		if (!Replication.IsServer() || !manager || GC_LobbyManager.GetManager() != manager)
			return;
		m_Manager = manager;
		m_aEvents = {};
		if (events)
		{
			foreach (GC_MissionEvent definition : events)
				m_aEvents.Insert(definition);
		}
		m_aFlags = {};
		if (flags)
		{
			foreach (GC_MissionFlag flag : flags)
				m_aFlags.Insert(flag);
		}
		// Other world entities, including the faction manager, initialize first.
		GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Initialize()
	{
		if (!Replication.IsServer() || !m_Manager || GC_LobbyManager.GetManager() != m_Manager)
		{
			Stop();
			return;
		}
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(m_Manager);
		string authoringError;
		if (!GC_MissionAuthoringEntity.Collect(m_Manager.GetOwner().GetWorld(), m_aEvents, m_aFlags, authoringError))
		{
			PrintFormat("GC | Mission Framework | invalid entity configuration: %1", authoringError, level: LogLevel.ERROR);
			Stop();
			return;
		}
		m_Runtime = new GC_MissionRuntime();
		if (m_Runtime.Start(context, m_aEvents, m_aFlags))
			GetGame().GetCallqueue().CallLater(Tick, 250, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!Replication.IsServer() || !m_Manager || GC_LobbyManager.GetManager() != m_Manager
			|| m_Manager.GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
		{
			Stop();
			return;
		}
		m_Runtime.Update(m_Manager.GetOwner().GetWorld().GetWorldTime() * 0.001);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Initialize);
			GetGame().GetCallqueue().Remove(Tick);
		}
		if (m_Runtime)
			m_Runtime.Stop();
		m_aEvents = null;
		m_aFlags = null;
		m_Manager = null;
	}

	//------------------------------------------------------------------------------------------------
	GC_MissionRuntime GetRuntime()
	{
		return m_Runtime;
	}

	//------------------------------------------------------------------------------------------------
	string Describe()
	{
		if (!m_Runtime)
			return "inactive";
		return m_Runtime.Describe();
	}
}
