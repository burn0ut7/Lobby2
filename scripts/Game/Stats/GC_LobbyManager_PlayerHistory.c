//! Extends only GC Lobby's own manager. No base-game behavior or player ownership changes.
modded class GC_LobbyManager
{
	protected ref GC_PlayerHistory m_GC_PlayerHistory;
	protected ref map<int, string> m_mGC_NewPlayers = new map<int, string>();

	//------------------------------------------------------------------------------------------------
	bool IsNewPlayer(int playerId)
	{
		if (GetManager() != this || m_bShutdown || playerId <= 0)
			return false;
		string identity;
		return m_mGC_NewPlayers.Find(playerId, identity)
			&& GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			&& GetPlayerVoiceIdentity(playerId) == identity;
	}

	//------------------------------------------------------------------------------------------------
	void PublishNewPlayer(int playerId, string identity, bool isNew)
	{
		if (!Replication.IsServer() || GetManager() != this || m_bShutdown)
			return;
		if (isNew && (playerId <= 0 || identity.IsEmpty()
			|| !GetGame().GetPlayerManager().IsPlayerConnected(playerId)
			|| SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId) != identity))
			return;
		string existing;
		bool present = m_mGC_NewPlayers.Find(playerId, existing);
		if ((!isNew && !present) || (isNew && present && existing == identity))
			return;
		GC_ApplyNewPlayer(playerId, identity, isNew);
		Rpc(GC_RpcDo_NewPlayer, playerId, identity, isNew);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ApplyNewPlayer(int playerId, string identity, bool isNew)
	{
		if (isNew)
			m_mGC_NewPlayers.Set(playerId, identity);
		else
			m_mGC_NewPlayers.Remove(playerId);
		m_OnPlayerRoleChanged.Invoke(playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void GC_RpcDo_NewPlayer(int playerId, string identity, bool isNew)
	{
		if (!Replication.IsServer() && GetManager() == this && !m_bShutdown)
			GC_ApplyNewPlayer(playerId, identity, isNew);
	}

	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (Replication.IsServer() && GetManager() == this && mode && mode.IsCollectionEnabled())
		{
			m_GC_PlayerHistory = new GC_PlayerHistory();
			m_GC_PlayerHistory.Start(this);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PublishNewPlayer(playerId, string.Empty, false);
		super.OnPlayerDisconnected(playerId, cause, timeout);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;
		writer.WriteInt(m_mGC_NewPlayers.Count());
		foreach (int playerId, string identity : m_mGC_NewPlayers)
		{
			writer.WriteInt(playerId);
			writer.WriteString(identity);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;
		int count;
		if (!reader.ReadInt(count) || count < 0 || count > 256)
			return false;
		m_mGC_NewPlayers.Clear();
		for (int index; index < count; index++)
		{
			int playerId;
			string identity;
			if (!reader.ReadInt(playerId) || !reader.ReadString(identity))
				return false;
			GC_ApplyNewPlayer(playerId, identity, true);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		if (m_GC_PlayerHistory)
			m_GC_PlayerHistory.Stop();
		m_GC_PlayerHistory = null;
		m_mGC_NewPlayers.Clear();
		super.OnGameEnd();
	}
}
