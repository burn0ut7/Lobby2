//! Owns one in-flight native ban-service operation for this Lobby2 lifetime.
//! Native persistence is authoritative; no local replacement ban database exists.
class GC_AdminBanService
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_RequesterController;
	protected BanServiceApi m_Api;
	protected ref StateBackendCallback m_Callback;
	protected ref BanListPageParams m_PageParams;
	protected int m_iRequester;
	protected int m_iRequestId;
	protected UUID m_sRequesterIdentity;
	protected int m_iTarget;
	protected UUID m_sTargetIdentity;
	protected string m_sOperation;
	protected int m_iSequence;
	protected bool m_bStopped;
	protected string m_sReason;
	protected int m_iExpiresAt;

	//------------------------------------------------------------------------------------------------
	void GC_AdminBanService(GC_LobbyManager manager)
	{
		m_Manager = manager;
	}

	//------------------------------------------------------------------------------------------------
	string Request(int requesterId, string operation, int targetId, UUID identity, string reason, int duration, int page, int requestId)
	{
		if (m_bStopped || !m_Manager || GC_LobbyManager.GetManager() != m_Manager || !Replication.IsServer())
			return "rejected-inactive-mode";
		if (m_Callback)
			return "rejected-ban-service-busy";
		if (RplSession.Mode() != RplMode.Dedicated)
			return "unsupported-ban-dedicated-only";
		BackendApi backend = GetGame().GetBackendApi();
		if (!backend)
			return "unavailable-ban-service";
		m_Api = backend.GetBanServiceApi();
		if (!m_Api)
			return "unavailable-ban-service";

		GC_LobbyPlayerController requester = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (!requester)
			return "rejected-stale-requester";
		m_RequesterController = requester;
		m_iRequester = requesterId;
		m_iRequestId = requestId;
		m_sRequesterIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(requesterId);
		m_iTarget = targetId;
		m_sTargetIdentity = identity;
		m_sOperation = operation;
		m_sReason = reason;
		m_iExpiresAt = 0;
		if (duration > 0)
			m_iExpiresAt = System.GetUnixTime() + duration;
		m_iSequence++;
		m_Callback = new StateBackendCallback();
		bool submitted;
		if (operation == "player-ban")
			submitted = m_Api.CreateBanIdentityId(m_Callback, identity, reason, duration);
		else if (operation == "player-unban")
		{
			array<string> identities = {identity};
			submitted = m_Api.RemoveBans(m_Callback, identities);
		}
		else if (operation == "ban-list")
		{
			m_PageParams = new BanListPageParams();
			m_PageParams.limit = 10;
			m_PageParams.offset = page * 10;
			submitted = m_Api.RequestServerBanList(m_Callback, m_PageParams);
		}
		if (!submitted)
		{
			m_Callback = null;
			m_PageParams = null;
			return "failed-ban-submission";
		}
		GetGame().GetCallqueue().CallLater(Poll, 250, true);
		return string.Format("pending-ban-operation=%1", m_iSequence);
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (m_bStopped || !m_Manager || GC_LobbyManager.GetManager() != m_Manager || !Replication.IsServer())
		{
			Shutdown();
			return;
		}
		if (!m_Callback || m_Callback.m_eState == EBackendCallbackState.EBCS_PENDING)
			return;
		GetGame().GetCallqueue().Remove(Poll);
		string outcome = "failed-ban-service";
		if (m_Callback.m_eState == EBackendCallbackState.EBCS_TIMEOUT)
			outcome = "uncertain-ban-timeout";
		else if (m_Callback.m_eState == EBackendCallbackState.EBCS_SUCCESS)
			outcome = "applied";
		if (outcome == "applied" && m_sOperation == "player-ban")
			m_Manager.RecordModerationAction(m_sTargetIdentity, "Banned", m_sReason, m_iExpiresAt);
		else if (outcome == "applied" && m_sOperation == "player-unban")
			m_Manager.RecordModerationAction(m_sTargetIdentity, "Unbanned", "", 0);

		// Request IDs belong to a controller lifetime. An account reconnecting
		// must not receive a previous session's delayed result with a reused ID.
		bool requesterCurrent = m_RequesterController
			&& GetGame().GetPlayerManager().GetPlayerController(m_iRequester) == m_RequesterController
			&& GetGame().GetPlayerManager().IsPlayerConnected(m_iRequester)
			&& SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iRequester) == m_sRequesterIdentity
			&& SCR_Global.IsAdmin(m_iRequester);
		if (outcome == "applied" && m_sOperation == "player-ban" && requesterCurrent)
		{
			// Re-resolve the banned account, including manually entered identities that connected meanwhile.
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int playerId : players)
			{
				if (SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId) == m_sTargetIdentity)
					GetGame().GetPlayerManager().KickPlayer(playerId, PlayerManagerKickReason.BAN);
			}
		}
		GC_AdminActionLogger.Log(m_iRequester, m_sOperation,
			string.Format("operation=%1 actorIdentity=%2 identity=%3 request=%4", m_iSequence, m_sRequesterIdentity, m_sTargetIdentity, m_iRequestId),
			string.Format("%1 apiCode=%2 currentAdmin=%3", outcome, m_Callback.m_eLastError, requesterCurrent), m_iTarget, m_sTargetIdentity);
		GC_LobbyPlayerController controller;
		if (requesterCurrent)
			controller = m_RequesterController;
		if (controller)
		{
			if (outcome == "applied" && m_sOperation == "ban-list")
				SendBanPage(controller);
			controller.SendAdminActionResult(outcome, m_iRequestId);
		}
		m_Callback = null;
		m_PageParams = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void SendBanPage(GC_LobbyPlayerController controller)
	{
		array<OnlineBanListData> bans = {};
		m_Api.GetPageItems(bans);
		array<string> identities = {};
		array<string> labels = {};
		foreach (OnlineBanListData ban : bans)
		{
			identities.Insert(ban.identityId);
			labels.Insert(string.Format("%1 / %2 / %3 / expires: %4", ban.bannedName, ban.identityId, ban.reason, ban.expiresAt));
		}
		controller.SendAdminBanPage(identities, labels, m_PageParams.offset / 10, m_Api.GetTotalItemCount(), m_iRequestId);
	}

	//------------------------------------------------------------------------------------------------
	void Shutdown()
	{
		if (m_bStopped)
			return;
		m_bStopped = true;
		GetGame().GetCallqueue().Remove(Poll);
		if (m_Callback)
			GC_AdminActionLogger.Log(m_iRequester, m_sOperation, string.Format("operation=%1 identity=%2", m_iSequence, m_sTargetIdentity), "unobserved-after-mode-exit");
		// The native API exposes no cancellation operation. Do not claim cancellation.
		m_RequesterController = null;
		m_Manager = null;
	}
}
