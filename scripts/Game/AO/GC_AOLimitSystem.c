//! Per-character violation clocks; possession is always resolved from PlayerManager.
class GC_AOPlayerState
{
	IEntity m_Character;
	ref map<GC_AOLimitEntity, float> m_mDeadlines = new map<GC_AOLimitEntity, float>();
	GC_AOLimitEntity m_Selected;
	float m_fDeadline;
}

//! A safe transform belongs to the actual moving body, including occupied vehicles.
class GC_AOReturnState
{
	vector m_aTransform[4];
	bool m_bHasSafeTransform;
	bool m_bReturning;
	bool m_bFailed;
}

//! One server-owned evaluator. Limits never independently teleport or kill players.
class GC_AOLimitSystem
{
	protected GC_LobbyGameMode m_Mode;
	protected ref array<GC_AOLimitEntity> m_aLimits = {};
	protected ref map<int, ref GC_AOPlayerState> m_mPlayers = new map<int, ref GC_AOPlayerState>();
	protected ref map<IEntity, ref GC_AOReturnState> m_mReturns = new map<IEntity, ref GC_AOReturnState>();

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyGameMode mode)
	{
		m_Mode = mode;
		GetGame().GetCallqueue().CallLater(Evaluate, 100, true);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		GetGame().GetCallqueue().Remove(Evaluate);
		ClearPlayers();
		m_aLimits.Clear();
		m_Mode = null;
	}

	//------------------------------------------------------------------------------------------------
	bool Register(GC_AOLimitEntity limit)
	{
		if (!Replication.IsServer())
			return false;
		if (!m_aLimits.Contains(limit))
			m_aLimits.Insert(limit);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Unregister(GC_AOLimitEntity limit)
	{
		ResetLimit(limit);
		m_aLimits.RemoveItem(limit);
	}

	//------------------------------------------------------------------------------------------------
	void ResetLimit(GC_AOLimitEntity limit)
	{
		foreach (int playerId, GC_AOPlayerState state : m_mPlayers)
		{
			state.m_mDeadlines.Remove(limit);
			if (state.m_Selected == limit)
			{
				state.m_Selected = null;
				SendWarning(playerId, state.m_Character, -1);
			}
		}
		// Retain history: every destination is revalidated against current rules.
		// Removing a rule must not lose the return point needed by another rule.
		foreach (IEntity mover, GC_AOReturnState returnState : m_mReturns)
			returnState.m_bFailed = false;
	}

	//------------------------------------------------------------------------------------------------
	static bool TakesPrecedence(GC_AOLimitEntity candidate, float deadline, GC_AOLimitEntity current, float currentDeadline)
	{
		if (!current)
			return true;
		if (candidate.GetPriority() != current.GetPriority())
			return candidate.GetPriority() > current.GetPriority();
		if (candidate.GetBehavior() != current.GetBehavior())
			return candidate.GetBehavior() == GC_EAOBehavior.HARD_RETURN;
		if (candidate.GetBehavior() == GC_EAOBehavior.SOFT_KILL && deadline != currentDeadline)
			return deadline < currentDeadline;
		// Exact ties retain the first evaluated limit in the server's scan.
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void SendWarning(int playerId, IEntity character, float seconds)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller)
			controller.GC_SendAOWarning(character, seconds);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearPlayers()
	{
		foreach (int playerId, GC_AOPlayerState state : m_mPlayers)
			SendWarning(playerId, state.m_Character, -1);
		m_mPlayers.Clear();
		m_mReturns.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected GC_AOPlayerState EvaluatePlayer(int playerId, IEntity character, IEntity vehicle, float now)
	{
		GC_AOPlayerState state;
		if (!m_mPlayers.Find(playerId, state) || state.m_Character != character)
		{
			state = new GC_AOPlayerState();
			state.m_Character = character;
		}
		GC_AOLimitEntity previous = state.m_Selected;
		state.m_Selected = null;
		state.m_fDeadline = 0;
		IEntity mover = character;
		if (vehicle)
			mover = vehicle;
		foreach (GC_AOLimitEntity limit : m_aLimits)
		{
			if (!limit || !limit.AppliesTo(playerId, character, vehicle) || limit.Permits(mover.GetOrigin()))
			{
				state.m_mDeadlines.Remove(limit);
				continue;
			}
			float deadline;
			if (!state.m_mDeadlines.Find(limit, deadline))
			{
				deadline = now + limit.GetGraceSeconds();
				state.m_mDeadlines.Set(limit, deadline);
			}
			if (TakesPrecedence(limit, deadline, state.m_Selected, state.m_fDeadline))
			{
				state.m_Selected = limit;
				state.m_fDeadline = deadline;
			}
		}
		if (state.m_Selected && state.m_Selected.GetBehavior() == GC_EAOBehavior.SOFT_KILL)
		{
			SendWarning(playerId, character, Math.Max(0, state.m_fDeadline - now));
			if (now >= state.m_fDeadline)
			{
				SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
				if (damage)
				{
					damage.SetHealthScaled(0);
					PrintFormat("GC | AO | expired player=%1 limit=%2 dead=%3", playerId, state.m_Selected.GetName(), damage.IsDestroyed());
				}
				SendWarning(playerId, character, -1);
			}
		}
		else if (previous)
			SendWarning(playerId, character, -1);
		return state;
	}

	//------------------------------------------------------------------------------------------------
	//! Validate against every applicable hard rule for every connected occupant.
	protected bool IsSafeForMover(IEntity mover, vector position, out bool constrained)
	{
		constrained = false;
		foreach (int playerId, GC_AOPlayerState state : m_mPlayers)
		{
			IEntity character = state.m_Character;
			if (!character)
				continue;
			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
			if (vehicle != mover && character != mover)
				continue;
			foreach (GC_AOLimitEntity limit : m_aLimits)
			{
				if (!limit || limit.GetBehavior() != GC_EAOBehavior.HARD_RETURN || !limit.AppliesTo(playerId, character, vehicle))
					continue;
				constrained = true;
				if (!limit.Permits(position))
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void EvaluateReturn(IEntity mover, GC_AOLimitEntity selected, GC_AOReturnState state)
	{
		bool constrained;
		if (IsSafeForMover(mover, mover.GetOrigin(), constrained))
		{
			// An exemption is not evidence of a safe location for later re-entry
			// into the rule. Keep the last actually constrained safe transform.
			if (constrained)
			{
				mover.GetWorldTransform(state.m_aTransform);
				state.m_bHasSafeTransform = true;
			}
			if (state.m_bReturning)
				PrintFormat("GC | AO | return observed mover=%1", mover);
			state.m_bReturning = false;
			state.m_bFailed = false;
			return;
		}
		if (!selected)
			return;
		vector destination[4];
		bool hasDestination;
		if (state.m_bHasSafeTransform && IsSafeForMover(mover, state.m_aTransform[3], constrained))
		{
			Math3D.MatrixCopy(state.m_aTransform, destination);
			hasDestination = true;
		}
		else if (selected.GetReturnTransform(destination))
			hasDestination = IsSafeForMover(mover, destination[3], constrained);
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(mover.FindComponent(SCR_EditableEntityComponent));
		if (!hasDestination || !editable)
		{
			if (!state.m_bFailed)
				PrintFormat("GC | AO | hard return unavailable: safe destination=%1 editable=%2 mover=%3 limit=%4", hasDestination, editable, mover, selected.GetName(), level: LogLevel.ERROR);
			state.m_bFailed = true;
			return;
		}
		// Native movement can cross the edge again before our next sample.
		// Enforce the current violation each tick; an accepted owner RPC is not
		// a receipt that requires a later inside sample before enforcing again.
		bool accepted = editable.SetTransform(destination);
		if (accepted && !state.m_bReturning)
			PrintFormat("GC | AO | return dispatched=1 mover=%1 limit=%2 destination=%3", mover, selected.GetName(), destination[3]);
		else if (!accepted && !state.m_bFailed)
			PrintFormat("GC | AO | return rejected mover=%1 limit=%2", mover, selected.GetName(), level: LogLevel.ERROR);
		state.m_bReturning = accepted;
		state.m_bFailed = !accepted;
	}

	//------------------------------------------------------------------------------------------------
	protected void Evaluate()
	{
		if (!Replication.IsServer() || GC_LobbyGameMode.GetGameMode() != m_Mode || !GC_LobbyManager.GetManager()
			|| m_Mode.GetState() != SCR_EGameModeState.GAME)
		{
			ClearPlayers();
			return;
		}
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (freeze && freeze.IsFreezeActive())
		{
			ClearPlayers();
			return;
		}
		float now = m_Mode.GetWorld().GetWorldTime() * 0.001;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		map<int, ref GC_AOPlayerState> currentPlayers = new map<int, ref GC_AOPlayerState>();
		map<IEntity, GC_AOLimitEntity> movers = new map<IEntity, GC_AOLimitEntity>();
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller || controller.GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
				continue;
			ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
			if (!character || !SCR_AIDamageHandling.IsAlive(character))
				continue;
			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
			GC_AOPlayerState state = EvaluatePlayer(playerId, character, vehicle, now);
			currentPlayers.Set(playerId, state);
			IEntity mover = character;
			if (vehicle)
				mover = vehicle;
			GC_AOLimitEntity selected;
			movers.Find(mover, selected);
			if (state.m_Selected && state.m_Selected.GetBehavior() == GC_EAOBehavior.HARD_RETURN
				&& TakesPrecedence(state.m_Selected, 0, selected, 0))
				selected = state.m_Selected;
			movers.Set(mover, selected);
		}
		foreach (int oldPlayerId, GC_AOPlayerState oldState : m_mPlayers)
		{
			if (!currentPlayers.Contains(oldPlayerId))
				SendWarning(oldPlayerId, oldState.m_Character, -1);
		}
		m_mPlayers = currentPlayers;
		map<IEntity, ref GC_AOReturnState> currentReturns = new map<IEntity, ref GC_AOReturnState>();
		foreach (IEntity movingEntity, GC_AOLimitEntity selectedLimit : movers)
		{
			GC_AOReturnState returnState;
			if (!m_mReturns.Find(movingEntity, returnState))
				returnState = new GC_AOReturnState();
			EvaluateReturn(movingEntity, selectedLimit, returnState);
			currentReturns.Set(movingEntity, returnState);
		}
		m_mReturns = currentReturns;
	}
}
