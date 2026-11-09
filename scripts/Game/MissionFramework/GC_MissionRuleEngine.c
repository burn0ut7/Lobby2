//! Typed events keep flag changes separate from named test events.
class GC_MissionRuleEvent
{
	GC_ERuleEvent m_eKind;
	string m_sName;
	bool m_bFlagCall;
	bool m_bFlagClear;
	bool m_bInitialState;
	ref array<GC_RuleCondition> m_aRandomChecks;
	ref array<GC_RuleCondition> m_aDamageChecks;

	void GC_MissionRuleEvent(GC_ERuleEvent kind, string name, bool flagCall = false)
	{
		m_eKind = kind;
		m_sName = name;
		m_bFlagCall = flagCall;
	}
}

//! Runtime flag memory is separate from authored containers.
class GC_MissionFlagState
{
	string m_sName;
	bool m_bRepeatable;
	ref GC_RuleCondition m_Condition;
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
	protected ref map<string, int> m_mFlagCounts = new map<string, int>();
	protected ref map<string, ref GC_MissionFlagState> m_mFlagDefinitions = new map<string, ref GC_MissionFlagState>();
	protected ref array<string> m_aCalledFlags = {};
	protected ref array<string> m_aPulseFlags = {};
	protected ref map<string, bool> m_mEventResults = new map<string, bool>();
	protected ref array<string> m_aPulseEvents = {};
	protected ref array<GC_MissionRuleState> m_aReactiveRuns = {};
	protected ref array<ref GC_MissionRuleEvent> m_aEvents = {};
	protected SCR_EGameModeState m_eGameModeState = -1;
	protected bool m_bRunning;
	protected bool m_bDispatching;
	protected float m_fNow;
	protected ref array<ref GC_RandomConditionState> m_aRandomConditions = {};
	protected ref map<GC_RuleCondition, bool> m_mRandomResults = new map<GC_RuleCondition, bool>();
	protected ref GC_CasualtySnapshot m_Casualties;
	protected BaseWorld m_DamageWorld;
	protected ref map<GC_RuleCondition, ref GC_EntityDamageObservation> m_mDamage = new map<GC_RuleCondition, ref GC_EntityDamageObservation>();
	protected int m_iRuns;
	protected int m_iInvalidRules;
	protected bool m_bEventLimitReached;

