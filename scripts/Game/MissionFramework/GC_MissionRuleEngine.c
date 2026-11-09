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
	ref GC_CompiledMissionRule m_Rule;
	GC_MissionRuleState m_Parent;
	ref array<ref GC_MissionRuleState> m_aChoices;
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
	protected SCR_EGameModeState m_eGameModeState = -1;
	protected bool m_bRunning;
	protected bool m_bDispatching;
	protected float m_fNow;
	protected int m_iRuns;
	protected int m_iInvalidRules;
	protected bool m_bEventLimitReached;

	//------------------------------------------------------------------------------------------------
	void Start(array<ref GC_CompiledMissionRule> rules, array<ref GC_MissionFlag> flags = null, SCR_EGameModeState initialState = -1)
	{
		Stop();
		m_aStates.Clear();
		m_mFlags.Clear();
		m_fNow = 0;
		m_eGameModeState = -1;
		m_iRuns = 0;
		m_iInvalidRules = 0;
		m_bEventLimitReached = false;
		if (flags)
		{
			foreach (GC_MissionFlag flag : flags)
			{
				if (!flag || flag.m_sName.IsEmpty() || flag.m_iStartingValue < 0 || flag.m_iStartingValue > 1
					|| m_mFlags.Contains(flag.m_sName) || flags.Count() > 256)
				{
					m_mFlags.Clear();
					m_iInvalidRules = 1;
					Print("GC | Mission Framework | Invalid or duplicate Mission Flag; rules not started.", LogLevel.ERROR);
					return;
				}
				m_mFlags.Set(flag.m_sName, flag.m_iStartingValue == 1);
			}
		}
		if (!rules || rules.Count() > 256)
		{
			m_iInvalidRules = 1;
			Print("GC | Mission Framework | Cannot start: the rule list is missing or contains more than 256 rules.", LogLevel.ERROR);
			return;
		}
		int total = rules.Count();
		foreach (GC_CompiledMissionRule countedRule : rules)
		{
			if (countedRule && countedRule.m_aChoices)
				total += countedRule.m_aChoices.Count();
		}
		if (total > 256)
		{
			m_iInvalidRules = 1;
			Print("GC | Mission Framework | More than 256 total events including group children; not started.", LogLevel.ERROR);
			return;
		}
		foreach (GC_CompiledMissionRule rule : rules)
		{
			GC_MissionRuleState state = CreateState(rule);
			if (!state.m_bValid)
			{
				m_iInvalidRules++;
				Print("GC | Mission Framework | Incomplete or invalid event skipped.", LogLevel.ERROR);
			}
		}
		m_bRunning = true;
		if (GC_LobbyPhase.IsValid(initialState))
			SetGameModeState(initialState);
		else
		{
			// Historical interpreter callers without a live state retain match-start semantics.
			m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.MATCH_BEGINS, ""));
			DrainEvents();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected GC_MissionRuleState CreateState(GC_CompiledMissionRule rule, GC_MissionRuleState parent = null)
	{
		GC_MissionRuleState state = new GC_MissionRuleState();
		state.m_Rule = rule;
		state.m_Parent = parent;
		state.m_bValid = rule && rule.IsValid();
		m_aStates.Insert(state);
		if (state.m_bValid)
		{
			state.m_fNextCheck = rule.m_fEverySeconds;
			if (rule.m_aChoices)
			{
				state.m_aChoices = {};
				foreach (GC_CompiledMissionRule child : rule.m_aChoices)
					state.m_aChoices.Insert(CreateState(child, state));
			}
		}
		return state;
	}

	//------------------------------------------------------------------------------------------------
	//! Observe the owning mode; this never changes the engine's authoritative phase.
	void SetGameModeState(SCR_EGameModeState state, float elapsedSeconds = -1)
	{
		if (!m_bRunning || !GC_LobbyPhase.IsValid(state) || state == m_eGameModeState)
			return;
		if (elapsedSeconds > m_fNow)
			m_fNow = elapsedSeconds;
		m_eGameModeState = state;
		foreach (GC_MissionRuleState pending : m_aStates)
		{
			if (!pending.m_bValid || !pending.m_bPending)
				continue;
			GC_CompiledMissionRule rule = pending.m_Rule;
			// Selected children inherit the parent event's match-end lifetime.
			if (pending.m_Parent)
				rule = pending.m_Parent.m_Rule;
			if (!ConditionsPass(pending)
				|| (state == SCR_EGameModeState.POSTGAME && rule.m_eWhen != GC_ERuleEvent.GAME_MODE_STATE))
				pending.m_bPending = false;
		}
		m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.GAME_MODE_STATE, ""));
		if (state == SCR_EGameModeState.GAME)
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
			if (!state.m_Parent && state.m_bValid && state.m_Rule.m_bEnabled && state.m_Rule.m_eWhen == kind
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
			if (!state.m_Parent && state.m_Rule.m_eWhen == GC_ERuleEvent.TIMER && m_eGameModeState != SCR_EGameModeState.POSTGAME && state.m_fNextCheck <= m_fNow)
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
		GC_CompiledMissionRule rule = state.m_Rule;
		if (rule.m_bRunOnlyOnce && state.m_iRuns > 0)
			return false;
		return ConditionsPass(state);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ConditionsPass(GC_MissionRuleState state)
	{
		if (!state.m_bValid || !state.m_Rule.m_bEnabled)
			return false;
		if (state.m_Rule.m_If && !state.m_Rule.m_If.Evaluate(m_mFlags, m_eGameModeState))
			return false;
		return !state.m_Parent || ConditionsPass(state.m_Parent);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryRun(GC_MissionRuleState state)
	{
		if (!CanRun(state))
			return;
		float chance = state.m_Rule.m_fChance;
		if (chance <= 0 || (chance < 1 && RandomUnit() >= chance))
			return;
		float delay = state.m_Rule.m_fWaitSeconds;
		GC_RuleRandom random = state.m_Rule.m_Random;
		if (random)
		{
			if (!PassChance(random.m_fChancePercent))
				return;
			delay += random.m_fMinimumSeconds + (random.m_fMaximumSeconds - random.m_fMinimumSeconds) * RandomUnit();
		}
		if (delay > 0)
		{
			state.m_bPending = true;
			state.m_fDue = m_fNow + delay;
			return;
		}
		Execute(state);
	}

	//------------------------------------------------------------------------------------------------
	protected void Execute(GC_MissionRuleState state)
	{
		if (state.m_aChoices)
		{
			ExecuteRandomGroup(state);
			return;
		}
		// Commit once-only state before actions can emit another event.
		state.m_iRuns++;
		m_iRuns++;
		int selected = -1;
		if (state.m_Rule.m_Random && state.m_Rule.m_Random.m_iActionSelection == 1)
			selected = Math.Min(Math.Floor(RandomUnit() * state.m_Rule.m_aThen.Count()), state.m_Rule.m_aThen.Count() - 1);
		foreach (int index, GC_RuleAction action : state.m_Rule.m_aThen)
		{
			if (selected >= 0 && index != selected)
				continue;
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
				case GC_ERuleAction.RANDOM_FLAG:
					SetFlag(action.m_sTextOrName, PassChance(action.m_fChancePercent));
					break;
				case GC_ERuleAction.SELECT_FLAGS:
					SelectFlags(action);
					break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteRandomGroup(GC_MissionRuleState group)
	{
		array<GC_MissionRuleState> pool = {};
		foreach (GC_MissionRuleState child : group.m_aChoices)
		{
			if (CanRun(child))
				pool.Insert(child);
		}
		// No eligible events: keep a once-only group available for a later notification.
		if (pool.IsEmpty())
			return;
		array<GC_MissionRuleState> selected = {};
		int picks = Math.Min(group.m_Rule.m_iPickCount, pool.Count());
		for (int i; i < picks; i++)
		{
			int index = Math.Min(Math.Floor(RandomUnit() * pool.Count()), pool.Count() - 1);
			selected.Insert(pool[index]);
			pool.RemoveOrdered(index);
		}
		// Commit selection once before child actions can emit another notification.
		group.m_iRuns++;
		foreach (GC_MissionRuleState chosen : selected)
			TryRun(chosen);
	}

	//------------------------------------------------------------------------------------------------
	protected float RandomUnit()
	{
		return Math.RandomFloat01();
	}

	//------------------------------------------------------------------------------------------------
	protected bool PassChance(float percent)
	{
		return percent >= 100 || (percent > 0 && RandomUnit() < percent * 0.01);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectFlags(GC_RuleAction action)
	{
		array<string> pool = {};
		pool.Copy(action.m_aFlagNames);
		array<string> selected = {};
		for (int index; index < action.m_iSelectionCount; index++)
		{
			int pick = Math.Min(Math.Floor(RandomUnit() * pool.Count()), pool.Count() - 1);
			selected.Insert(pool[pick]);
			pool.RemoveOrdered(pick);
		}
		// Dispatch is already deferred by Execute's caller: observers see the final selection.
		foreach (string name : action.m_aFlagNames)
			SetFlag(name, selected.Contains(name));
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
				if (state.m_Parent || !state.m_bValid)
					continue;
				GC_CompiledMissionRule rule = state.m_Rule;
				if (rule.m_eWhen == notification.m_eKind && (notification.m_eKind == GC_ERuleEvent.MATCH_BEGINS || notification.m_eKind == GC_ERuleEvent.GAME_MODE_STATE
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
