//! The same immutable session events feed backend history, telemetry, spectator and debrief.
modded class GC_SessionLogComponent
{
	//------------------------------------------------------------------------------------------------
	void ReadDebriefCombat(string identity, int before, int filter, out array<string> rows, out int next, out string status)
	{
		rows = {};
		next = -1;
		status = "No events";
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || GetInstance() != this || !mode || mode.GetState() != SCR_EGameModeState.POSTGAME || identity.IsEmpty() || before < 0 || filter < 0 || filter > 4)
		{
			status = "Unavailable";
			return;
		}
		GC_SpectatorEventBuffer buffer = m_mPersonalEvents.Get(identity);
		if (!buffer)
			return;
		int last;
		for (int i; i < buffer.Count(); i++)
		{
			GC_SessionEvent entry = buffer.GetNewest(i);
			if (before > 0 && entry.m_iSequence >= before)
				continue;
			bool damage = entry.m_sType == "damage";
			bool incoming = entry.m_sTarget == identity;
			bool outgoing = entry.m_sActor == identity;
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
			if (incoming)
				label = "Death";
			if (damage)
				label = "Damage";
			if (entry.m_sType == "teamKill")
				label = "Team kill";
			rows.Insert(label);
			rows.Insert(GC_DebriefText.Actor(entry.m_sActorName, entry.m_sActorKind, entry.m_sActorPrefab));
			rows.Insert(GC_DebriefText.Actor(entry.m_sTargetName, entry.m_sTargetKind, entry.m_sTargetPrefab));
			string amount = "—";
			if (damage)
				amount = (Math.Round(entry.m_fDamage * 10) * 0.1).ToString();
			rows.Insert(amount);
			string detail = FilePath.StripExtension(FilePath.StripPath(entry.m_sWeapon));
			detail.Replace("_", " ");
			if (entry.m_fDistance >= 0)
			{
				if (!detail.IsEmpty())
					detail += " · ";
				detail += string.Format("%1 m", Math.Round(entry.m_fDistance));
			}
			if (!entry.m_sDeathCause.IsEmpty() && entry.m_sDeathCause != "other")
			{
				if (!detail.IsEmpty())
					detail += " · ";
				detail += entry.m_sDeathCause;
			}
			rows.Insert(GC_DebriefText.Short(detail));
			GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
			string actorFaction;
			string targetFaction;
			if (stats)
			{
				actorFaction = stats.GetReportFactionAt(entry.m_sActor, m_fStarted + entry.m_fSeconds);
				targetFaction = stats.GetReportFactionAt(entry.m_sTarget, m_fStarted + entry.m_fSeconds);
			}
			rows.Insert(actorFaction);
			rows.Insert(targetFaction);
		}
		if (!rows.IsEmpty())
			status = string.Empty;
		if (buffer.WasTruncated())
			status = "Recent 256 events";
	}
}
