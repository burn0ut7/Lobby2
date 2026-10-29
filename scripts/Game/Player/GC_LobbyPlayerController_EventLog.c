modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_OnGC_SpectatorEvents = new ScriptInvoker();
	protected ref array<string> m_aGC_LiveEvents = {};
	protected ref array<string> m_aGC_PersonalEvents = {};
	protected bool m_bGC_EventFeedRequested;
	protected bool m_bGC_LiveEventsLoaded;
	protected bool m_bGC_PersonalEventsLoaded;
	protected bool m_bGC_PersonalEventsPending;
	protected int m_iGC_EventEpoch;
	protected int m_iGC_PersonalNext;
	protected int m_iGC_PersonalBefore;
	protected string m_sGC_LiveEventStatus;
	protected string m_sGC_PersonalEventStatus;
	protected GC_SessionLogComponent m_GC_EventSource;
	protected int m_iGC_ServerEventEpoch = -1;
	protected int m_iGC_ServerEventRevision = -1;
	protected int m_iGC_ServerPersonalCutoff;
	protected int m_iGC_ServerPersonalNext = -1;
	protected ref array<string> m_aGC_ServerLiveEvents = {};

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_ClearSpectatorEventCache();
		super.ShutdownLobbyPresentation();
	}

	override protected void SetLobbyPlayerState(GC_ELobbyPlayerState state)
	{
		bool changed = GetLobbyPlayerState() != state;
		super.SetLobbyPlayerState(state);
		if (changed && state != GC_ELobbyPlayerState.SPECTATOR)
			GC_ClearSpectatorEventCache();
	}

	//------------------------------------------------------------------------------------------------
	//! Subscribe once per spectator period, independently of panel visibility or tab changes.
	void GC_EnsureSpectatorEvents(bool everyone)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this
			|| GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return;
		if (!m_bGC_EventFeedRequested)
		{
			m_bGC_EventFeedRequested = true;
			Rpc(GC_RpcAsk_SubscribeSpectatorEvents, m_iGC_EventEpoch);
		}
		if (!everyone && !m_bGC_PersonalEventsLoaded)
			GC_LoadPersonalEventPage();
	}

	void GC_LoadPersonalEventPage()
	{
		if (!m_bGC_EventFeedRequested || m_bGC_PersonalEventsPending
			|| (m_bGC_PersonalEventsLoaded && m_iGC_PersonalNext == 0)
			|| GetGame().GetPlayerController() != this || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return;
		m_bGC_PersonalEventsPending = true;
		m_iGC_PersonalBefore = m_iGC_PersonalNext;
		Rpc(GC_RpcAsk_PersonalEvents, m_iGC_EventEpoch, m_iGC_PersonalBefore);
	}

	array<string> GC_GetCachedSpectatorEvents(bool everyone)
	{
		if (everyone)
			return m_aGC_LiveEvents;
		return m_aGC_PersonalEvents;
	}

	bool GC_AreSpectatorEventsLoaded(bool everyone)
	{
		if (everyone)
			return m_bGC_LiveEventsLoaded;
		return m_bGC_PersonalEventsLoaded;
	}

	int GC_GetPersonalEventNext()
	{
		return m_iGC_PersonalNext;
	}

	string GC_GetSpectatorEventStatus(bool everyone)
	{
		if (!GC_AreSpectatorEventsLoaded(everyone))
			return "Loading...";
		if (everyone)
			return m_sGC_LiveEventStatus;
		return m_sGC_PersonalEventStatus;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_StopServerEventFeed()
	{
		if (m_GC_EventSource)
			m_GC_EventSource.m_OnSpectatorCombat.Remove(GC_PushSpectatorEvents);
		m_GC_EventSource = null;
		m_iGC_ServerEventEpoch = -1;
		m_iGC_ServerEventRevision = -1;
		m_iGC_ServerPersonalNext = -1;
		m_aGC_ServerLiveEvents.Clear();
	}

	void GC_ClearSpectatorEventCache()
	{
		GC_StopServerEventFeed();
		m_iGC_EventEpoch++;
		m_bGC_EventFeedRequested = false;
		m_bGC_LiveEventsLoaded = false;
		m_bGC_PersonalEventsLoaded = false;
		m_bGC_PersonalEventsPending = false;
		m_iGC_PersonalNext = 0;
		m_aGC_LiveEvents.Clear();
		m_aGC_PersonalEvents.Clear();
		m_sGC_LiveEventStatus = string.Empty;
		m_sGC_PersonalEventStatus = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_SubscribeSpectatorEvents(int epoch)
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !logs || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR
			|| SCR_PlayerIdentityUtils.GetPlayerIdentityId(GetPlayerId()).IsEmpty())
			return;
		if (m_GC_EventSource == logs)
			return;
		GC_StopServerEventFeed();
		m_GC_EventSource = logs;
		m_iGC_ServerEventEpoch = epoch;
		m_iGC_ServerPersonalCutoff = logs.GetSpectatorRevision() + 1;
		logs.m_OnSpectatorCombat.Insert(GC_PushSpectatorEvents);
		GC_PushSpectatorEvents();
	}

	//------------------------------------------------------------------------------------------------
	//! At most one changed batch per second. No RPC at all for an unchanged feed.
	protected void GC_PushSpectatorEvents()
	{
		if (!Replication.IsServer() || !m_GC_EventSource)
			return;
		if (m_GC_EventSource != GC_SessionLogComponent.GetInstance() || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
		{
			GC_StopServerEventFeed();
			return;
		}
		array<string> rows;
		int next, revision;
		string status;
		bool unchanged;
		m_GC_EventSource.ReadSpectatorPage(GetPlayerId(), true, 0, rows, next, status, revision, unchanged, m_iGC_ServerEventRevision);
		if (revision < 0)
		{
			GC_StopServerEventFeed();
			return;
		}
		if (unchanged)
			return;
		array<string> changed = {};
		array<int> order = {};
		bool same = rows.Count() == m_aGC_ServerLiveEvents.Count();
		for (int i; i < rows.Count(); i += 7)
		{
			int sequence = rows[i].ToInt();
			order.Insert(sequence);
			int old = GC_FindEventRow(m_aGC_ServerLiveEvents, sequence);
			bool identical = old >= 0;
			for (int j; identical && j < 7; j++)
				identical = rows[i + j] == m_aGC_ServerLiveEvents[old + j];
			if (!identical)
			{
				for (int value; value < 7; value++)
					changed.Insert(rows[i + value]);
			}
			if (old != i || !identical)
				same = false;
		}
		bool initial = m_iGC_ServerEventRevision < 0;
		m_iGC_ServerEventRevision = revision;
		m_aGC_ServerLiveEvents.Copy(rows);
		if (initial || !same)
			Rpc(GC_RpcDo_SpectatorFeed, m_iGC_ServerEventEpoch, changed, order, status);
	}

	protected static int GC_FindEventRow(array<string> rows, int sequence)
	{
		for (int i; i < rows.Count(); i += 7)
		{
			if (rows[i].ToInt() == sequence)
				return i;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_SpectatorFeed(int epoch, array<string> changed, array<int> order, string status)
	{
		if (!GC_CanReceiveEvents(epoch) || changed.Count() % 7 != 0)
			return;
		array<string> merged = {};
		foreach (int sequence : order)
		{
			array<string> source = changed;
			int index = GC_FindEventRow(changed, sequence);
			if (index < 0)
			{
				source = m_aGC_LiveEvents;
				index = GC_FindEventRow(source, sequence);
			}
			if (index < 0)
				return;
			for (int i; i < 7; i++)
				merged.Insert(source[index + i]);
		}
		m_aGC_LiveEvents.Copy(merged);
		m_bGC_LiveEventsLoaded = true;
		m_sGC_LiveEventStatus = status;
		m_OnGC_SpectatorEvents.Invoke(true);
	}

	protected bool GC_CanReceiveEvents(int epoch)
	{
		return GC_LobbyGameMode.GetGameMode() && GetGame().GetPlayerController() == this
			&& GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR && m_bGC_EventFeedRequested && epoch == m_iGC_EventEpoch;
	}

	//------------------------------------------------------------------------------------------------
	//! Personal pages are fetched once, on demand, from the pre-subscription history.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_PersonalEvents(int epoch, int before)
	{
		if (!Replication.IsServer() || !m_GC_EventSource || m_GC_EventSource != GC_SessionLogComponent.GetInstance()
			|| GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR || epoch != m_iGC_ServerEventEpoch)
			return;
		if ((m_iGC_ServerPersonalNext == -1 && before != 0)
			|| (m_iGC_ServerPersonalNext != -1 && (before <= 0 || before != m_iGC_ServerPersonalNext)))
			return;
		int cursor = before;
		if (cursor == 0)
			cursor = m_iGC_ServerPersonalCutoff;
		array<string> rows;
		int next, revision;
		string status;
		bool unchanged;
		m_GC_EventSource.ReadSpectatorPage(GetPlayerId(), false, cursor, rows, next, status, revision, unchanged);
		m_iGC_ServerPersonalNext = next;
		Rpc(GC_RpcDo_PersonalEvents, epoch, before, rows, next, status);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_PersonalEvents(int epoch, int before, array<string> rows, int next, string status)
	{
		if (!GC_CanReceiveEvents(epoch) || !m_bGC_PersonalEventsPending || before != m_iGC_PersonalBefore || rows.Count() % 7 != 0)
			return;
		foreach (string value : rows)
			m_aGC_PersonalEvents.Insert(value);
		m_bGC_PersonalEventsPending = false;
		m_bGC_PersonalEventsLoaded = true;
		m_iGC_PersonalNext = next;
		m_sGC_PersonalEventStatus = status;
		m_OnGC_SpectatorEvents.Invoke(false);
	}
}
