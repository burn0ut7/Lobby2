//! The server owns a single ballot for POSTGAME. Catalogue credentials never leave authority.
class GC_MissionVote
{
	protected GC_LobbyManager m_Manager;
	protected RestContext m_Context;
	protected ref RestCallback m_Callback;
	protected ref array<string> m_aCandidates = {};
	protected ref array<string> m_aScenarios = {};
	protected ref array<string> m_aRows = {};
	protected ref array<int> m_aCounts = {};
	protected ref map<string, int> m_mBallots = new map<string, int>();
	protected string m_sRound;
	protected string m_sStatus = "#GC-Vote_NotConfigured";
	protected int m_iOffset;
	protected int m_iWinner = -1;
	protected bool m_bReady;
	protected bool m_bStarted;
	protected bool m_bClosed;
	protected bool m_bStopped;

	//------------------------------------------------------------------------------------------------
	void GC_MissionVote(GC_LobbyManager manager)
	{
		m_Manager = manager;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCurrent()
	{
		return !m_bStopped && m_Manager && Replication.IsServer() && GC_LobbyManager.GetManager() == m_Manager;
	}

	//------------------------------------------------------------------------------------------------
	void Prepare()
	{
		if (!IsCurrent())
			return;
		m_Context = GC_MissionServerConnection.Open(m_sStatus);
		if (!m_Context)
			return;
		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnCatalog);
		m_Callback.SetOnError(OnError);
		m_sStatus = "#GC-Vote_Loading";
	}

