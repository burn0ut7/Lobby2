#ifdef WORKBENCH

modded class SCR_WLibComponentBase
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAnimationTiming()
	{
		return string.Format("seconds=%1 rate=%2", m_fAnimationTime, m_fAnimationRate);
	}
}

#endif