	//------------------------------------------------------------------------------------------------
	void Start(array<ref GC_CompiledMissionRule> rules, array<ref GC_MissionFlag> flags = null, SCR_EGameModeState initialState = -1, array<string> authoredEventNames = null, GC_CasualtySnapshot casualties = null, BaseWorld damageWorld = null)
	{
		Stop();
		m_Casualties = casualties;
		m_aStates.Clear();
		m_aRandomConditions.Clear();
		m_mFlags.Clear();
		m_mFlagCounts.Clear();
		m_mFlagDefinitions.Clear();
		m_mEventResults.Clear();
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
				GC_RuleCondition condition = flag.BuildConditions();
				if (flag.m_aConditions && !flag.m_aConditions.IsEmpty() && !condition)
				{
					m_mFlags.Clear();
					m_mFlagDefinitions.Clear();
					m_iInvalidRules = 1;
					Print("GC | Mission Framework | Invalid flag conditions; not started.", LogLevel.ERROR);
					return;
				}
				GC_MissionFlagState memory = new GC_MissionFlagState();
				memory.m_sName = flag.m_sName;
				memory.m_bRepeatable = flag.m_bRepeatable;
				memory.m_Condition = condition;
				m_mFlagDefinitions.Set(flag.m_sName, memory);
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
		RegisterEventNames(authoredEventNames);
		RegisterRandomConditions();
		m_DamageWorld = damageWorld;
		RegisterEntityDamageConditions();
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
	protected void RegisterEventNames(array<string> authoredEventNames)
	{
		array<string> suppliedNames = authoredEventNames;
		if (!suppliedNames)
		{
			suppliedNames = {};
			foreach (GC_MissionRuleState state : m_aStates)
			{
				if (state.m_Rule)
					suppliedNames.Insert(state.m_Rule.m_sName);
			}
		}
		map<string, int> names = new map<string, int>();
		foreach (string name : suppliedNames)
		{
			if (name.IsEmpty())
				continue;
			int count;
			names.Find(name, count);
			names.Set(name, count + 1);
		}
		foreach (GC_MissionRuleState state : m_aStates)
		{
			if (state.m_bValid && !state.m_Rule.m_sName.IsEmpty() && names.Get(state.m_Rule.m_sName) == 1)
				m_mEventResults.Set(state.m_Rule.m_sName, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The owner supplies the frozen starting cohort before Match entry, then notifies confirmed deaths.
	void SetCasualties(GC_CasualtySnapshot casualties, bool notify = true)
	{
		m_Casualties = casualties;
		if (notify && m_bRunning && m_eGameModeState == SCR_EGameModeState.GAME)
			QueueEvent(GC_ERuleEvent.CASUALTIES_CHANGED, "casualties");
	}

	//------------------------------------------------------------------------------------------------
	bool GetEventResult(string name)
	{
		bool result;
		m_mEventResults.Find(name, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void PublishEventResult(GC_MissionRuleState state)
	{
		string name = state.m_Rule.m_sName;
		if (!m_bRunning || !m_mEventResults.Contains(name))
			return;
		m_mEventResults.Set(name, true);
		if (!state.m_Rule.m_bRunOnlyOnce && !m_aPulseEvents.Contains(name))
			m_aPulseEvents.Insert(name);
		bool watched;
		foreach (GC_MissionRuleState observer : m_aStates)
		{
			if (!observer.m_Parent && observer.m_bValid && observer.m_Rule.m_bObserveEvents && observer.m_Rule.m_If && observer.m_Rule.m_If.UsesEvent(name))
				watched = true;
		}
		foreach (string flagName, GC_MissionFlagState memory : m_mFlagDefinitions)
		{
			if (memory.m_Condition && memory.m_Condition.UsesEvent(name))
				watched = true;
		}
		if (!watched)
			return;
		if (m_aEvents.Count() >= 256)
		{
			StopEventLoop();
			return;
		}
		m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.EVENT_RAN, name));
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
		bool initialObservation = !GC_LobbyPhase.IsValid(m_eGameModeState);
		m_eGameModeState = state;
		ResetRandomConditions();
		if (state == SCR_EGameModeState.GAME)
			BindEntityDamageConditions();
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
		GC_MissionRuleEvent observation = new GC_MissionRuleEvent(GC_ERuleEvent.GAME_MODE_STATE, "");
		observation.m_bInitialState = initialObservation;
		m_aEvents.Insert(observation);
		if (state == SCR_EGameModeState.GAME)
		{
			m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.MATCH_BEGINS, ""));
			if (m_Casualties)
				m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.CASUALTIES_CHANGED, ""));
		}
		if (state == SCR_EGameModeState.GAME)
			QueueEntityDamageChanges();
		DrainEvents();
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bRunning = false;
		m_mRandomResults.Clear();
		m_Casualties = null;
		foreach (GC_RuleCondition check, GC_EntityDamageObservation observation : m_mDamage)
			observation.Stop();
		m_mDamage.Clear();
		m_DamageWorld = null;
		m_aEvents.Clear();
		ResetPulses();
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
	int GetFlagCallCount(string name)
	{
		int count;
		m_mFlagCounts.Find(name, count);
		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanCallFlag(string name)
	{
		GC_MissionFlagState memory;
		if (!m_bRunning || !m_mFlagDefinitions.Find(name, memory) || m_aCalledFlags.Contains(name))
			return false;
		if (!memory.m_bRepeatable && (GetFlag(name) || GetFlagCallCount(name) > 0))
			return false;
		if (GetFlagCallCount(name) == int.MAX)
			return false;
		return !memory.m_Condition || memory.m_Condition.Evaluate(m_mFlags, m_eGameModeState, m_mFlagCounts, m_mEventResults, m_mRandomResults, m_Casualties, m_mDamage);
	}

	//------------------------------------------------------------------------------------------------
	//! True means queued; queued calls recheck conditions when handled.
	bool CallFlag(string name)
	{
		if (!m_bRunning || !m_mFlagDefinitions.Contains(name) || m_aCalledFlags.Contains(name))
			return false;
		// Earlier queued actions may make conditions pass. Check again at execution.
		if (!m_bDispatching && !CanCallFlag(name))
			return false;
		if (m_aEvents.Count() >= 256)
		{
			StopEventLoop();
			return false;
		}
		m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.FLAG_ON, name, true));
		DrainEvents();
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	protected void AcceptFlagCall(string name)
	{
		if (!CanCallFlag(name))
			return;
		GC_MissionFlagState memory = m_mFlagDefinitions.Get(name);
		m_aCalledFlags.Insert(name);
		m_mFlagCounts.Set(name, GetFlagCallCount(name) + 1);
		m_mFlags.Set(name, true);
		if (memory.m_bRepeatable)
			m_aPulseFlags.Insert(name);
		m_aEvents.Insert(new GC_MissionRuleEvent(GC_ERuleEvent.FLAG_ON, name));
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetPulses()
	{
		foreach (string name : m_aPulseFlags)
			m_mFlags.Set(name, false);
		foreach (string eventName : m_aPulseEvents)
			m_mEventResults.Set(eventName, false);
		m_aPulseEvents.Clear();
		m_aPulseFlags.Clear();
		m_aCalledFlags.Clear();
		m_aReactiveRuns.Clear();
	}

	//------------------------------------------------------------------------------------------------
	bool SetFlag(string name, bool answer)
	{
		if (!m_bRunning || name.IsEmpty())
			return false;
		if (!m_mFlagDefinitions.Contains(name))
			return ApplyFlagValue(name, answer);
		if (answer)
			return CallFlag(name);
		if (m_aEvents.Count() >= 256)
		{
			StopEventLoop();
			return false;
		}
		GC_MissionRuleEvent clear = new GC_MissionRuleEvent(GC_ERuleEvent.FLAG_OFF, name);
		clear.m_bFlagClear = true;
		m_aEvents.Insert(clear);
		DrainEvents();
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ApplyFlagValue(string name, bool answer)
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
		{
			foreach (GC_MissionRuleState observer : m_aStates)
			{
				if (observer.m_bValid && observer.m_Rule.m_bObserveFlags && observer.m_Rule.m_If.UsesFlag(name))
					watched = true;
			}
			foreach (string key, GC_MissionFlagState flagObserver : m_mFlagDefinitions)
			{
				if (key != name && flagObserver.m_Condition && flagObserver.m_Condition.UsesFlag(name))
					watched = true;
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
		AdvanceRandomConditions();
		QueueEntityDamageChanges();
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
	protected void RegisterEntityDamageConditions()
	{
		array<GC_RuleCondition> conditions = {};
		foreach (GC_MissionRuleState state : m_aStates)
		{
			if (state.m_bValid && state.m_Rule.m_bEnabled && state.m_Rule.m_If)
				state.m_Rule.m_If.CollectEntityDamageConditions(conditions);
		}
		foreach (string name, GC_MissionFlagState memory : m_mFlagDefinitions)
		{
			if (memory.m_Condition)
				memory.m_Condition.CollectEntityDamageConditions(conditions);
		}
		foreach (GC_RuleCondition condition : conditions)
			m_mDamage.Set(condition, new GC_EntityDamageObservation());
		BindEntityDamageConditions();
	}

	//------------------------------------------------------------------------------------------------
	protected void BindEntityDamageConditions()
	{
		foreach (GC_RuleCondition condition, GC_EntityDamageObservation observation : m_mDamage)
			observation.Bind(m_DamageWorld, condition);
	}

	//------------------------------------------------------------------------------------------------
	//! Coalesce health/state callbacks so the complete tree sees the latest settled damage values.
	protected void QueueEntityDamageChanges()
	{
		if (m_eGameModeState != SCR_EGameModeState.GAME || m_mDamage.IsEmpty())
			return;
		GC_MissionRuleEvent notification = new GC_MissionRuleEvent(GC_ERuleEvent.ENTITY_DAMAGE_CHANGED, "");
		notification.m_aDamageChecks = {};
		foreach (GC_RuleCondition condition, GC_EntityDamageObservation observation : m_mDamage)
		{
			if (observation.ConsumeChange())
				notification.m_aDamageChecks.Insert(condition);
		}
		if (!notification.m_aDamageChecks.IsEmpty())
			m_aEvents.Insert(notification);
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterRandomConditions()
	{
		array<GC_RuleCondition> conditions = {};
		foreach (GC_MissionRuleState state : m_aStates)
		{
			if (state.m_bValid && state.m_Rule.m_bEnabled && state.m_Rule.m_If)
				state.m_Rule.m_If.CollectRandomConditions(conditions);
		}
		foreach (string name, GC_MissionFlagState memory : m_mFlagDefinitions)
		{
			if (memory.m_Condition)
				memory.m_Condition.CollectRandomConditions(conditions);
		}
		foreach (GC_RuleCondition condition : conditions)
		{
			GC_RandomConditionState scheduled = new GC_RandomConditionState();
			scheduled.m_Condition = condition;
			m_aRandomConditions.Insert(scheduled);
		}
		ResetRandomConditions();
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetRandomConditions()
	{
		m_mRandomResults.Clear();
		foreach (GC_RandomConditionState scheduled : m_aRandomConditions)
		{
			scheduled.m_fNextCheck = m_fNow + scheduled.m_Condition.m_fRandomInterval;
			scheduled.m_bHasRolled = false;
			m_mRandomResults.Set(scheduled.m_Condition, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Roll every due leaf before notifying any owner, so nesting sees one consistent snapshot.
	protected void AdvanceRandomConditions()
	{
		if (m_eGameModeState != SCR_EGameModeState.GAME || m_aRandomConditions.IsEmpty())
			return;
		GC_MissionRuleEvent notification = new GC_MissionRuleEvent(GC_ERuleEvent.RANDOM_CHECK, "");
		notification.m_aRandomChecks = {};
		foreach (GC_RandomConditionState scheduled : m_aRandomConditions)
		{
			if (scheduled.m_fNextCheck > m_fNow || (scheduled.m_bHasRolled && scheduled.m_Condition.m_iRandomBehavior == 1))
				continue;
			GC_RuleCondition condition = scheduled.m_Condition;
			float chance = condition.m_fRandomChance;
			bool result = chance >= 1 || (chance > 0 && RandomUnit() < chance);
			m_mRandomResults.Set(condition, result);
			scheduled.m_bHasRolled = true;
			// A late update rolls once, never bursts through missed intervals.
			scheduled.m_fNextCheck = m_fNow + condition.m_fRandomInterval;
			notification.m_aRandomChecks.Insert(condition);
		}
		if (!notification.m_aRandomChecks.IsEmpty())
			m_aEvents.Insert(notification);
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
		if (state.m_Rule.m_If && !state.m_Rule.m_If.Evaluate(m_mFlags, m_eGameModeState, m_mFlagCounts, m_mEventResults, m_mRandomResults, m_Casualties, m_mDamage))
			return false;
		return !state.m_Parent || ConditionsPass(state.m_Parent);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryRun(GC_MissionRuleState state)
	{
		if (!CanRun(state))
			return;
		if (state.m_Rule.m_bObserveFlags || state.m_Rule.m_bObserveEvents || (state.m_Rule.m_If && (state.m_Rule.m_If.UsesRandom() || state.m_Rule.m_If.UsesCasualties() || state.m_Rule.m_If.UsesEntityDamage())))
		{
			if (m_aReactiveRuns.Contains(state))
				return;
			m_aReactiveRuns.Insert(state);
		}
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
		// A reactive event must not call itself again through its result in this cycle.
		if ((state.m_Rule.m_bObserveFlags || state.m_Rule.m_bObserveEvents || (state.m_Rule.m_If && (state.m_Rule.m_If.UsesRandom() || state.m_Rule.m_If.UsesCasualties() || state.m_Rule.m_If.UsesEntityDamage()))) && !m_aReactiveRuns.Contains(state))
			m_aReactiveRuns.Insert(state);
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
				case GC_ERuleAction.CALL_FLAG:
					CallFlag(action.m_sTextOrName);
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
		PublishEventResult(state);
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
		PublishEventResult(group);
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
			if (notification.m_bFlagClear)
			{
				ApplyFlagValue(notification.m_sName, false);
				continue;
			}
			if (notification.m_bFlagCall)
			{
				AcceptFlagCall(notification.m_sName);
				continue;
			}
			bool damageChanged = notification.m_eKind == GC_ERuleEvent.ENTITY_DAMAGE_CHANGED;
			bool casualtyChanged = notification.m_eKind == GC_ERuleEvent.CASUALTIES_CHANGED;
			bool randomCheck = notification.m_eKind == GC_ERuleEvent.RANDOM_CHECK;
			bool eventRan = notification.m_eKind == GC_ERuleEvent.EVENT_RAN;
			bool flagChanged = notification.m_eKind == GC_ERuleEvent.FLAG_ON || notification.m_eKind == GC_ERuleEvent.FLAG_OFF;
			foreach (string flagName, GC_MissionFlagState memory : m_mFlagDefinitions)
			{
				if (!memory.m_Condition)
					continue;
				if ((flagChanged && flagName != notification.m_sName && memory.m_Condition.UsesFlag(notification.m_sName))
					|| (eventRan && memory.m_Condition.UsesEvent(notification.m_sName))
					|| (randomCheck && memory.m_Condition.UsesRandom(notification.m_aRandomChecks))
					|| (casualtyChanged && memory.m_Condition.UsesCasualties())
					|| (damageChanged && memory.m_Condition.UsesEntityDamage(notification.m_aDamageChecks))
					|| (notification.m_eKind == GC_ERuleEvent.GAME_MODE_STATE && memory.m_Condition.UsesGameModeState()))
					CallFlag(flagName);
			}
			foreach (GC_MissionRuleState state : m_aStates)
			{
				if (state.m_Parent || !state.m_bValid)
					continue;
				GC_CompiledMissionRule rule = state.m_Rule;
				if (rule.m_If && ((flagChanged && rule.m_bObserveFlags && rule.m_If.UsesFlag(notification.m_sName))
					|| (eventRan && rule.m_bObserveEvents && rule.m_If.UsesEvent(notification.m_sName))
					|| (randomCheck && rule.m_If.UsesRandom(notification.m_aRandomChecks))
					|| (casualtyChanged && rule.m_If.UsesCasualties())
					|| (damageChanged && rule.m_If.UsesEntityDamage(notification.m_aDamageChecks))))
				{
					// One eligible attempt per notification chain; false checks may pass on a later notification.
					TryRun(state);
					continue;
				}
				// An event-result reference does not subscribe to unrelated phase changes.
				if (notification.m_eKind == GC_ERuleEvent.GAME_MODE_STATE && !notification.m_bInitialState
					&& rule.m_bObserveEvents && rule.m_If && !rule.m_If.UsesGameModeState())
					continue;
				if (rule.m_eWhen == notification.m_eKind && (notification.m_eKind == GC_ERuleEvent.MATCH_BEGINS || notification.m_eKind == GC_ERuleEvent.GAME_MODE_STATE
					|| rule.m_sTestEventName == notification.m_sName))
					TryRun(state);
			}
		}
		ResetPulses();
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
