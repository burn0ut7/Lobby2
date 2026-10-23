#ifdef WORKBENCH
//! Explicit, bounded observations. No menu actions or permission changes run automatically.
modded class GC_LobbyManager
{
	protected int m_iGC_ProfileSamples;
	protected string m_sGC_ProfileSamples;
	protected ref array<float> m_aGC_FrameTimes = {};
	protected int m_iGC_FramesRemaining;
	protected bool m_bGC_OwnFrameMask;
	protected int m_iGC_FrameStart;
	protected int m_iGC_FrameEnd;

	//------------------------------------------------------------------------------------------------
	//! Native component frame intervals; rendering/GPU presentation is a separate metric.
	string GC_TestAdminFramesStart(int frames)
	{
		if (GetManager() != this || m_iGC_FramesRemaining > 0 || frames < 100 || frames > 2000)
			return "UNAVAILABLE active manager, idle recorder and 100-2000 frames required";
		m_aGC_FrameTimes.Clear();
		m_iGC_FramesRemaining = frames;
		m_iGC_FrameStart = System.GetTickCount();
		m_iGC_FrameEnd = 0;
		m_bGC_OwnFrameMask = (GetEventMask() & EntityEvent.FRAME) == 0;
		SetEventMask(GetOwner(), EntityEvent.FRAME);
		return "STARTED native EOnFrame intervals";
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (GetManager() != this || m_iGC_FramesRemaining <= 0)
			return;
		m_aGC_FrameTimes.Insert(timeSlice * 1000);
		m_iGC_FramesRemaining--;
		if (m_iGC_FramesRemaining == 0)
		{
			m_iGC_FrameEnd = System.GetTickCount();
			if (m_bGC_OwnFrameMask)
				ClearEventMask(owner, EntityEvent.FRAME);
			m_bGC_OwnFrameMask = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminFramesReport()
	{
		int width, height;
		System.GetRenderingResolution(width, height);
		string report = string.Format("remaining=%1 start=%2 end=%3 render=%4x%5 samples=%6\n",
			m_iGC_FramesRemaining, m_iGC_FrameStart, m_iGC_FrameEnd, width, height, m_aGC_FrameTimes.Count());
		foreach (float frame : m_aGC_FrameTimes)
			report += frame.ToString() + ",";
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminProfileStart(int samples)
	{
		if (GetManager() != this || m_iGC_ProfileSamples > 0 || samples < 20 || samples > 200)
			return "UNAVAILABLE active manager, idle recorder and 20-200 samples required";
		m_iGC_ProfileSamples = samples;
		m_sGC_ProfileSamples = "tick,frame,fps,frameMs,renderMs,gpuMs,simulationMs\n";
		GetGame().GetCallqueue().CallLater(GC_TestAdminProfileSample, 100);
		return "STARTED native ProfileData samples at 100ms intervals; averages are not individual frame times";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestAdminProfileSample()
	{
		if (GetManager() != this || m_iGC_ProfileSamples <= 0)
			return;
		ProfileData profile = System.GetProfileData();
		m_sGC_ProfileSamples += string.Format("%1,%2,%3,%4,%5,%6,%7\n", System.GetTickCount(),
			GetGame().GetWorld().GetFrameNumber(), profile.FPS, profile.FrameTimeMs,
			profile.RenderTimeMs, profile.GPUTimeMs, profile.SimulationTimeMs);
		m_iGC_ProfileSamples--;
		if (m_iGC_ProfileSamples > 0)
			GetGame().GetCallqueue().CallLater(GC_TestAdminProfileSample, 100);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminProfileReport()
	{
		return string.Format("remaining=%1 adapter=%2\n%3", m_iGC_ProfileSamples, System.GetAdapterName(), m_sGC_ProfileSamples);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Shutdown()
	{
		if (m_bGC_OwnFrameMask)
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
		m_bGC_OwnFrameMask = false;
		m_iGC_FramesRemaining = 0;
		GetGame().GetCallqueue().Remove(GC_TestAdminProfileSample);
		m_iGC_ProfileSamples = 0;
		super.Shutdown();
	}
}
#endif
