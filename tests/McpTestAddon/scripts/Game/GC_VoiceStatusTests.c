//! Observe rendered status after production voice requests.
#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceIcons()
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		string result;
		foreach (Widget widget : widgets)
		{
			if (!widget.FindHandler(GC_VoicePlayerRowComponent))
				continue;
			TextWidget name = TextWidget.Cast(widget.FindAnyWidget("PlayerName"));
			Widget voice = widget.FindAnyWidget("VoiceIcon");
			result += string.Format("name=%1 rowVisible=%2 voice=%3 deafened=%4 color=%5 opacity=%6\n",
				name.GetText(), GC_TestVisible(widget), voice.IsVisible(),
				widget.FindAnyWidget("VoiceDeafenedIcon").IsVisible(), voice.GetColor().ToString(), voice.GetOpacity());
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Synthetic activity exercises the production row listener and its idle timeout.
	//! This does not establish audible transmission or native microphone capture.
	string GC_TestVoiceActivity(int pulses)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || pulses < 1 || pulses > 30)
			return "UNAVAILABLE controller or pulse count";
		controller.m_OnVoiceActivity.Invoke(controller.GetPlayerId());
		if (pulses > 1)
			GetGame().GetCallqueue().CallLater(GC_TestVoiceActivity, 200, false, pulses - 1);
		return "DISPATCHED synthetic activity through production listener";
	}

}
#endif
