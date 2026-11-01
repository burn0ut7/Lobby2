#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Presentation-only count fixture; -1 restores the actual native roster count.
	string GC_TestHeaderCount(int count)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget root = menu.GetRootWidget();
		TextWidget label = TextWidget.Cast(root.FindAnyWidget("PlayerCountText"));
		if (!label)
			return "UNAVAILABLE count";
		if (count < 0)
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			count = players.Count();
		}
		label.SetText(count.ToString());
		return "displayedCount=" + count.ToString();
	}
}
#endif
