//! Shared geometry for the setup, admin staging and announcement countdown layout.
class GC_CountdownHud
{
	static const float WIDTH = 250;

	//------------------------------------------------------------------------------------------------
	//! All boundary HUD cards share a width; label and time fill the row within its margins.
	static void Fit(Widget root, TextWidget timer)
	{
		TextWidget title = TextWidget.Cast(root.FindAnyWidget("StagingCountdownTitle"));
		float timerWidth, timerHeight;
		timer.GetTextSize(timerWidth, timerHeight);
		FrameSlot.SetAnchorMax(title, 0, 1);
		FrameSlot.SetSizeX(title, WIDTH - timerWidth - 20);
		title.SetDesiredFontSize(18);
		title.SetMinFontSize(8);
		FrameSlot.SetPosX(timer, -timerWidth - 6);
		FrameSlot.SetSizeX(timer, timerWidth);
		FrameSlot.SetSizeX(root.FindAnyWidget("StagingCountdownCard"), WIDTH);
	}
}
