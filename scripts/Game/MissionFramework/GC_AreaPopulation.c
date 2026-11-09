class GC_AreaCharacter
{
	IEntity m_Entity;
	vector m_vPosition;
	string m_sFaction;
	bool m_bPlayer;
}

//! One native-registry snapshot per sample, only when a Players / AI target needs it.
class GC_AreaPopulation
{
	ref array<ref GC_AreaCharacter> m_aCharacters = {};
	protected ref set<IEntity> m_aSeen = new set<IEntity>();
	protected bool m_bAvailable;
	protected bool m_bReportedLimit;

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_bAvailable;
	}

	//------------------------------------------------------------------------------------------------
	void Sample(GC_LobbyManager manager, bool includeAI)
	{
		m_aCharacters.Clear();
		m_aSeen.Clear();
		m_bAvailable = false;
		if (!manager || manager != GC_LobbyManager.GetManager() || !Replication.IsServer() || !GetGame())
			return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || (includeAI && !GetGame().GetAIWorld()))
			return;
		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int id : ids)
			AddCharacter(players.GetPlayerControlledEntity(id), true);
		if (includeAI)
		{
			set<IEntity> slots = new set<IEntity>();
			foreach (GC_Slot slot : manager.GetSlots())
			{
				if (slot && slot.GetEntity())
					slots.Insert(slot.GetEntity());
			}
			array<AIAgent> agents = {};
			GetGame().GetAIWorld().GetAIAgents(agents);
			if (agents.Count() > 4096)
			{
				ReportLimit();
				return;
			}
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				IEntity entity = agent.GetControlledEntity();
				if (!entity || slots.Contains(entity) || m_aSeen.Contains(entity) || players.GetPlayerIdFromControlledEntity(entity) > 0)
					continue;
				SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
				if (group && group.IsPlayable())
					continue;
				AddCharacter(entity, false);
			}
		}
		if (m_aCharacters.Count() > 4096)
		{
			ReportLimit();
			return;
		}
		m_bAvailable = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddCharacter(IEntity entity, bool player)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character || m_aSeen.Contains(entity) || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.IsDead())
			return;
		m_aSeen.Insert(entity);
		GC_AreaCharacter entry = new GC_AreaCharacter();
		entry.m_Entity = entity;
		entry.m_vPosition = entity.GetOrigin();
		entry.m_sFaction = character.GetFactionKey();
		entry.m_bPlayer = player;
		m_aCharacters.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected void ReportLimit()
	{
		if (!m_bReportedLimit)
			Print("GC | Mission Framework | Area population exceeds 4096 registered agents/characters; population checks unavailable.", LogLevel.WARNING);
		m_bReportedLimit = true;
	}
}
