//! Lobby2-owned controller transport; clients cannot choose the requesting player.
modded class GC_LobbyPlayerController
{
	ref ScriptInvoker m_GC_OnBoundaryResult = new ScriptInvoker();
	protected bool m_bGC_BoundaryMap;
	protected Widget m_GC_AnnouncementHud;
	protected float m_fGC_AnnouncementDeadline;
	protected float m_fGC_AnnouncementUpdated;

	//------------------------------------------------------------------------------------------------
	void GC_RequestBoundary(string action, string key, string title, int seconds, array<string> factions, int requestId)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		if (Replication.IsServer())
			GC_RpcAskBoundary(action, key, title, seconds, factions, requestId);
		else
			Rpc(GC_RpcAskBoundary, action, key, title, seconds, factions, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskBoundary(string action, string key, string title, int seconds, array<string> factions, int requestId)
	{
		if (!Replication.IsServer())
			return;
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		string result, snapshot;
		if (mode && mode.GetBoundaryAdmin())
		{
			result = mode.GetBoundaryAdmin().Execute(GetPlayerId(), action, key, title, seconds, factions);
			snapshot = mode.GetBoundaryAdmin().Snapshot(GetPlayerId());
		}
		else
		{
			result = "Boundary controls unavailable in this mode.";
			GC_AdminActionLogger.Log(GetPlayerId(), "boundary-" + action, key, "rejected-inactive-mode");
		}
		if (GetGame().GetPlayerController() == this)
			GC_RpcDoBoundaryResult(requestId, result, snapshot);
		else
			Rpc(GC_RpcDoBoundaryResult, requestId, result, snapshot);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoBoundaryResult(int requestId, string result, string snapshot)
	{
		if (GetGame().GetPlayerController() == this && GC_LobbyGameMode.GetGameMode())
			m_GC_OnBoundaryResult.Invoke(requestId, result, snapshot);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SetBoundaryMap(bool visible)
	{
		if (Replication.IsServer())
			m_bGC_BoundaryMap = visible && SCR_Global.IsAdmin(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	bool GC_ShowBoundaryMap()
	{
		return m_bGC_BoundaryMap && SCR_Global.IsAdmin(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendAnnouncement(string title, float seconds)
	{
		if (!Replication.IsServer())
			return;
		if (GetGame().GetPlayerController() == this)
			GC_RpcDoAnnouncement(title, seconds);
		else
			Rpc(GC_RpcDoAnnouncement, title, seconds);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_AnnouncementActive()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		float now = GetGame().GetWorld().GetWorldTime();
		return mode && mode.GetState() == SCR_EGameModeState.GAME
			&& now < m_fGC_AnnouncementDeadline && now - m_fGC_AnnouncementUpdated < 1500;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoAnnouncement(string title, float seconds)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		m_fGC_AnnouncementUpdated = GetGame().GetWorld().GetWorldTime();
		m_fGC_AnnouncementDeadline = m_fGC_AnnouncementUpdated + seconds * 1000;
		if (!GC_AnnouncementActive())
		{
			GC_CloseAnnouncement();
			return;
		}
		if (!m_GC_AnnouncementHud)
		{
			m_GC_AnnouncementHud = GetGame().GetWorkspace().CreateWidgets("{79C8E2442DF142AB}UI/layouts/Mission/Lobby_StagingCountdown.layout");
			if (!m_GC_AnnouncementHud)
				return;
			m_GC_AnnouncementHud.SetName("LobbyAnnouncementCountdown");
			GetGame().GetCallqueue().CallLater(GC_RefreshAnnouncement, 100, true);
		}
		TextWidget heading = TextWidget.Cast(m_GC_AnnouncementHud.FindAnyWidget("StagingCountdownTitle"));
		heading.SetText(title);
		float textWidth, textHeight;
		heading.GetTextSize(textWidth, textHeight);
		FrameSlot.SetSizeX(m_GC_AnnouncementHud.FindAnyWidget("StagingCountdownCard"), Math.Max(324, textWidth + 140));
		FrameSlot.SetAnchorMin(heading, 0, 0);
		FrameSlot.SetAnchorMax(heading, 0, 1);
		FrameSlot.SetSizeX(heading, textWidth + 4);
		GC_RefreshAnnouncement();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RefreshAnnouncement()
	{
		if (!GC_AnnouncementActive() || !m_GC_AnnouncementHud)
		{
			GC_CloseAnnouncement();
			return;
		}
		int remaining = Math.Ceil((m_fGC_AnnouncementDeadline - GetGame().GetWorld().GetWorldTime()) * 0.001);
		string seconds = (remaining % 60).ToString();
		if (remaining % 60 < 10)
			seconds = "0" + seconds;
		TextWidget.Cast(m_GC_AnnouncementHud.FindAnyWidget("StagingCountdownTime")).SetText((remaining / 60).ToString() + ":" + seconds);
		float top = 28;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(this);
		if (controller.GC_SetupActive())
			top += 52;
		FrameSlot.SetPos(m_GC_AnnouncementHud.FindAnyWidget("StagingCountdownCard"), 0, top);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_CloseAnnouncement()
	{
		GetGame().GetCallqueue().Remove(GC_RefreshAnnouncement);
		if (m_GC_AnnouncementHud)
			m_GC_AnnouncementHud.RemoveFromHierarchy();
		m_GC_AnnouncementHud = null;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_CloseAnnouncement();
		m_bGC_BoundaryMap = false;
		m_fGC_AnnouncementDeadline = 0;
		super.ShutdownLobbyPresentation();
	}
}
