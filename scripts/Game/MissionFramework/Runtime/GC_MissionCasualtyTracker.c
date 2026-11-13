//! Match-start cohort owned exclusively by the active authoritative GC Lobby manager.
class GC_MissionCasualtyTracker
{
	protected ref array<IEntity> m_aRecordedDeaths = {};
	protected ref GC_CasualtySnapshot m_Snapshot = new GC_CasualtySnapshot();
	protected ref map<IEntity, ref GC_CasualtyTotals> m_mLiving = new map<IEntity, ref GC_CasualtyTotals>();

	//------------------------------------------------------------------------------------------------
	GC_CasualtySnapshot GetSnapshot()
	{
		return m_Snapshot;
	}

	//------------------------------------------------------------------------------------------------
	void Capture(GC_LobbyManager manager)
	{
		m_mLiving.Clear();
		m_aRecordedDeaths.Clear();
		m_Snapshot = new GC_CasualtySnapshot();
		PlayerManager players = GetGame().GetPlayerManager();
		array<int> playerIds = {};
		players.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			AddCharacter(players.GetPlayerControlledEntity(playerId), true);

		// Match entry precedes the queued possession catch-up. Connected claimed
		// slots already have their starting characters and must not count as AI.
		foreach (GC_Slot slot : manager.GetSlots())
		{
			if (slot && slot.HasClaim() && !slot.IsKIA() && playerIds.Contains(slot.GetPlayerId()))
				AddCharacter(slot.GetEntity(), true);
		}

		array<AIAgent> agents = {};
		if (GetGame().GetAIWorld())
			GetGame().GetAIWorld().GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity entity = agent.GetControlledEntity();
			if (!entity || manager.GetSlotByEntity(entity) || players.GetPlayerIdFromControlledEntity(entity) > 0)
				continue;
			SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (group && group.IsPlayable())
				continue;
			AddCharacter(entity, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddCharacter(IEntity entity, bool player)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character || m_mLiving.Contains(entity) || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.IsDead())
			return;
		m_mLiving.Set(entity, m_Snapshot.AddStartingLife(character.GetFactionKey(), player));
	}

	//------------------------------------------------------------------------------------------------
	//! Called only for the engine's confirmed controllable-destroyed notification.
	bool RecordDeath(IEntity entity, GC_LobbyManager manager, int playerId)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent) || m_aRecordedDeaths.Contains(entity))
			return false;
		GC_CasualtyTotals starting;
		m_mLiving.Find(entity, starting);
		GC_Slot slot = manager.GetSlotByEntity(entity);
		bool player = playerId > 0 || (starting && starting.m_bPlayers) || (slot && slot.HasClaim());
		if (!player && !starting)
		{
			if (slot)
				return false;
			AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (control && control.GetAIAgent())
			{
				SCR_AIGroup group = SCR_AIGroup.Cast(control.GetAIAgent().GetParentGroup());
				if (group && group.IsPlayable())
					return false;
			}
		}
		m_aRecordedDeaths.Insert(entity);
		if (starting)
		{
			m_mLiving.Remove(entity);
			starting.m_iDeaths++;
		}
		m_Snapshot.RecordTotalDeath(character.GetFactionKey(), player);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Deletion releases weak entity keys without changing death totals or the starting denominator.
	void ForgetEntity(IEntity entity)
	{
		if (!entity)
			return;
		m_mLiving.Remove(entity);
		m_aRecordedDeaths.RemoveItem(entity);
	}
}
