//! Sample statistics, not a frame-time trace. FPS is the host's last-ten-frame average.
class GC_MissionPerformance
{
	int m_iSamples;
	float m_fFpsSum;
	float m_fMinimum;
	float m_fMaximum;
	int m_iBelow15;
	int m_iPeakPlayers;
	int m_iPeakAI;
	float m_fLargestGap;
	float m_fLastSample = -1;

	//------------------------------------------------------------------------------------------------
	void Sample(float seconds, float fps, int players, int activeAI)
	{
		if (m_iSamples == 0)
			m_fMinimum = fps;
		if (m_fLastSample >= 0)
			m_fLargestGap = Math.Max(m_fLargestGap, seconds - m_fLastSample);
		m_fLastSample = seconds;
		m_iSamples++;
		m_fFpsSum += fps;
		m_fMinimum = Math.Min(m_fMinimum, fps);
		m_fMaximum = Math.Max(m_fMaximum, fps);
		if (fps < 15)
			m_iBelow15++;
		m_iPeakPlayers = Math.Max(m_iPeakPlayers, players);
		m_iPeakAI = Math.Max(m_iPeakAI, activeAI);
	}

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		float mean;
		if (m_iSamples > 0)
			mean = m_fFpsSum / m_iSamples;
		context.WriteValue("samples", m_iSamples);
		context.WriteValue("meanSampleFps", mean);
		context.WriteValue("minimumSampleFps", m_fMinimum);
		context.WriteValue("maximumSampleFps", m_fMaximum);
		context.WriteValue("samplesBelow15Fps", m_iBelow15);
		context.WriteValue("peakSampledPlayers", m_iPeakPlayers);
		context.WriteValue("peakSampledActiveAI", m_iPeakAI);
		context.WriteValue("largestSampleGapSeconds", m_fLargestGap);
	}
}
