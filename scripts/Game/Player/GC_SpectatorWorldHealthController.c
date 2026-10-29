//! Only characters represented by this client's visible markers need world health.
modded class GC_LobbyPlayerController
{
	protected int m_iGC_WorldHealthEpoch;
	protected int m_iGC_ServerWorldHealthEpoch;
	protected ref set<RplId> m_aGC_ObservedHealth = new set<RplId>();
	protected ref set<RplId> m_aGC_RequestedHealth = new set<RplId>();
	protected ref map<RplId, int> m_mGC_WorldHealth = new map<RplId, int>();
	protected ref array<RplId> m_aGC_ServerWorldEntities = {};
	protected ref map<RplId, int> m_mGC_ServerWorldValues = new map<RplId, int>();

	int GC_ObserveWorldHealth(IEntity entity)
	{
		if (!m_bGC_WorldHealth || !entity || entity.FindComponent(GC_LobbySpectatorBodyComponent))
			return -1;
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid())
			return -1;
		if (m_aGC_ObservedHealth.Count() < 1024)
			m_aGC_ObservedHealth.Insert(id);
		int value;
		if (m_mGC_WorldHealth.Find(id, value))
			return value;
		return -1;
	}

	override void GC_ToggleWorldHealth()
	{
		super.GC_ToggleWorldHealth();
		m_iGC_WorldHealthEpoch++;
		GetGame().GetCallqueue().Remove(GC_RequestWorldHealth);
		m_aGC_ObservedHealth.Clear();
		m_aGC_RequestedHealth.Clear();
		m_mGC_WorldHealth.Clear();
		if (m_bGC_WorldHealth)
			GetGame().GetCallqueue().CallLater(GC_RequestWorldHealth, 1000, true);
	}

	override void GC_ClearSpectatorEventCache()
	{
		super.GC_ClearSpectatorEventCache();
		m_iGC_WorldHealthEpoch++;
		GetGame().GetCallqueue().Remove(GC_RequestWorldHealth);
		m_aGC_ObservedHealth.Clear();
		m_aGC_RequestedHealth.Clear();
		m_mGC_WorldHealth.Clear();
		m_aGC_ServerWorldEntities.Clear();
		m_mGC_ServerWorldValues.Clear();
	}

	protected void GC_RequestWorldHealth()
	{
		if (!GC_CanJumpSpectatorCamera() || !m_bGC_WorldHealth)
			return;
		bool same = m_aGC_ObservedHealth.Count() == m_aGC_RequestedHealth.Count();
		foreach (RplId id : m_aGC_ObservedHealth)
		{
			if (!m_aGC_RequestedHealth.Contains(id))
				same = false;
		}
		if (!same)
		{
			array<RplId> ids = {};
			m_aGC_RequestedHealth.Clear();
			foreach (RplId observed : m_aGC_ObservedHealth)
			{
				ids.Insert(observed);
				m_aGC_RequestedHealth.Insert(observed);
			}
			Rpc(GC_RpcAsk_WorldHealth, m_iGC_WorldHealthEpoch, ids);
			array<RplId> stale = {};
			foreach (RplId cached, int value : m_mGC_WorldHealth)
			{
				if (!m_aGC_RequestedHealth.Contains(cached))
					stale.Insert(cached);
			}
			foreach (RplId remove : stale)
				m_mGC_WorldHealth.Remove(remove);
		}
		m_aGC_ObservedHealth.Clear();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_WorldHealth(int epoch, array<RplId> ids)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR || !m_bGC_ServerWorldHealth || ids.Count() > 1024)
			return;
		if (epoch != m_iGC_ServerWorldHealthEpoch)
			m_mGC_ServerWorldValues.Clear();
		m_iGC_ServerWorldHealthEpoch = epoch;
		m_aGC_ServerWorldEntities.Copy(ids);
	}

	override protected void GC_UpdateFocusFeed()
	{
		super.GC_UpdateFocusFeed();
		if (!m_bGC_ServerWorldHealth || !GC_LobbyManager.GetManager() || GetLobbyPlayerState() != GC_ELobbyPlayerState.SPECTATOR)
		{
			m_aGC_ServerWorldEntities.Clear();
			m_mGC_ServerWorldValues.Clear();
			return;
		}
		array<RplId> changed = {};
		array<int> values = {};
		map<RplId, int> current = new map<RplId, int>();
		foreach (RplId id : m_aGC_ServerWorldEntities)
		{
			IEntity entity = IEntity.Cast(Replication.FindItem(id));
			int value = GC_ReadCharacterHealth(entity);
			current.Set(id, value);
			int previous;
			if (!m_mGC_ServerWorldValues.Find(id, previous) || previous != value)
			{
				changed.Insert(id);
				values.Insert(value);
			}
		}
		m_mGC_ServerWorldValues = current;
		if (!changed.IsEmpty())
			Rpc(GC_RpcDo_WorldHealth, m_iGC_ServerWorldHealthEpoch, changed, values);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_WorldHealth(int epoch, array<RplId> ids, array<int> values)
	{
		if (!GC_CanJumpSpectatorCamera() || !m_bGC_WorldHealth || ids.Count() != values.Count() || epoch != m_iGC_WorldHealthEpoch)
			return;
		foreach (int i, RplId id : ids)
		{
			if (m_aGC_RequestedHealth.Contains(id))
				m_mGC_WorldHealth.Set(id, values[i]);
		}
	}
}
