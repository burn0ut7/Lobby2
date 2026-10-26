//! Lobby-component-owned lifecycle. The component supplies authored events; this
//! module owns initialization, its only timer, execution and teardown.
class GC_MissionFramework
{
	protected GC_LobbyManager m_Manager;
	protected ref array<ref GC_MissionEvent> m_aEvents;
	protected ref GC_MissionRuntime m_Runtime;

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyManager manager, array<ref GC_MissionEvent> events)
	{
		Stop();
		if (!Replication.IsServer() || !manager || GC_LobbyManager.GetManager() != manager || !events || events.IsEmpty())
			return;
		m_Manager = manager;
		m_aEvents = events;
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
		m_Runtime = new GC_MissionRuntime();
		if (m_Runtime.Start(context, m_aEvents))
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
		m_Manager = null;
	}

	//------------------------------------------------------------------------------------------------
	string Describe()
	{
		if (!m_Runtime)
			return "inactive";
		return m_Runtime.Describe();
	}
}
