//! Per-condition runtime state. Authored conditions never retain session history.
class GC_MissionCaptureProgress
{
	protected bool m_bInitialized;
	protected bool m_bCaptured;
	protected float m_fLastTime;
	protected float m_fProgress;
	protected int m_iStatus = -1;

	//------------------------------------------------------------------------------------------------
	//! Status: captured, capturing, capture interrupted, lost, losing, loss interrupted.
	bool Update(bool met, float now, float seconds, bool discoverImmediately, out int changedStatus)
	{
		changedStatus = -1;
		if (!m_bInitialized)
		{
			m_bInitialized = true;
			m_fLastTime = now;
			if (discoverImmediately)
			{
				m_bCaptured = met;
				return m_bCaptured;
			}
		}
		float elapsed = Math.Max(0, now - m_fLastTime);
		m_fLastTime = now;
		int status = m_iStatus;
		if (met != m_bCaptured)
		{
			m_fProgress += elapsed;
			if (m_fProgress >= seconds)
			{
				m_bCaptured = met;
				m_fProgress = 0;
				status = 3;
				if (met)
					status = 0;
			}
			else
			{
				status = 4;
				if (met)
					status = 1;
			}
		}
		else if (m_fProgress > 0)
		{
			m_fProgress = Math.Max(0, m_fProgress - elapsed);
			status = 2;
			if (m_bCaptured)
				status = 5;
		}
		if (status != m_iStatus)
		{
			m_iStatus = status;
			changedStatus = status;
		}
		return m_bCaptured;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Capture Progress")]
class GC_CaptureCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.Object, "Condition that captures this objective. Use an area condition or a Condition Group.")]
	ref GC_MissionCondition m_Condition;
	[Attribute("10", UIWidgets.EditBox, "Seconds to capture or lose the objective. Interrupted progress gradually runs back toward zero.", params: "0 86400 0.1")]
	float m_fSeconds;
	[Attribute("0", UIWidgets.CheckBox, "Use the first condition result immediately, without a capture delay. Later changes still take the configured time.", category: "Advanced")]
	bool m_bDiscoverImmediately;
	[Attribute("Objective", UIWidgets.EditBox, "Name used in status messages and logs.")]
	string m_sName;
	[Attribute("", UIWidgets.EditBox, "Optional message when captured.", category: "Status Messages")]
	string m_sCaptured;
	[Attribute("", UIWidgets.EditBox, "Optional message when capture starts.", category: "Status Messages")]
	string m_sCapturing;
	[Attribute("", UIWidgets.EditBox, "Optional message when capture is interrupted.", category: "Status Messages")]
	string m_sCaptureInterrupted;
	[Attribute("", UIWidgets.EditBox, "Optional message when lost.", category: "Status Messages")]
	string m_sLost;
	[Attribute("", UIWidgets.EditBox, "Optional message when loss starts.", category: "Status Messages")]
	string m_sLosing;
	[Attribute("", UIWidgets.EditBox, "Optional message when loss is interrupted.", category: "Status Messages")]
	string m_sLossInterrupted;
	[Attribute("", UIWidgets.EditBox, "Message recipients. Empty means everyone.", category: "Status Messages")]
	ref array<string> m_aFactions;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_Condition || m_fSeconds < 0 || m_fSeconds > 86400)
		{
			error = "Capture Progress: add a condition and duration from 0 to 86400 seconds.";
			return false;
		}
		if (m_aFactions)
		{
			foreach (string faction : m_aFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Capture Progress: unknown faction " + faction;
					return false;
				}
			}
		}
		return m_Condition.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override void Reset(GC_LobbyMissionContext context)
	{
		super.Reset(context);
		m_Condition.Reset(context);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		int changed;
		bool captured = context.CaptureProgress(this).Update(m_Condition.Evaluate(context), context.GetTime(), m_fSeconds, m_bDiscoverImmediately, changed);
		if (changed >= 0)
		{
			array<string> statuses = {"captured", "capturing", "capture interrupted", "lost", "losing", "loss interrupted"};
			PrintFormat("GC | Mission Framework | capture: name=%1 status=%2", m_sName, statuses[changed]);
			array<string> messages = {m_sCaptured, m_sCapturing, m_sCaptureInterrupted, m_sLost, m_sLosing, m_sLossInterrupted};
			if (!messages[changed].IsEmpty())
				context.GetManager().GC_SendMissionMessage(m_sName, messages[changed], 5, m_aFactions, false, false);
		}
		return captured;
	}
}
