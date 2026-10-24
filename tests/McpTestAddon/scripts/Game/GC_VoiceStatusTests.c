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
			result += string.Format("name=%1 rowVisible=%2 priority=%3 adminMuted=%4 voice=%5 deafened=%6\n",
				name.GetText(), GC_TestVisible(widget), widget.FindAnyWidget("PrioritySpeakerIcon").IsVisible(),
				widget.FindAnyWidget("AdminMutedIcon").IsVisible(), widget.FindAnyWidget("VoiceIcon").IsVisible(),
				widget.FindAnyWidget("VoiceDeafenedIcon").IsVisible());
		}
		return result;
	}

}
#endif
