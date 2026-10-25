//! Shared geometry for the setup, admin staging and announcement countdown layout.
class GC_CountdownHud
{
	//------------------------------------------------------------------------------------------------
	//! Six-unit outer margins and an eight-unit gap stay consistent with any heading.
	static void Fit(Widget root, TextWidget timer)
	{
		TextWidget title = TextWidget.Cast(root.FindAnyWidget("StagingCountdownTitle"));
		float titleWidth, titleHeight, timerWidth, timerHeight;
		title.GetTextSize(titleWidth, titleHeight);
		timer.GetTextSize(timerWidth, timerHeight);
		FrameSlot.SetAnchorMax(title, 0, 1);
		FrameSlot.SetSizeX(title, titleWidth + 1);
		FrameSlot.SetPosX(timer, -timerWidth - 6);
		FrameSlot.SetSizeX(timer, timerWidth);
		FrameSlot.SetSizeX(root.FindAnyWidget("StagingCountdownCard"), titleWidth + timerWidth + 20);
	}
}
