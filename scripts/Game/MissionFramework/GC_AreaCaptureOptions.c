[BaseContainerProps(), BaseContainerCustomStringTitleField("Capture")]
class GC_AreaCaptureOptions
{
	[Attribute("0", UIWidgets.EditBox, "Seconds of a changed area requirement needed to capture or lose the area. Zero changes instantly. Progress is measured at Check Interval; 10 seconds at 2 seconds needs five ticks.", "0 86400")]
	float m_fSeconds;
	[Attribute("1", UIWidgets.EditBox, "Seconds between capture checks, in whole seconds. Uses the existing area sampler. No catch-up ticks are invented after a delayed server update.", "1 3600 1")]
	int m_iCheckInterval = 1;
	[Attribute("0", UIWidgets.ComboBox, "When the requirement returns to the current captured state before a change completes: Roll back removes progress over time, Hold keeps it, Reset clears it. The same rule applies to interrupted capture and interrupted loss.", enums: {ParamEnum("Roll back", "0"), ParamEnum("Hold", "1"), ParamEnum("Reset", "2")})]
	int m_iInterruption;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: first valid Match observation establishes the initial result immediately and silently, like TilW discovery. Checked: start uncaptured and require the full capture time.")]
	bool m_bSkipDiscovery;
	[Attribute("0", UIWidgets.CheckBox, "Keep the result after its first completed timed change, including a loss after initial discovery. Discovery itself does not stop the capture. Missing areas still fail closed.")]
	bool m_bStopAfterFirstChange;
	[Attribute("", UIWidgets.Object, "Optional status messages. Leave unset for silent objectives. Empty message fields suppress individual transitions.")]
	ref GC_CaptureMessages m_Messages;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return m_fSeconds >= 0 && m_fSeconds <= 86400 && m_iCheckInterval >= 1 && m_iCheckInterval <= 3600
			&& m_iInterruption >= 0 && m_iInterruption <= 2 && (!m_Messages || m_Messages.IsValid());
	}

	//------------------------------------------------------------------------------------------------
	GC_AreaCaptureOptions Copy()
	{
		GC_AreaCaptureOptions result = new GC_AreaCaptureOptions();
		result.m_fSeconds = m_fSeconds;
		result.m_iCheckInterval = m_iCheckInterval;
		result.m_iInterruption = m_iInterruption;
		result.m_bSkipDiscovery = m_bSkipDiscovery;
		result.m_bStopAfterFirstChange = m_bStopAfterFirstChange;
		if (m_Messages)
			result.m_Messages = m_Messages.Copy();
		return result;
	}
}

//! One condition's capture memory; native factions and entities remain owned by the game.
class GC_AreaCaptureState
{
	protected bool m_bInitialized;
	protected bool m_bCaptured;
	protected bool m_bStopped;
	protected float m_fProgress;
	protected float m_fLastCheck;
	protected int m_iStatus = -1;

	//------------------------------------------------------------------------------------------------
	bool IsCaptured()
	{
		return m_bCaptured;
	}

	//------------------------------------------------------------------------------------------------
	float GetProgress()
	{
		return m_fProgress;
	}

	//------------------------------------------------------------------------------------------------
	void Suspend(float now)
	{
		m_fProgress = 0;
		m_fLastCheck = now;
		m_iStatus = -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns a changed message status, or -1. No UI, callbacks or clock reads.
	int Sample(GC_AreaCaptureOptions options, bool requirement, float now)
	{
		if (!options || !options.IsValid() || now < m_fLastCheck || m_bStopped)
			return -1;
		if (!m_bInitialized)
		{
			m_bInitialized = true;
			m_fLastCheck = now;
			if (!options.m_bSkipDiscovery)
			{
				m_bCaptured = requirement;
				return -1;
			}
			if (!requirement)
				return -1;
			if (options.m_fSeconds > 0)
				return ChangeStatus(1);
		}
		else if (now - m_fLastCheck < options.m_iCheckInterval)
			return -1;
		// A sample represents at most one configured tick, never an unobserved stall.
		float delta = Math.Min(now - m_fLastCheck, options.m_iCheckInterval);
		m_fLastCheck = now;
		int direction;
		if (m_bCaptured)
			direction = 3;
		if (requirement != m_bCaptured)
		{
			m_fProgress += delta;
			if (m_fProgress >= options.m_fSeconds)
			{
				m_bCaptured = requirement;
				m_fProgress = 0;
				m_bStopped = options.m_bStopAfterFirstChange;
				return ChangeStatus(direction);
			}
			return ChangeStatus(direction + 1);
		}
		bool interrupted = m_fProgress > 0 || m_iStatus == direction + 1;
		if (options.m_iInterruption == 0)
			m_fProgress = Math.Max(0, m_fProgress - delta);
		else if (options.m_iInterruption == 2)
			m_fProgress = 0;
		if (interrupted)
			return ChangeStatus(direction + 2);
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int ChangeStatus(int status)
	{
		if (m_iStatus == status)
			return -1;
		m_iStatus = status;
		return status;
	}
}
