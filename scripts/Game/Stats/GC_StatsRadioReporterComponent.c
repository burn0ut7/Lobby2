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
		if (!mode || GetGame().GetPlayerController() != owner)
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
