//! Typed events keep flag changes separate from named test events.
class GC_MissionRuleEvent
{
	GC_ERuleEvent m_eKind;
	string m_sName;

	void GC_MissionRuleEvent(GC_ERuleEvent kind, string name)
	{
		m_eKind = kind;
		m_sName = name;
	}
}

//! Runtime state is separate from Workbench-authored settings.
class GC_MissionRuleState
{
	ref GC_MissionRule m_Rule;
	bool m_bValid;
	bool m_bPending;
	int m_iRuns;
	float m_fDue;
	float m_fNextCheck;
}

//! Server-local rule interpreter. The owning component enforces mode and authority.
class GC_MissionRuleEngine
{
	protected ref array<ref GC_MissionRuleState> m_aStates = {};
	protected ref map<string, bool> m_mFlags = new map<string, bool>();
	protected ref array<ref GC_MissionRuleEvent> m_aEvents = {};
	protected bool m_bRunning;
	protected bool m_bDispatching;
	protected float m_fNow;
	protected int m_iRuns;
	protected int m_iInvalidRules;
	protected bool m_bEventLimitReached;

	//------------------------------------------------------------------------------------------------
	void Start(array<ref GC_MissionRule> rules)
	{
		Stop();
		m_aStates.Clear();
		m_mFlags.Clear();
		m_fNow = 0;
		m_iRuns = 0;
		m_iInvalidRules = 0;
		m_bEventLimitReached = false;
		if (!rules || rules.Count() > 256)
		{
			m_iInvalidRules = 1;
			Print("GC | Mission Framework | Cannot start: the rule list is missing or contains more than 256 rules.", LogLevel.ERROR);
			return;
		}
		foreach (GC_MissionRule rule : rules)
		{
			GC_MissionRuleState state = new GC_MissionRuleState();
			state.m_Rule = rule;
			state.m_bValid = rule && rule.IsValid();
			if (!state.m_bValid)
			{
				m_iInvalidRules++;
				PrintFormat("GC | Mission Framework | Rule %1 is incomplete or invalid; skipped.", m_aStates.Count() + 1);
			}
			else
				state.m_fNextCheck = rule.m_fEverySeconds;
			m_aStates.Insert(state);
		}
		m_bRunning = true;
		m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.MATCH_BEGINS, ""));
		DrainEvents();
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bRunning = false;
		m_aEvents.Clear();
		foreach (GC_MissionRuleState state : m_aStates)
			state.m_bPending = false;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	bool GetFlag(string name)
	{
		bool answer;
		m_mFlags.Find(name, answer);
		return answer;
	}

	//------------------------------------------------------------------------------------------------
	bool SetFlag(string name, bool answer)
	{
		if (!m_bRunning || name.IsEmpty())
			return false;
		if (GetFlag(name) == answer)
			return true;
		m_mFlags.Set(name, answer);
		GC_ERuleEvent kind = GC_ERuleEvent.FLAG_OFF;
		if (answer)
			kind = GC_ERuleEvent.FLAG_ON;
		// Existing flag-only rules must not consume the event-chain budget.
		bool watched;
		foreach (GC_MissionRuleState state : m_aStates)
		{
			if (state.m_bValid && state.m_Rule.m_bEnabled && state.m_Rule.m_eWhen == kind
				&& state.m_Rule.m_sTestEventName == name)
			{
				watched = true;
				break;
			}
		}
		if (!watched)
			return true;
		return QueueEvent(kind, name);
	}

	//------------------------------------------------------------------------------------------------
	int GetRunCount()
	{
		return m_iRuns;
	}

	//------------------------------------------------------------------------------------------------
	int GetInvalidRuleCount()
	{
		return m_iInvalidRules;
	}

	//------------------------------------------------------------------------------------------------
	bool EventLimitReached()
	{
		return m_bEventLimitReached;
	}

	//------------------------------------------------------------------------------------------------
	bool SendTestEvent(string name)
	{
		return QueueEvent(GC_ERuleEvent.TEST_EVENT, name);
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueueEvent(GC_ERuleEvent kind, string name)
	{
		if (!m_bRunning || name.IsEmpty())
			return false;
		if (m_aEvents.Count() >= 256)
		{
			StopEventLoop();
			return false;
		}
		m_aEvents.Insert(new GC_MissionRuleEvent(kind, name));
		DrainEvents();
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	//! elapsedSeconds is monotonic time since Start, supplied by the owning world.
	void Advance(float elapsedSeconds)
	{
		if (!m_bRunning || elapsedSeconds < m_fNow)
			return;
		m_fNow = elapsedSeconds;
		// Queue events until this pass completes, preventing reentrant timer execution.
		m_bDispatching = true;
		foreach (GC_MissionRuleState state : m_aStates)
		{
			if (!m_bRunning)
				break;
			if (!state.m_bValid)
				continue;
			if (state.m_bPending && state.m_fDue <= m_fNow)
			{
				state.m_bPending = false;
				if (CanRun(state))
					Execute(state);
			}
			if (state.m_Rule.m_eWhen == GC_ERuleEvent.TIMER && state.m_fNextCheck <= m_fNow)
			{
				state.m_fNextCheck = m_fNow + state.m_Rule.m_fEverySeconds;
				TryRun(state);
			}
		}
		m_bDispatching = false;
		DrainEvents();
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanRun(GC_MissionRuleState state)
	{
		if (!m_bRunning || !state.m_bValid || !state.m_Rule.m_bEnabled || state.m_bPending)
			return false;
		GC_MissionRule rule = state.m_Rule;
		if (rule.m_bRunOnlyOnce && state.m_iRuns > 0)
			return false;
		return !rule.m_If || rule.m_If.Evaluate(m_mFlags);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryRun(GC_MissionRuleState state)
	{
		if (!CanRun(state))
			return;
		if (state.m_Rule.m_fWaitSeconds > 0)
		{
			state.m_bPending = true;
			state.m_fDue = m_fNow + state.m_Rule.m_fWaitSeconds;
			return;
		}
		Execute(state);
	}

	//------------------------------------------------------------------------------------------------
	protected void Execute(GC_MissionRuleState state)
	{
		// Commit once-only state before actions can emit another event.
		state.m_iRuns++;
		m_iRuns++;
		foreach (GC_RuleAction action : state.m_Rule.m_aThen)
		{
			if (!m_bRunning)
				break;
			switch (action.m_eDo)
			{
				case GC_ERuleAction.WRITE_LOG:
					PrintFormat("GC | Mission Framework | %1: %2", state.m_Rule.m_sName, action.m_sTextOrName);
					break;
				case GC_ERuleAction.SET_FLAG:
					SetFlag(action.m_sTextOrName, action.m_bFlagAnswer);
					break;
				case GC_ERuleAction.SEND_TEST_EVENT:
					SendTestEvent(action.m_sTextOrName);
					break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DrainEvents()
	{
		if (m_bDispatching || !m_bRunning)
			return;
		m_bDispatching = true;
		int processed;
		while (m_bRunning && !m_aEvents.IsEmpty())
		{
			if (++processed > 256)
			{
				StopEventLoop();
				break;
			}
			GC_MissionRuleEvent notification = m_aEvents[0];
			m_aEvents.RemoveOrdered(0);
			foreach (GC_MissionRuleState state : m_aStates)
			{
				if (!state.m_bValid)
					continue;
				GC_MissionRule rule = state.m_Rule;
				if (rule.m_eWhen == notification.m_eKind && (notification.m_eKind == GC_ERuleEvent.MATCH_BEGINS
					|| rule.m_sTestEventName == notification.m_sName))
					TryRun(state);
			}
		}
		m_bDispatching = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void StopEventLoop()
	{
		m_bEventLimitReached = true;
		Stop();
		Print("GC | Mission Framework | Stopped: too many chained test events. Check for rules that keep sending each other events.", LogLevel.ERROR);
	}
}