	//------------------------------------------------------------------------------------------------
	protected void FetchPage()
	{
		m_Context.GET(m_Callback, "api/v1/catalog/voting?offset=" + m_iOffset);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnError(RestCallback callback)
	{
		if (IsCurrent())
			m_sStatus = "#GC-Vote_Unavailable";
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCatalog(RestCallback callback)
	{
		if (!IsCurrent() || m_bClosed || callback.GetData().Length() > 262144)
			return;
		JsonLoadContext json = new JsonLoadContext(false);
		array<string> rows = {};
		int version, next;
		if (!json.LoadFromString(callback.GetData()) || !json.ReadValue("version", version) || version != 1
			|| !json.ReadValue("rows", rows) || rows.Count() > 25 || !json.ReadValue("nextOffset", next))
		{
			m_sStatus = "#GC-Vote_InvalidResponse";
			return;
		}
		foreach (string row : rows)
		{
			string scenario;
			JsonLoadContext item = new JsonLoadContext(false);
			if (row.Length() > 3800 || !item.LoadFromString(row) || !item.ReadValue("scenario", scenario)
				|| scenario.IsEmpty() || m_aScenarios.Contains(scenario))
				continue;
			// A remote catalogue entry is not proof that this server has the resource.
			SCR_MissionHeader header = SCR_MissionHeader.Cast(MissionHeader.ReadMissionHeader(scenario));
			if (!header || header.GetWorldPath().IsEmpty())
				continue;
			m_aScenarios.Insert(scenario);
			m_aCandidates.Insert(row);
		}
		if (next >= 0)
		{
			if (next != m_iOffset + 25 || next > 1000)
			{
				m_sStatus = "#GC-Vote_Limit";
				return;
			}
			m_iOffset = next;
			FetchPage();
			return;
		}
		m_bReady = true;
		if (m_bStarted)
			SelectCandidates();
	}

	//------------------------------------------------------------------------------------------------
	void Start()
	{
		if (!IsCurrent() || m_bStarted || m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME)
			return;
		m_bStarted = true;
		m_sRound = PersistenceIdUtils.Generate();
		// Fetch at debrief so the ballot uses current catalogue/history data.
		if (m_Context)
			FetchPage();
		GetGame().GetCallqueue().CallLater(Tick, 250, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectCandidates()
	{
		if (m_bClosed || !m_aRows.IsEmpty())
			return;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		SelectForPopulation(players.Count());
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectForPopulation(int players)
	{
		array<int> upper = {}, lower = {}, shared = {};
		for (int i; i < m_aCandidates.Count(); i++)
		{
			int band = GetPopulationBand(m_aCandidates[i], players);
			if (band == 0)
				lower.Insert(i);
			else if (band == 1)
				upper.Insert(i);
			else if (band == 2)
				shared.Insert(i);
		}
		m_sStatus = "#GC-Vote_Empty";
		if (upper.IsEmpty() && lower.IsEmpty() && shared.IsEmpty())
			return;
		// Fixed slots preserve top/bottom placement even when one band is sparse.
		for (int slot; slot < 6; slot++)
		{
			m_aRows.Insert("");
			m_aCounts.Insert(0);
		}
		FillPopulationRow(upper, 0);
		FillPopulationRow(lower, 3);
		// Exact-current capacities fit either row; consume them only once.
		FillPopulationRow(shared, 0);
		FillPopulationRow(shared, 3);
		m_sStatus = "#GC-Vote_Choose";
	}

	//------------------------------------------------------------------------------------------------
	protected void FillPopulationRow(array<int> available, int first)
	{
		for (int slot = first; slot < first + 3 && !available.IsEmpty(); slot++)
		{
			if (!m_aRows[slot].IsEmpty())
				continue;
			int choice = Math.RandomInt(0, available.Count());
			m_aRows[slot] = m_aCandidates[available[choice]];
			available.RemoveOrdered(choice);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Maximum capacity determines the row; the mission minimum must fit today's population.
	protected int GetPopulationBand(string row, int players)
	{
		if (players <= 0)
			return -1;
		JsonLoadContext json = new JsonLoadContext(false);
		int minimum, maximum;
		int lowerCapacity = Math.Ceil(players * 0.8);
		int upperCapacity = Math.Floor(players * 1.2);
		if (!json.LoadFromString(row) || !json.ReadValue("minPlayers", minimum)
			|| !json.ReadValue("maxPlayers", maximum) || minimum <= 0 || maximum < minimum
			|| minimum > players || maximum < lowerCapacity || maximum > upperCapacity)
			return -1;
		if (maximum == players)
			return 2;
		if (maximum > players)
			return 1;
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasCandidate(int index)
	{
		return index >= 0 && index < m_aRows.Count() && !m_aRows[index].IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	int Remaining()
	{
		return Math.Max(0, 120 - m_Manager.GetPhaseElapsedSeconds());
	}

	//------------------------------------------------------------------------------------------------
	void CastVote(GC_LobbyPlayerController controller, string round, int choice)
	{
		if (!IsCurrent() || !m_bStarted || m_bClosed || Remaining() <= 0 || round != m_sRound
			|| m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME || !HasCandidate(choice))
			return;
		int playerId = controller.GetPlayerId();
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			|| GetGame().GetPlayerManager().GetPlayerController(playerId) != controller)
			return;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (identity.IsEmpty())
			return;
		int previous;
		if (m_mBallots.Find(identity, previous))
			m_aCounts[previous] = m_aCounts[previous] - 1;
		m_mBallots.Set(identity, choice);
		m_aCounts[choice] = m_aCounts[choice] + 1;
	}

	//------------------------------------------------------------------------------------------------
	void Deliver(GC_LobbyPlayerController controller, string knownRound)
	{
		if (!IsCurrent() || !m_bStarted || m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME)
			return;
		int selected = -1;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(controller.GetPlayerId());
		if (!identity.IsEmpty() && !m_mBallots.Find(identity, selected))
			selected = -1;
		controller.GC_SendVoteState(m_sRound, m_sStatus, Remaining(), m_aCounts, selected, m_iWinner, m_bClosed);
		if (knownRound != m_sRound)
		{
			foreach (int index, string row : m_aRows)
				controller.GC_SendVoteCard(m_sRound, index, row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!IsCurrent() || m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME)
		{
			Stop();
			return;
		}
		if (Remaining() > 0 || m_bClosed)
			return;
		m_bClosed = true;
		GetGame().GetCallqueue().Remove(Tick);
		if (m_aRows.IsEmpty())
		{
			m_sStatus = "#GC-Vote_EndedEmpty";
			return;
		}
		m_iWinner = PickWinner();
		if (m_iWinner < 0)
		{
			m_sStatus = "#GC-Vote_EndedEmpty";
			return;
		}
		m_sStatus = "#GC-Vote_LoadingWinner";
		GetGame().GetCallqueue().CallLater(LoadWinner, 3000);
	}

	//------------------------------------------------------------------------------------------------
	protected int PickWinner()
	{
		int highest = -1;
		array<int> tied = {};
		foreach (int index, int count : m_aCounts)
		{
			if (!HasCandidate(index))
				continue;
			if (count > highest)
			{
				highest = count;
				tied.Clear();
			}
			if (count == highest)
				tied.Insert(index);
		}
		if (tied.IsEmpty())
			return -1;
		return tied[Math.RandomInt(0, tied.Count())];
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadWinner()
	{
		if (!IsCurrent() || m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME || m_iWinner < 0)
			return;
		JsonLoadContext json = new JsonLoadContext(false);
		string scenario;
		if (!json.LoadFromString(m_aRows[m_iWinner]) || !json.ReadValue("scenario", scenario))
			return;
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		string addonList;
		foreach (string addon : addons)
		{
			if (GameProject.IsVanillaAddon(addon))
				continue;
			if (!addonList.IsEmpty())
				addonList += ",";
			addonList += addon;
		}
		if (!GameStateTransitions.RequestScenarioChangeTransition(scenario, string.Empty, addonList))
			m_sStatus = "#GC-Vote_LoadFailed";
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bStopped = true;
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(LoadWinner);
		}
	}
}
