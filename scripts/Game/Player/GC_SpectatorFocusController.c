//! Spectator selection is independent of admin selection and camera attachment.
modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_OnGC_SpectatorFocus = new ScriptInvoker();
	ref ScriptInvoker m_OnGC_SpectatorHealth = new ScriptInvoker();
	protected int m_iGC_FocusedPlayer;
	protected int m_iGC_FocusGeneration;
	protected bool m_bGC_WorldHealth;
	protected ref map<int, int> m_mGC_Health = new map<int, int>();
	protected ref array<string> m_aGC_FocusEvents = {};
	protected string m_sGC_FocusStatus;
	protected int m_iGC_FocusNext;
	protected bool m_bGC_FocusPagePending;
	protected int m_iGC_ServerFocus;
	protected string m_sGC_ServerFocusIdentity;
	protected int m_iGC_ServerFocusGeneration;
	protected ref map<int, int> m_mGC_ServerHealth = new map<int, int>();
	protected ref array<string> m_aGC_ServerFocusEvents = {};
	protected int m_iGC_ServerFocusRevision = -1;
	protected int m_iGC_ServerFocusNext;

	//------------------------------------------------------------------------------------------------
	int GC_GetFocusedPlayer()
	{
		return m_iGC_FocusedPlayer;
	}
	bool GC_AreWorldHealthBarsEnabled()
	{
		return m_bGC_WorldHealth;
	}
	array<string> GC_GetFocusEvents()
	{
		return m_aGC_FocusEvents;
	}
	string GC_GetFocusEventStatus()
	{
		return m_sGC_FocusStatus;
	}
	int GC_GetFocusEventNext()
	{
		return m_iGC_FocusNext;
	}

	//! Packed whole-percent health and life state. -1 means unavailable, not healthy.
	int GC_GetPlayerHealth(int playerId)
	{
		int value;
		if (m_mGC_Health.Find(playerId, value))
			return value;
		return -1;
	}

	static int GC_GetSpectatorEntityPlayer(IEntity entity)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !entity || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return 0;
		GC_Slot slot = manager.GetSlotByEntity(entity);
		if (slot)
			return slot.GetPlayerId();
		return GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
	}

	void GC_SelectSpectatorPlayer(int playerId)
	{
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || playerId < 0)
			return;
		if (m_iGC_FocusedPlayer != playerId)
		{
			m_iGC_FocusedPlayer = playerId;
			m_iGC_FocusGeneration++;
			m_aGC_FocusEvents.Clear();
			m_iGC_FocusNext = 0;
			m_bGC_FocusPagePending = false;
			m_sGC_FocusStatus = "Loading...";
			GC_SendFocusSettings();
			m_OnGC_SpectatorFocus.Invoke();
		}
		GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (menu && playerId > 0)
			menu.ShowFocusedPlayer();
	}

	void GC_ToggleWorldHealth()
	{
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera())
			return;
		m_bGC_WorldHealth = !m_bGC_WorldHealth;
		m_OnGC_SpectatorHealth.Invoke();
	}


	protected void GC_SendFocusSettings()
	{
		Rpc(GC_RpcAsk_FocusSettings, m_iGC_FocusGeneration, m_iGC_FocusedPlayer);
	}

	//------------------------------------------------------------------------------------------------
	override void GC_ClearSpectatorEventCache()
	{
		super.GC_ClearSpectatorEventCache();
		GC_StopFocusFeed();
		m_iGC_FocusGeneration++;
		m_iGC_FocusedPlayer = 0;
		m_bGC_WorldHealth = false;
		m_mGC_Health.Clear();
		m_aGC_FocusEvents.Clear();
		m_iGC_FocusNext = 0;
		m_bGC_FocusPagePending = false;
	}

	protected void GC_StopFocusFeed()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateFocusFeed);
		m_iGC_ServerFocus = 0;
		m_sGC_ServerFocusIdentity = string.Empty;
		m_mGC_ServerHealth.Clear();
		m_aGC_ServerFocusEvents.Clear();
		m_iGC_ServerFocusRevision = -1;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_FocusSettings(int generation, int playerId)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
			return;
		if (playerId < 0 || (playerId > 0 && !GetGame().GetPlayerManager().IsPlayerConnected(playerId)))
			return;
		bool changed = playerId != m_iGC_ServerFocus || generation != m_iGC_ServerFocusGeneration;
		m_iGC_ServerFocus = playerId;
		m_iGC_ServerFocusGeneration = generation;
		m_sGC_ServerFocusIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (changed)
		{
			m_mGC_ServerHealth.Clear();
			m_iGC_ServerFocusRevision = -1;
			m_iGC_ServerFocusNext = 0;
			m_aGC_ServerFocusEvents.Clear();
		}
		GetGame().GetCallqueue().Remove(GC_UpdateFocusFeed);
		GC_UpdateFocusFeed();
		if (playerId > 0)
			GetGame().GetCallqueue().CallLater(GC_UpdateFocusFeed, 1000, true);
	}

	//! Poll native health only for opted-in spectators. Changed integers are batched in one owner RPC.
	protected void GC_UpdateFocusFeed()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!Replication.IsServer() || !manager || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
		{
			GC_StopFocusFeed();
			return;
		}
		PlayerManager players = GetGame().GetPlayerManager();
		array<int> ids = {};
		if (m_iGC_ServerFocus > 0)
			ids.Insert(m_iGC_ServerFocus);
		array<int> changes = {};
		map<int, int> current = new map<int, int>();
		foreach (int id : ids)
		{
			int value = -1;
			IEntity entity = players.GetPlayerControlledEntity(id);
			GC_Slot slot = manager.GetSlotByPlayerId(id);
			if (slot && slot.GetState() == GC_SlotState.KIA)
			{
				entity = slot.GetEntity();
				if (players.IsPlayerConnected(id))
					value = 300;
			}
			if (players.IsPlayerConnected(id) && entity)
				value = GC_ReadCharacterHealth(entity);
			if (id == m_iGC_ServerFocus && SCR_PlayerIdentityUtils.GetPlayerIdentityId(id) != m_sGC_ServerFocusIdentity)
				value = -1;
			current.Set(id, value);
			int previous;
			if (!m_mGC_ServerHealth.Find(id, previous) || previous != value)
			{
				changes.Insert(id);
				changes.Insert(value);
			}
		}
		foreach (int oldId, int oldValue : m_mGC_ServerHealth)
		{
			if (!current.Contains(oldId))
			{
				changes.Insert(oldId);
				changes.Insert(-1);
			}
		}
		m_mGC_ServerHealth = current;
		if (!changes.IsEmpty())
			Rpc(GC_RpcDo_HealthChanges, m_iGC_ServerFocusGeneration, changes);
		GC_PushFocusedEvents();
	}

	static int GC_ReadCharacterHealth(IEntity entity)
	{
		if (!entity || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return -1;
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		SCR_CharacterControllerComponent character = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (!damage || !character)
			return -1;
		if (character.IsDead())
			return 300;
		int value = Math.Round(Math.Clamp(damage.GetHealthScaled(), 0, 1) * 100);
		if (character.IsUnconscious())
			value += 600;
		else if (damage.IsBleeding())
			value += 900;
		return value;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_HealthChanges(int generation, array<int> changes)
	{
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || generation != m_iGC_FocusGeneration || changes.Count() % 2 != 0)
			return;
		for (int i; i < changes.Count(); i += 2)
		{
			if (changes[i + 1] < 0)
				m_mGC_Health.Remove(changes[i]);
			else
				m_mGC_Health.Set(changes[i], changes[i + 1]);
		}
		m_OnGC_SpectatorHealth.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_PushFocusedEvents()
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!logs || m_iGC_ServerFocus <= 0)
			return;
		if (m_sGC_ServerFocusIdentity.IsEmpty() || SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iGC_ServerFocus) != m_sGC_ServerFocusIdentity)
			return;
		if (logs.GetSpectatorRevision() == m_iGC_ServerFocusRevision)
			return;
		array<string> rows;
		int next, revision;
		string status;
		bool unchanged;
		logs.ReadSpectatorPage(GetPlayerId(), false, 0, rows, next, status, revision, unchanged, -1, m_iGC_ServerFocus);
		bool same = m_iGC_ServerFocusRevision >= 0 && rows.Count() == m_aGC_ServerFocusEvents.Count();
		for (int i; same && i < rows.Count(); i++)
			same = rows[i] == m_aGC_ServerFocusEvents[i];
		m_iGC_ServerFocusRevision = revision;
		if (same)
			return;
		bool initial = m_aGC_ServerFocusEvents.IsEmpty();
		int previousHead;
		if (!initial)
			previousHead = m_aGC_ServerFocusEvents[0].ToInt();
		m_aGC_ServerFocusEvents.Copy(rows);
		if (initial)
			m_iGC_ServerFocusNext = next;
		// Catch up across bursts before publishing the new head. The personal ring
		// bounds this to 256 records; an old paging cursor must not leave a gap.
		int catchup = next;
		while (!initial && catchup > 0 && rows.Count() < 256 * 7 && rows[rows.Count() - 7].ToInt() > previousHead)
		{
			array<string> page;
			int pageNext, pageRevision;
			string pageStatus;
			bool pageUnchanged;
			logs.ReadSpectatorPage(GetPlayerId(), false, catchup, page, pageNext, pageStatus, pageRevision, pageUnchanged, -1, m_iGC_ServerFocus);
			if (page.IsEmpty())
				break;
			foreach (string value : page)
				rows.Insert(value);
			catchup = pageNext;
		}
		Rpc(GC_RpcDo_FocusEvents, m_iGC_ServerFocusGeneration, 0, rows, next, status);
	}

	void GC_LoadFocusedEventPage()
	{
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || m_iGC_FocusNext <= 0 || m_bGC_FocusPagePending || m_aGC_FocusEvents.Count() >= 256 * 7)
			return;
		m_bGC_FocusPagePending = true;
		Rpc(GC_RpcAsk_FocusPage, m_iGC_FocusGeneration, m_iGC_FocusNext);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_FocusPage(int generation, int before)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR
			|| generation != m_iGC_ServerFocusGeneration || before <= 0 || before != m_iGC_ServerFocusNext || m_iGC_ServerFocus <= 0)
			return;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!logs || SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iGC_ServerFocus) != m_sGC_ServerFocusIdentity)
			return;
		array<string> rows;
		int next, revision;
		string status;
		bool unchanged;
		logs.ReadSpectatorPage(GetPlayerId(), false, before, rows, next, status, revision, unchanged, -1, m_iGC_ServerFocus);
		m_iGC_ServerFocusNext = next;
		Rpc(GC_RpcDo_FocusEvents, generation, before, rows, next, status);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_FocusEvents(int generation, int before, array<string> rows, int next, string status)
	{
		if (!GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera() || generation != m_iGC_FocusGeneration || rows.Count() % 7 != 0)
			return;
		bool initial = m_aGC_FocusEvents.IsEmpty();
		if (before == 0)
		{
			array<string> merged = {};
			merged.Copy(rows);
			if (!rows.IsEmpty())
			{
				int oldest = rows[rows.Count() - 7].ToInt();
				for (int i; i < m_aGC_FocusEvents.Count() && merged.Count() < 256 * 7; i += 7)
				{
					if (m_aGC_FocusEvents[i].ToInt() >= oldest)
						continue;
					for (int j; j < 7; j++)
						merged.Insert(m_aGC_FocusEvents[i + j]);
				}
			}
			m_aGC_FocusEvents = merged;
		}
		else
		{
			for (int row; row < rows.Count() && m_aGC_FocusEvents.Count() < 256 * 7; row += 7)
			{
				if (GC_FindEventRow(m_aGC_FocusEvents, rows[row].ToInt()) >= 0)
					continue;
				for (int column; column < 7; column++)
					m_aGC_FocusEvents.Insert(rows[row + column]);
			}
		}
		if (initial || before > 0)
			m_iGC_FocusNext = next;
		if (before > 0)
			m_bGC_FocusPagePending = false;
		m_sGC_FocusStatus = status;
		if (!m_aGC_FocusEvents.IsEmpty() && status == "No events")
			m_sGC_FocusStatus = string.Empty;
		m_OnGC_SpectatorEvents.Invoke(false);
	}
}
