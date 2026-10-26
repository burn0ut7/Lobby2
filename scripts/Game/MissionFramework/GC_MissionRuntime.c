class GC_PendingMissionAction
{
	ref GC_MissionAction m_Action;
	int m_iEarliestTick;
	string m_sEventName;
	bool m_bAllowBeforeGame;
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
	protected ref array<ref GC_MissionFlag> m_aFlags;
	protected ref map<string, bool> m_mFlags = new map<string, bool>();
	protected ref set<int> m_DisabledEvents = new set<int>();
	protected ref set<int> m_RequestedEvents = new set<int>();
	protected ref set<string> m_RunScripts = new set<string>();
	protected int m_iTick;
	protected float m_fNow;
	protected bool m_bExecutingBeforeGame;
	protected string m_sExecutingEvent;
	protected int m_iExecutedActions;
	protected bool m_bStopped = true;
	protected bool m_bValidConfiguration;
	protected ref ScriptInvoker m_OnFlagChanged = new ScriptInvoker();
	protected ref set<IEntity> m_ProtectedEntities = new set<IEntity>();
	protected ref GC_MissionAISettings m_AISettings = new GC_MissionAISettings();

	//------------------------------------------------------------------------------------------------
	bool Start(GC_LobbyMissionContext context, array<ref GC_MissionEvent> events, array<ref GC_MissionFlag> flags = null)
	{
		Stop();
		m_bValidConfiguration = false;
		if (!context || !context.IsActive() || !events || (flags && flags.Count() > 4096))
		{
			Print("GC | Mission Framework | rejected: invalid context, event array or flag count", LogLevel.ERROR);
			return false;
		}
		m_Context = context;
		m_aEvents = events;
		m_aFlags = flags;
		context.Bind(this);
		m_mFlags.Clear();
		if (flags)
		{
			foreach (GC_MissionFlag flag : flags)
			{
				if (!flag || flag.m_sName.IsEmpty() || m_mFlags.Contains(flag.m_sName)
					|| (flag.m_bRandomizeInitially && (flag.m_fInitialChance < 0 || flag.m_fInitialChance > 100)))
				{
					Print("GC | Mission Framework | invalid or duplicate flag name", LogLevel.ERROR);
					Stop();
					return false;
				}
				bool initial = flag.m_bInitiallySet;
				if (flag.m_bRandomizeInitially)
					initial = flag.m_fInitialChance >= 100 || (flag.m_fInitialChance > 0 && Math.RandomFloat01() < flag.m_fInitialChance * 0.01);
				m_mFlags.Set(flag.m_sName, initial);
			}
			foreach (GC_MissionFlag flag : flags)
			{
				string flagError;
				if (flag.m_Condition && !flag.m_Condition.Validate(context, flagError))
				{
					PrintFormat("GC | Mission Framework | invalid flag=%1: %2", flag.m_sName, flagError, level: LogLevel.ERROR);
					Stop();
					return false;
				}
			}
		}
		set<string> names = new set<string>();
		foreach (int eventIndex, GC_MissionEvent missionEvent : events)
		{
			string error;
			if (!missionEvent || !missionEvent.Validate(context, error))
			{
				PrintFormat("GC | Mission Framework | invalid event at index %1: %2", eventIndex, error, level: LogLevel.ERROR);
				Stop();
				return false;
			}
			if (names.Contains(missionEvent.m_sName))
			{
				PrintFormat("GC | Mission Framework | duplicate event name: %1", missionEvent.m_sName, level: LogLevel.ERROR);
				Stop();
				return false;
			}
			names.Insert(missionEvent.m_sName);
		}
		m_Context = context;
		m_aEvents = events;
		m_StartedEvents.Clear();
		m_DisabledEvents.Clear();
		m_RequestedEvents.Clear();
		m_RunScripts.Clear();
		m_iExecutedActions = 0;
		m_bStopped = false;
		m_bValidConfiguration = true;
		context.StartObservations();
		foreach (string flagName, bool initialValue : m_mFlags)
		{
			PrintFormat("GC | Mission Framework | flag initialized: name=%1 set=%2", flagName, initialValue);
			context.GetManager().GC_PublishMissionFlag(flagName, initialValue);
		}
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
		m_iTick++;
		m_fNow = now;
		m_Context.SetTime(now);
		m_AISettings.ReleasePlayerControlled();
		m_Context.CapturePlayers();
		// Automatic flags read one prior flag snapshot; changes publish together.
		// This makes flag dependencies deterministic and prevents recursive cycles.
		map<string, bool> updates = new map<string, bool>();
		if (m_aFlags)
		{
			foreach (GC_MissionFlag flag : m_aFlags)
			{
				if (!flag.m_Condition || (flag.m_bKeepSet && IsFlagSet(flag.m_sName)))
					continue;
				updates.Set(flag.m_sName, flag.m_Condition.Evaluate(m_Context));
			}
		}
		foreach (string name, bool value : updates)
			SetFlag(name, value);
		if (m_bStopped || !m_Context)
			return;
		foreach (int index, GC_MissionEvent missionEvent : m_aEvents)
		{
			if (m_DisabledEvents.Contains(index) || (!m_Context.IsRunning() && !missionEvent.m_bAllowBeforeGame)
				|| m_StartedEvents.Contains(index) || (!m_RequestedEvents.Contains(index) && !missionEvent.IsMet(m_Context)))
				continue;
			m_RequestedEvents.RemoveItem(index);
			m_StartedEvents.Insert(index);
			PrintFormat("GC | Mission Framework | event started: %1", missionEvent.m_sName);
			foreach (GC_MissionAction action : missionEvent.m_aActions)
			{
				GC_PendingMissionAction pending = new GC_PendingMissionAction();
				pending.m_Action = action;
				pending.m_sEventName = missionEvent.m_sName;
				pending.m_bAllowBeforeGame = missionEvent.m_bAllowBeforeGame;
				pending.m_fDueTime = now + action.m_fDelayAfterEventStarts;
				m_aPending.Insert(pending);
				PrintFormat("GC | Mission Framework | action queued: event=%1 type=%2 delaySeconds=%3", missionEvent.m_sName, action.Type(), action.m_fDelayAfterEventStarts);
			}
		}
		for (int i; i < m_aPending.Count();)
		{
			if (!m_Context || !m_Context.IsActive())
			{
				Stop();
				return;
			}
			GC_PendingMissionAction pending = m_aPending[i];
			if (pending.m_iEarliestTick > m_iTick || pending.m_fDueTime > now || (!m_Context.IsRunning() && !pending.m_bAllowBeforeGame))
			{
				i++;
				continue;
			}
			// Retain the removed record through execution; never retry a failed mutation.
			ref GC_PendingMissionAction executing = pending;
			m_aPending.RemoveOrdered(i);
			m_bExecutingBeforeGame = executing.m_bAllowBeforeGame;
			m_sExecutingEvent = executing.m_sEventName;
			bool applied = executing.m_Action.Execute(m_Context);
			m_iExecutedActions++;
			PrintFormat("GC | Mission Framework | action: event=%1 type=%2 applied=%3", executing.m_sEventName, executing.m_Action.Type(), applied);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (!m_bStopped)
			PrintFormat("GC | Mission Framework | stopped: cancelledActions=%1", m_aPending.Count());
		m_bStopped = true;
		m_aPending.Clear();
		if (m_Context)
			m_Context.StopObservations();
		m_Context = null;
		m_aEvents = null;
		m_aFlags = null;
		m_OnFlagChanged.Clear();
		m_ProtectedEntities.Clear();
		m_AISettings.Clear();
	}

	//------------------------------------------------------------------------------------------------
	bool SetAIHoldPosition(IEntity entity, bool hold)
	{
		return !m_bStopped && m_Context && m_Context.IsActive() && m_AISettings.HoldPosition(entity, hold);
	}

	//------------------------------------------------------------------------------------------------
	bool QueueFollowup(GC_MissionAction action, string name)
	{
		if (m_bStopped || !m_Context || !m_Context.IsActive() || !action)
			return false;
		GC_PendingMissionAction pending = new GC_PendingMissionAction();
		pending.m_Action = action;
		// Follow-ups retain the originating event so Disable cancels the entire chain.
		pending.m_sEventName = m_sExecutingEvent;
		pending.m_bAllowBeforeGame = m_bExecutingBeforeGame;
		pending.m_fDueTime = m_fNow + action.m_fDelayAfterEventStarts;
		pending.m_iEarliestTick = m_iTick + 1;
		m_aPending.Insert(pending);
		PrintFormat("GC | Mission Framework | follow-up queued: name=%1 delaySeconds=%2", name, action.m_fDelayAfterEventStarts);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool HasFlag(string name)
	{
		return m_mFlags.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	bool IsFlagSet(string name)
	{
		return m_mFlags.Get(name);
	}

	//------------------------------------------------------------------------------------------------
	bool SetFlag(string name, bool value)
	{
		if (m_bStopped || !m_Context || !m_Context.IsActive() || name.IsEmpty())
			return false;
		bool exists = HasFlag(name);
		if (!exists && m_mFlags.Count() >= 4096)
		{
			PrintFormat("GC | Mission Framework | flag rejected: name=%1 capacity=4096", name, level: LogLevel.ERROR);
			return false;
		}
		if (exists && m_mFlags.Get(name) == value)
			return true;
		m_mFlags.Set(name, value);
		if (exists)
			PrintFormat("GC | Mission Framework | flag changed: name=%1 set=%2", name, value);
		else
			PrintFormat("GC | Mission Framework | flag created: name=%1 set=%2", name, value);
		m_Context.GetManager().GC_PublishMissionFlag(name, value);
		// An absent flag reads false; creating it false is not a value change.
		if (exists || value)
			m_OnFlagChanged.Invoke(name, value);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side custom-script notification. Callbacks receive (string name, bool value).
	ScriptInvoker GetOnFlagChanged()
	{
		return m_OnFlagChanged;
	}

	//------------------------------------------------------------------------------------------------
	//! Invalid optional rules must never corrupt the owning lobby's replication stream.
	void CopyFlagSnapshot(notnull array<string> names, notnull array<bool> values)
	{
		names.Clear();
		values.Clear();
		if (!m_bValidConfiguration)
			return;
		foreach (string name, bool value : m_mFlags)
		{
			names.Insert(name);
			values.Insert(value);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ProtectEntity(IEntity entity, bool protect)
	{
		if (m_bStopped || !m_Context || !m_Context.IsActive() || !entity)
			return;
		if (protect)
			m_ProtectedEntities.Insert(entity);
		else
			m_ProtectedEntities.RemoveItem(entity);
	}

	//------------------------------------------------------------------------------------------------
	bool IsEntityProtected(IEntity entity)
	{
		foreach (IEntity protectedEntity : m_ProtectedEntities)
		{
			if (!protectedEntity)
				continue;
			IEntity ancestor = entity;
			while (ancestor)
			{
				if (ancestor == protectedEntity)
					return true;
				ancestor = ancestor.GetParent();
			}
			// Deleting an ancestor would also delete the protected descendant.
			ancestor = protectedEntity.GetParent();
			while (ancestor)
			{
				if (ancestor == entity)
					return true;
				ancestor = ancestor.GetParent();
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	int FindEvent(string name)
	{
		if (m_aEvents)
		{
			foreach (int index, GC_MissionEvent missionEvent : m_aEvents)
			{
				if (missionEvent && missionEvent.m_sName == name)
					return index;
			}
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Requests are evaluated on the next tick, never recursively in an action.
	bool ControlEvent(string name, GC_EMissionEventOperation operation, bool allowRepeat)
	{
		int index = FindEvent(name);
		if (m_bStopped || index < 0 || !m_Context.IsActive())
			return false;
		if (operation == GC_EMissionEventOperation.DISABLE)
		{
			foreach (GC_MissionCondition condition : m_aEvents[index].m_aConditions)
				condition.Reset(m_Context);
			m_DisabledEvents.Insert(index);
			m_RequestedEvents.RemoveItem(index);
			for (int i = m_aPending.Count() - 1; i >= 0; i--)
			{
				if (m_aPending[i].m_sEventName == name)
					m_aPending.RemoveOrdered(i);
			}
		}
		else if (operation == GC_EMissionEventOperation.REARM)
		{
			foreach (GC_MissionCondition condition : m_aEvents[index].m_aConditions)
				condition.Reset(m_Context);
			m_DisabledEvents.RemoveItem(index);
			m_StartedEvents.RemoveItem(index);
		}
		else
		{
			if (m_DisabledEvents.Contains(index) || (m_StartedEvents.Contains(index) && !allowRepeat))
				return false;
			m_StartedEvents.RemoveItem(index);
			m_RequestedEvents.Insert(index);
		}
		PrintFormat("GC | Mission Framework | event control: event=%1 operation=%2", name, typename.EnumToString(GC_EMissionEventOperation, operation));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool ClaimScript(string name, bool repeat)
	{
		if (m_bStopped || (!repeat && m_RunScripts.Contains(name)))
			return false;
		m_RunScripts.Insert(name);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string Describe()
	{
		return string.Format("stopped=%1 started=%2 pending=%3 attempted=%4", m_bStopped, m_StartedEvents.Count(), m_aPending.Count(), m_iExecutedActions);
	}
}
