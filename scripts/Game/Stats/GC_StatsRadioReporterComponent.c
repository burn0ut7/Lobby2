class GC_StatsRadioReporterComponentClass : ScriptComponentClass
{
}

//! Radio capture is local engine state. Report it in batches and label it client-reported.
class GC_StatsRadioReporterComponent : ScriptComponent
{
	protected float m_fSampleElapsed;
	protected float m_fReportElapsed;
	protected float m_fRadioSeconds;
	protected float m_fServerReported;
	protected float m_fServerReceivedAt;
	protected bool m_bSentFinal;
	protected SCR_VoNComponent m_Capture;
	protected IEntity m_CaptureBody;
	protected float m_fLocalUntil;
	protected float m_fRadioUntil;
	protected float m_fVoiceReportAt;
	protected float m_fVoiceReceivedAt = -1000;
	protected int m_iVoiceMask;

	//------------------------------------------------------------------------------------------------
	//! Observe capture only; never change voice routing or record audio.
	protected void OnVoiceCapture(BaseTransceiver transmitter)
	{
		float until = GetGame().GetWorld().GetWorldTime() + 500;
		if (transmitter)
			m_fRadioUntil = until;
		else
			m_fLocalUntil = until;
	}

	//------------------------------------------------------------------------------------------------
	protected void SampleVoice(GC_LobbyGameMode mode, PlayerController controller)
	{
		IEntity body;
		if (mode && mode.IsPlaybackTelemetryEnabled() && mode.GetState() == SCR_EGameModeState.GAME)
			body = controller.GetControlledEntity();
		if (body && body.FindComponent(GC_LobbySpectatorBodyComponent))
			body = null;
		if (body != m_CaptureBody)
		{
			if (m_Capture)
				m_Capture.m_OnCaptureVON.Remove(OnVoiceCapture);
			m_Capture = null;
			m_CaptureBody = body;
			m_fLocalUntil = 0;
			m_fRadioUntil = 0;
			m_iVoiceMask = 0;
			if (body)
				m_Capture = SCR_VoNComponent.Cast(body.FindComponent(SCR_VoNComponent));
			if (m_Capture)
				m_Capture.m_OnCaptureVON.Insert(OnVoiceCapture);
		}
		if (!body)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		int mask;
		if (now < m_fLocalUntil)
			mask |= 1;
		if (now < m_fRadioUntil)
			mask |= 2;
		if (mask == m_iVoiceMask && (mask == 0 || now - m_fVoiceReportAt < 500))
			return;
		if (now - m_fVoiceReportAt < 75)
			return;
		m_iVoiceMask = mask;
		m_fVoiceReportAt = now;
		Rpc(RpcAsk_Voice, Replication.FindItemId(body), mask);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Voice(RplId bodyId, int mask)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		PlayerController controller = PlayerController.Cast(GetOwner());
		if (!Replication.IsServer() || !mode || !mode.IsPlaybackTelemetryEnabled() || mode.GetState() != SCR_EGameModeState.GAME || !controller || mask < 0 || mask > 3)
			return;
		IEntity body = controller.GetControlledEntity();
		if (!body || bodyId == RplId.Invalid() || Replication.FindItemId(body) != bodyId)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fVoiceReceivedAt < 50)
			return;
		m_fVoiceReceivedAt = now;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
			logs.ObserveVoice(controller.GetPlayerId(), body, mask);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_StatsRadioReporterComponent()
	{
		if (m_Capture)
			m_Capture.m_OnCaptureVON.Remove(OnVoiceCapture);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (SCR_Global.IsEditMode())
			return;
		SetEventMask(owner, EntityEvent.FRAME);
		m_fServerReceivedAt = GetGame().GetWorld().GetWorldTime() * 0.001;
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (GetGame().GetPlayerController() != owner)
			return;
		SampleVoice(mode, PlayerController.Cast(owner));
		if (!mode || !mode.IsCollectionEnabled())
			return;
		if (mode.GetState() == SCR_EGameModeState.POSTGAME && !m_bSentFinal)
		{
			m_bSentFinal = true;
			Rpc(RpcAsk_RadioSeconds, m_fRadioSeconds);
		}
		if (mode.GetState() != SCR_EGameModeState.GAME)
			return;
		m_fSampleElapsed += timeSlice;
		m_fReportElapsed += timeSlice;
		if (m_fSampleElapsed < 0.25)
			return;
		PlayerController controller = PlayerController.Cast(owner);
		IEntity character = controller.GetControlledEntity();
		if (character && !character.FindComponent(GC_LobbySpectatorBodyComponent))
		{
			SCR_VoNComponent voice = SCR_VoNComponent.Cast(character.FindComponent(SCR_VoNComponent));
			if (voice && voice.IsTransmitingRadio())
				m_fRadioSeconds += Math.Min(m_fSampleElapsed, 0.5);
		}
		m_fSampleElapsed = 0;
		if (m_fReportElapsed >= 10)
		{
			m_fReportElapsed = 0;
			Rpc(RpcAsk_RadioSeconds, m_fRadioSeconds);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RadioSeconds(float cumulativeSeconds)
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		PlayerController controller = PlayerController.Cast(GetOwner());
		if (!Replication.IsServer() || !stats || !stats.AcceptsRadioReport() || !controller)
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		float elapsed = Math.Max(0, now - m_fServerReceivedAt);
		float delta = cumulativeSeconds - m_fServerReported;
		if (!(delta >= 0 && delta <= 86400))
			return;
		GC_StatsPlayer record = stats.GetRecordedPlayer(controller.GetPlayerId());
		if (!record)
			return;
		float remaining = Math.Max(0, stats.GetDuration() - record.m_fRadioSecondsReported);
		stats.CreditRadio(record, Math.Min(delta, Math.Min(elapsed, remaining)), m_fServerReceivedAt, now);
		m_fServerReceivedAt = now;
		m_fServerReported = cumulativeSeconds;
	}
}
