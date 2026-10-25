//! Opt-in measurement after native map interpolation, with production drawing untouched.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAOMapMotion(bool start, int frames)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return "UNAVAILABLE open map";
		if (start)
			mapEntity.GC_TestStartAOMotion(m_vGC_AOOrigin, frames);
		return mapEntity.GC_TestAOMotionReport();
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	GC_MapShapeOverlay GC_TestAOMapOverlay()
	{
		return m_GC_MapShapes;
	}
}

modded class GC_MapShapeOverlay
{
	//------------------------------------------------------------------------------------------------
	float GC_TestProjectionError(SCR_MapEntity mapEntity)
	{
		if (!m_Canvas || m_aCommands.IsEmpty() || m_aCommands.Count() != m_aCounts.Count() * 2)
			return -1;
		float error;
		int offset;
		foreach (int index, int count : m_aCounts)
		{
			LineDrawCommand line = LineDrawCommand.Cast(m_aCommands[index * 2 + 1]);
			for (int i; i < count; i++)
			{
				vector point = m_aPoints[offset + i];
				int x, y;
				mapEntity.WorldToScreen(point[0], point[2], x, y, true);
				error = Math.Max(error, Math.AbsFloat(line.m_Vertices[i * 2] - x));
				error = Math.Max(error, Math.AbsFloat(line.m_Vertices[i * 2 + 1] - y));
			}
			offset += count;
		}
		return error;
	}

	//------------------------------------------------------------------------------------------------
	CanvasWidgetCommand GC_TestFirstCommand()
	{
		if (m_aCommands.IsEmpty())
			return null;
		return m_aCommands[0];
	}
}

modded class SCR_MapEntity
{
	protected int m_iGC_AOMotionTarget;
	protected int m_iGC_AOMotionFrames;
	protected int m_iGC_AOMotionMisses;
	protected int m_iGC_AOMotionMissing;
	protected int m_iGC_AOCommandChanges;
	protected float m_fGC_AOMaxError;
	protected float m_fGC_AOMotionSeconds;
	protected vector m_vGC_AOMotionCenter;
	protected ref CanvasWidgetCommand m_GC_AOLastCommand;

	//------------------------------------------------------------------------------------------------
	void GC_TestStartAOMotion(vector center, int frames)
	{
		m_iGC_AOMotionTarget = Math.Clamp(frames, 60, 2000);
		m_iGC_AOMotionFrames = 0;
		m_iGC_AOMotionMisses = 0;
		m_iGC_AOMotionMissing = 0;
		m_iGC_AOCommandChanges = 0;
		m_fGC_AOMaxError = 0;
		m_fGC_AOMotionSeconds = 0;
		m_vGC_AOMotionCenter = center;
		m_GC_AOLastCommand = null;
		ZoomPanSmooth(2, center[0], center[2], 1);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMotionReport()
	{
		return string.Format("frames=%1/%2 seconds=%3 misalignedFrames=%4 missingFrames=%5 maxErrorPixels=%6 commandReplacements=%7",
			m_iGC_AOMotionFrames, m_iGC_AOMotionTarget, m_fGC_AOMotionSeconds, m_iGC_AOMotionMisses,
			m_iGC_AOMotionMissing, m_fGC_AOMaxError, m_iGC_AOCommandChanges);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!IsOpen() || m_iGC_AOMotionFrames >= m_iGC_AOMotionTarget || !GC_LobbyGameMode.GetGameMode())
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_MapShapeOverlay overlay;
		if (controller)
			overlay = controller.GC_TestAOMapOverlay();
		float error = -1;
		if (overlay)
			error = overlay.GC_TestProjectionError(this);
		if (error < 0)
			m_iGC_AOMotionMissing++;
		// Native forward projection truncates to integer pixels; production retains fractions.
		else if (error > 1.01)
			m_iGC_AOMotionMisses++;
		m_fGC_AOMaxError = Math.Max(m_fGC_AOMaxError, error);
		if (overlay)
		{
			CanvasWidgetCommand command = overlay.GC_TestFirstCommand();
			if (m_GC_AOLastCommand && command != m_GC_AOLastCommand)
				m_iGC_AOCommandChanges++;
			m_GC_AOLastCommand = command;
		}
		m_iGC_AOMotionFrames++;
		m_fGC_AOMotionSeconds += timeSlice;
		if (m_iGC_AOMotionFrames == m_iGC_AOMotionTarget / 3)
			ZoomPanSmooth(0.8, m_vGC_AOMotionCenter[0] + 150, m_vGC_AOMotionCenter[2] + 100, 1);
		if (m_iGC_AOMotionFrames == m_iGC_AOMotionTarget * 2 / 3)
			ZoomPanSmooth(1.5, m_vGC_AOMotionCenter[0] - 150, m_vGC_AOMotionCenter[2] - 100, 1);
	}
}
