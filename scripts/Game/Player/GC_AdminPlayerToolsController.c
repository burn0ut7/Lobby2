//! Teleport commands execute once on the character owner; authority observes completion.
modded class GC_LobbyPlayerController
{
	protected bool m_bGC_AdminTeleportPending;
	protected int m_iGC_TeleportRequester;
	protected UUID m_GC_TeleportRequesterIdentity;
	protected int m_iGC_TeleportRequest;
	protected int m_iGC_TeleportChecks;
	protected RplId m_GC_TeleportCharacter;
	protected vector m_vGC_TeleportPosition;
	protected string m_sGC_TeleportContext;
	protected bool m_bGC_TeleportFaction;
	protected int m_iGC_FactionRequest = -1;
	protected ref array<int> m_aGC_FactionPending = {};
	protected int m_iGC_FactionVerified;
	protected int m_iGC_FactionSkipped;
	protected int m_iGC_FactionUnverified;
	protected string m_sGC_FactionContext;

	//------------------------------------------------------------------------------------------------
	void GC_RequestFactionTeleport(string factionKey, int toId, string toIdentity, int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAskFactionTeleport, factionKey, toId, toIdentity, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskFactionTeleport(string factionKey, int toId, string toIdentity, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string result = "Admin tools are unavailable in this mode.";
		if (manager)
			result = manager.GC_ExecuteFactionTeleport(GetPlayerId(), factionKey, toId, toIdentity, requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "teleport-faction", factionKey, "rejected-inactive-mode");
		if (!result.IsEmpty())
			GC_SendPlayerToolResult(requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_HasFactionTeleport()
	{
		return m_iGC_FactionRequest >= 0;
	}

	//------------------------------------------------------------------------------------------------
	void GC_StartFactionTeleport(int requestId, array<int> players, int skipped, string context)
	{
		m_iGC_FactionRequest = requestId;
		m_aGC_FactionPending.Copy(players);
		m_iGC_FactionVerified = 0;
		m_iGC_FactionSkipped = skipped;
		m_iGC_FactionUnverified = 0;
		m_sGC_FactionContext = context;
		GetGame().GetCallqueue().CallLater(GC_FinishFactionTeleport, 7000);
	}

	//------------------------------------------------------------------------------------------------
	void GC_RecordFactionTeleport(int requestId, int playerId, bool verified)
	{
		if (!Replication.IsServer() || requestId != m_iGC_FactionRequest || !m_aGC_FactionPending.Contains(playerId))
			return;
		m_aGC_FactionPending.RemoveItem(playerId);
		if (verified)
			m_iGC_FactionVerified++;
		else
			m_iGC_FactionUnverified++;
		if (m_aGC_FactionPending.IsEmpty())
			GC_FinishFactionTeleport();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_FinishFactionTeleport()
	{
		GetGame().GetCallqueue().Remove(GC_FinishFactionTeleport);
		if (m_iGC_FactionRequest < 0)
			return;
		string result = string.Format("Teleported %1 player(s). Skipped %2. Unverified %3; check their positions before retrying.",
			m_iGC_FactionVerified, m_iGC_FactionSkipped, m_iGC_FactionUnverified + m_aGC_FactionPending.Count());
		GC_AdminActionLogger.Log(GetPlayerId(), "teleport-faction-result", m_sGC_FactionContext, result);
		GC_SendPlayerToolResult(m_iGC_FactionRequest, result);
		m_iGC_FactionRequest = -1;
		m_aGC_FactionPending.Clear();
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GetGame().GetCallqueue().Remove(GC_FinishFactionTeleport);
		if (m_iGC_FactionRequest >= 0)
			GC_AdminActionLogger.Log(GetPlayerId(), "teleport-faction-result", m_sGC_FactionContext, "cancelled-controller-shutdown");
		m_iGC_FactionRequest = -1;
		m_aGC_FactionPending.Clear();
		GetGame().GetCallqueue().Remove(GC_ObserveAdminTeleport);
		if (m_bGC_AdminTeleportPending)
		{
			GC_AdminActionLogger.Log(m_iGC_TeleportRequester, "teleport-result",
				m_sGC_TeleportContext, "cancelled-controller-shutdown");
			m_bGC_AdminTeleportPending = false;
		}
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestAdminTeleport(int fromId, string fromIdentity, int toId, string toIdentity, int requestId)
	{
		if (GC_LobbyManager.GetManager() && GetGame().GetPlayerController() == this)
			Rpc(GC_RpcAskAdminTeleport, fromId, fromIdentity, toId, toIdentity, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskAdminTeleport(int fromId, string fromIdentity, int toId, string toIdentity, int requestId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string result = "Admin tools are unavailable in this mode.";
		if (manager)
			result = manager.GC_ExecuteAdminTeleport(GetPlayerId(), fromId, fromIdentity, toId, toIdentity, requestId);
		else
			GC_AdminActionLogger.Log(GetPlayerId(), "teleport", fromId.ToString(), "rejected-inactive-mode");
		if (!result.IsEmpty())
			GC_SendPlayerToolResult(requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendPlayerToolResult(int requestId, string result)
	{
		if (Replication.IsServer())
			Rpc(RpcDo_AdminPlayerToolResult, requestId, result);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_BeginAdminTeleport(int requesterId, UUID requesterIdentity, int requestId, IEntity character, vector position, string context, bool faction = false)
	{
		if (!Replication.IsServer() || !GC_LobbyManager.GetManager() || m_bGC_AdminTeleportPending
			|| !SCR_Global.IsAdmin(requesterId) || GetControlledEntity() != character)
			return false;
		RplId id = Replication.FindItemId(character);
		if (id == RplId.Invalid())
			return false;
		m_bGC_AdminTeleportPending = true;
		m_iGC_TeleportRequester = requesterId;
		m_GC_TeleportRequesterIdentity = requesterIdentity;
		m_iGC_TeleportRequest = requestId;
		m_iGC_TeleportChecks = 0;
		m_GC_TeleportCharacter = id;
		m_vGC_TeleportPosition = position;
		m_sGC_TeleportContext = context;
		m_bGC_TeleportFaction = faction;
		Rpc(GC_RpcDoAdminTeleport, id, position);
		GetGame().GetCallqueue().CallLater(GC_ObserveAdminTeleport, 500);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoAdminTeleport(RplId expectedCharacter, vector position)
	{
		IEntity character = GetControlledEntity();
		if (!GC_LobbyManager.GetManager() || GetGame().GetPlayerController() != this || !character
			|| character.GetRootParent() != character || Replication.FindItemId(character) != expectedCharacter)
			return;
		SCR_Global.TeleportPlayer(GetPlayerId(), position);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ObserveAdminTeleport()
	{
		if (!Replication.IsServer() || !m_bGC_AdminTeleportPending)
			return;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity character = GetControlledEntity();
		string outcome;
		bool verified;
		if (!manager || !SCR_Global.IsAdmin(m_iGC_TeleportRequester)
			|| manager.GetPlayerVoiceIdentity(m_iGC_TeleportRequester) != m_GC_TeleportRequesterIdentity)
			outcome = "Teleport verification ended: admin session changed.";
		else if (!character || Replication.FindItemId(character) != m_GC_TeleportCharacter)
			outcome = "Teleport not verified: moving character changed.";
		else if (vector.DistanceSq(character.GetOrigin(), m_vGC_TeleportPosition) <= 9)
		{
			outcome = "Player teleported to the destination.";
			verified = true;
		}
		else if (++m_iGC_TeleportChecks >= 10)
			outcome = "Teleport could not be verified. Check the player position before retrying.";
		else
		{
			GetGame().GetCallqueue().CallLater(GC_ObserveAdminTeleport, 500);
			return;
		}
		m_bGC_AdminTeleportPending = false;
		GC_AdminActionLogger.Log(m_iGC_TeleportRequester, "teleport-result", m_sGC_TeleportContext, outcome);
		if (!manager || manager.GetPlayerVoiceIdentity(m_iGC_TeleportRequester) != m_GC_TeleportRequesterIdentity)
			return;
		GC_LobbyPlayerController requester = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(m_iGC_TeleportRequester));
		if (requester)
		{
			if (m_bGC_TeleportFaction)
				requester.GC_RecordFactionTeleport(m_iGC_TeleportRequest, GetPlayerId(), verified);
			else
				requester.GC_SendPlayerToolResult(m_iGC_TeleportRequest, outcome);
		}
	}
}
