#ifdef WORKBENCH
[BaseContainerProps(configRoot: true)]
class GC_CaptureAuthoringFixture
{
	[Attribute()]
	ref GC_AreaCaptureOptions m_Capture;
	[Attribute()]
	ref GC_LocalFactionShare m_Share;
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCaptureAuthoring()
	{
		Resource resource = Resource.Load("{F71C000000000010}Configs/CaptureAuthoring.conf");
		if (!resource || !resource.IsValid())
			return "Capture authoring fixture unavailable";
		GC_CaptureAuthoringFixture fixture = GC_CaptureAuthoringFixture.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		if (!fixture || !fixture.m_Capture || !fixture.m_Capture.m_Messages || !fixture.m_Share)
			return "Capture authoring fixture invalid";
		GC_AreaCaptureOptions capture = fixture.m_Capture;
		GC_CaptureMessages messages = capture.m_Messages;
		bool timing = capture.m_iCheckInterval == 3 && capture.m_fSeconds == 9 && capture.m_iInterruption == 2 && capture.m_bSkipDiscovery;
		bool text = messages.m_sLocation == "Custom depot" && messages.m_sCaptured == "Taken: %2" && messages.m_sCapturing.IsEmpty()
			&& messages.m_sCaptureInterrupted == "Paused" && messages.m_sLost == "Gone" && messages.m_sLosing == "Leaving" && messages.m_sLossInterrupted == "Recovered";
		bool share = Math.AbsFloat(fixture.m_Share.m_fRequiredShare - 0.6) < 0.0001;
		GC_AreaCaptureOptions copy = capture.Copy();
		bool copied = copy.IsValid() && copy.m_iCheckInterval == 3 && copy.m_Messages.m_sCaptured == "Taken: %2" && copy.m_Messages.m_sCapturing.IsEmpty();
		return string.Format("Capture authored settings: timing=%1 messages=%2 share=%3 copied=%4", timing, text, share, copied);
	}
}
#endif
