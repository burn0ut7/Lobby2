//! The same immutable session events feed backend history, telemetry, spectator and debrief.
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	void ReadDebriefCombat(string identity, int before, int filter, out array<string> rows, out int next, out string status, bool mission = false)
	{
		rows = {};
		next = -1;
		status = "No events";
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || GetInstance() != this || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME || (mission && (!identity.IsEmpty() || filter != 0)) || (!mission && identity.IsEmpty()) || before < 0 || filter < 0 || filter > 4)
		{
			status = "Unavailable";
			return;
		}
		GC_SpectatorEventBuffer buffer = m_SpectatorEvents;
		if (!mission)
			buffer = m_mPersonalEvents.Get(identity);
		if (!buffer)
			return;
		int last;
		for (int i; i < buffer.Count(); i++)
		{
			GC_SessionEvent entry = buffer.GetNewest(i);
			if (before > 0 && entry.m_iSequence >= before)
				continue;
			bool damage = entry.m_sType == "damage";
			bool incoming = !mission && entry.m_sTarget == identity;
			bool outgoing = !mission && entry.m_sActor == identity;
			if (filter == 1 && (damage || !outgoing || incoming))
				continue;
			if (filter == 2 && (damage || !incoming))
				continue;
			if (filter == 3 && (!damage || !incoming))
				continue;
			if (filter == 4 && (!damage || !outgoing))
				continue;
			if (rows.Count() == 48)
			{
				next = last;
				break;
			}
			last = entry.m_iSequence;
			rows.Insert(FormatEventTime(entry.m_fSeconds, false));
			string label = "Kill";
			if (incoming || entry.m_sType == "death")
				label = "Death";
			if (damage)
				label = "Damaged " + (Math.Round(entry.m_fDamage * 10) * 0.1).ToString();
			if (entry.m_sType == "teamKill")
				label = "Team kill";
			rows.Insert(label);
			GC_SessionEvent source = GC_DebriefText.CombatSource(entry);
			rows.Insert(GC_DebriefText.Actor(source.m_sActorName, source.m_sActorKind, source.m_sActorPrefab));
			rows.Insert(GC_DebriefText.Actor(entry.m_sTargetName, entry.m_sTargetKind, entry.m_sTargetPrefab));
			string amount = "—";
			if (damage)
				amount = (Math.Round(entry.m_fDamage * 10) * 0.1).ToString();
			rows.Insert(amount);
			rows.Insert(GC_DebriefText.CombatDetail(entry));
			GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
			string actorFaction = source.m_sActorFaction;
			string targetFaction = entry.m_sTargetFaction;
			if (stats)
			{
				if (actorFaction.IsEmpty())
					actorFaction = stats.GetReportFactionAt(source.m_sActor, m_fStarted + source.m_fSeconds);
				if (targetFaction.IsEmpty())
					targetFaction = stats.GetReportFactionAt(entry.m_sTarget, m_fStarted + entry.m_fSeconds);
			}
			rows.Insert(actorFaction);
			rows.Insert(targetFaction);
		}
		if (!rows.IsEmpty())
			status = string.Empty;
		if (buffer.WasTruncated())
		{
			status = "Recent 256 events";
			if (mission)
				status = "Recent 4096 events";
		}
	}
}
