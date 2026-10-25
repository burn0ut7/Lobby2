//! Read actual widget text and screen rectangles; gameplay uses existing fixtures.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSetupHud()
	{
		array<string> roots = {"LobbyStagingCountdown", "LobbySetupWarning", "LobbyAOWarning"};
		string result;
		foreach (string name : roots)
		{
			Widget root = GetGame().GetWorkspace().FindAnyWidget(name);
			if (!root)
			{
				result += name + "=absent; ";
				continue;
			}
			string cardName = "AOCard";
			string titleName = "AOTitle";
			string timerName = "AOTimer";
			if (name == "LobbyStagingCountdown")
			{
				cardName = "StagingCountdownCard";
				titleName = "StagingCountdownTitle";
				timerName = "StagingCountdownTime";
			}
			Widget card = root.FindAnyWidget(cardName);
			TextWidget title = TextWidget.Cast(root.FindAnyWidget(titleName));
			TextWidget timer = TextWidget.Cast(root.FindAnyWidget(timerName));
			if (!card || !title || !timer)
				return "FAILED widget structure " + name;
			float x, y, width, height;
			card.GetScreenPos(x, y);
			card.GetScreenSize(width, height);
			result += string.Format("%1 title=%2 timer=%3 rect=%4,%5,%6,%7; ", name, title.GetText(), timer.GetText(), x, y, width, height);
		}
		return result;
	}
}
