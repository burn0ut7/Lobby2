class GC_PendingMissionAction
{
	GC_MissionAction m_Action;
	string m_sEventName;
	float m_fDueTime;
}

//! One execution owner: validates the complete configuration before doing work,
//! samples conditions, schedules one-shot actions and discards work on shutdown.
class GC_MissionRuntime
{
	protected ref GC_LobbyMissionContext m_Context;
	protected ref array<ref GC_MissionEvent> m_aEvents;
	protected ref set<int> m_StartedEvents = new set<int>();
	protected ref array<ref GC_PendingMissionAction> m_aPending = {};
	protected int m_iExecutedActions;
	protected bool m_bStopped = true;

	//------------------------------------------------------------------------------------------------
	bool Start(GC_LobbyMissionContext context, array<ref GC_MissionEvent> events)
	{
		Stop();
		if (!context || !context.IsOwned() || !events || events.IsEmpty())
		{
			Print("GC | Mission Framework | rejected: active authority and at least one event required", LogLevel.ERROR);
			return false;
		}
		set<string> names = new set<string>();
		foreach (int eventIndex, GC_MissionEvent missionEvent : events)
		{
			string error;
			if (!missionEvent || !missionEvent.Validate(context, error))
			{
				PrintFormat("GC | Mission Framework | invalid event at index %1: %2", eventIndex, error, level: LogLevel.ERROR);
				return false;
			}
			if (names.Contains(missionEvent.m_sName))
			{
				PrintFormat("GC | Mission Framework | duplicate event name: %1", missionEvent.m_sName, level: LogLevel.ERROR);
				return false;
			}
			names.Insert(missionEvent.m_sName);
		}
		m_Context = context;
		m_aEvents = events;
		m_StartedEvents.Clear();
		m_iExecutedActions = 0;
		m_bStopped = false;
		PrintFormat("GC | Mission Framework | ready: events=%1", events.Count());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Time is mission world time in seconds. Only the owning server may execute.
	void Update(float now)
	{
		if (m_bStopped)
			return;
		if (m_Context.HasEnded())
		{
			Stop();
			return;
		}
		if (!m_Context.IsRunning())
			return;
		m_Context.CapturePlayers();
		foreach (int index, GC_MissionEvent missionEvent : m_aEvents)
		{
			if (m_StartedEvents.Contains(index) || !missionEvent.IsMet(m_Context))
				continue;
			m_StartedEvents.Insert(index);
			PrintFormat("GC | Mission Framework | event started: %1", missionEvent.m_sName);
			foreach (GC_MissionAction action : missionEvent.m_aActions)
			{
				GC_PendingMissionAction pending = new GC_PendingMissionAction();
				pending.m_Action = action;
				pending.m_sEventName = missionEvent.m_sName;
				pending.m_fDueTime = now + action.m_fDelayAfterEventStarts;
				m_aPending.Insert(pending);
			}
		}
		for (int i; i < m_aPending.Count();)
		{
			if (!m_Context.IsRunning())
			{
				Stop();
				return;
			}
			GC_PendingMissionAction pending = m_aPending[i];
			if (pending.m_fDueTime > now)
			{
				i++;
				continue;
			}
			// Retain the removed record through execution; never retry a failed mutation.
			ref GC_PendingMissionAction executing = pending;
			m_aPending.RemoveOrdered(i);
			bool applied = executing.m_Action.Execute(m_Context);
			m_iExecutedActions++;
			PrintFormat("GC | Mission Framework | action: event=%1 type=%2 applied=%3", executing.m_sEventName, executing.m_Action.Type(), applied);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bStopped = true;
		m_aPending.Clear();
		m_Context = null;
		m_aEvents = null;
	}

	//------------------------------------------------------------------------------------------------
	string Describe()
	{
		return string.Format("stopped=%1 started=%2 pending=%3 attempted=%4", m_bStopped, m_StartedEvents.Count(), m_aPending.Count(), m_iExecutedActions);
	}
}
