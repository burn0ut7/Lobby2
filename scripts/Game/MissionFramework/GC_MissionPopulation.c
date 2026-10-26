//! Session observations, never a second player/faction ownership source.
//! Native game-mode callbacks account for AI killed between polling ticks.
class GC_MissionPopulation
{
	protected GC_LobbyGameMode m_Mode;
	protected ref map<string, int> m_mLivingPlayers = new map<string, int>();
	protected ref map<string, int> m_mPeakPlayers = new map<string, int>();
	protected ref map<string, int> m_mAILives = new map<string, int>();
	protected ref map<string, int> m_mAIDeaths = new map<string, int>();
	protected ref set<EntityID> m_ObservedAI = new set<EntityID>();
	protected ref set<EntityID> m_DeadAI = new set<EntityID>();
	protected ref array<SCR_ChimeraCharacter> m_aCharacters = {};
	protected bool m_bIncludeAI;

	//------------------------------------------------------------------------------------------------
	void Start(bool includeAI)
	{
		m_bIncludeAI = includeAI;
		m_Mode = GC_LobbyGameMode.GetGameMode();
		if (includeAI && m_Mode)
			m_Mode.GetOnControllableDestroyed().Insert(OnDestroyed);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (m_Mode && m_bIncludeAI)
			m_Mode.GetOnControllableDestroyed().Remove(OnDestroyed);
		m_Mode = null;
		m_aCharacters.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDestroyed(SCR_InstigatorContextData data)
	{
		if (!m_Mode || GC_LobbyGameMode.GetGameMode() != m_Mode || !GC_LobbyManager.GetManager()
			|| m_Mode.GetState() != SCR_EGameModeState.GAME || data.GetVictimPlayerID() > 0)
			return;
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(data.GetVictimEntity());
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		string faction = character.GetFactionKey();
		if (faction.IsEmpty() || m_DeadAI.Contains(character.GetID()))
			return;
		ObserveAI(character);
		m_DeadAI.Insert(character.GetID());
		m_mAIDeaths.Set(faction, m_mAIDeaths.Get(faction) + 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void ObserveAI(SCR_ChimeraCharacter character)
	{
		if (m_ObservedAI.Contains(character.GetID()) || character.GetFactionKey().IsEmpty())
			return;
		m_ObservedAI.Insert(character.GetID());
		string faction = character.GetFactionKey();
		m_mAILives.Set(faction, m_mAILives.Get(faction) + 1);
	}

	//------------------------------------------------------------------------------------------------
	void Capture(bool recordCasualties)
	{
		m_mLivingPlayers.Clear();
		m_aCharacters.Clear();
		PlayerManager players = GetGame().GetPlayerManager();
		SCR_FactionManager factions = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!players || !factions)
			return;
		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int id : ids)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(players.GetPlayerControlledEntity(id));
			if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent) || !SCR_AIDamageHandling.IsAlive(character))
				continue;
			Faction faction = factions.GetPlayerFaction(id);
			if (!faction)
				continue;
			string key = faction.GetFactionKey();
			m_mLivingPlayers.Set(key, m_mLivingPlayers.Get(key) + 1);
			m_aCharacters.Insert(character);
		}
		foreach (string key, int count : m_mLivingPlayers)
		{
			if (recordCasualties && count > m_mPeakPlayers.Get(key))
				m_mPeakPlayers.Set(key, count);
		}
		if (!m_bIncludeAI || !GetGame().GetAIWorld())
			return;
		array<AIAgent> agents = {};
		GetGame().GetAIWorld().GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (!character || m_aCharacters.Contains(character) || !SCR_AIDamageHandling.IsAlive(character)
				|| character.FindComponent(GC_LobbySpectatorBodyComponent)
				|| players.GetPlayerIdFromControlledEntity(character) > 0)
				continue;
			if (recordCasualties)
				ObserveAI(character);
			m_aCharacters.Insert(character);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool PlayerCasualties(string faction, out float percentage)
	{
		int peak = m_mPeakPlayers.Get(faction);
		if (peak <= 0)
			return false;
		percentage = 100.0 * (peak - m_mLivingPlayers.Get(faction)) / peak;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool AICasualties(string faction, out float percentage)
	{
		int lives = m_mAILives.Get(faction);
		if (lives <= 0)
			return false;
		percentage = 100.0 * m_mAIDeaths.Get(faction) / lives;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	array<SCR_ChimeraCharacter> GetCharacters()
	{
		return m_aCharacters;
	}
}
