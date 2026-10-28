#ifdef WORKBENCH
modded class GC_SpectatorFactionListComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFactionState(string action)
	{
		if (action == "ratio")
		{
			// Native layout probe only; no player counts or backend state are changed.
			Widget ratios = m_wRoot.FindAnyWidget("FactionRatios");
			Clear(ratios);
			array<int> weights = {5, 10};
			foreach (int index, int weight : weights)
			{
				Widget segment = GetGame().GetWorkspace().CreateWidgets("{D389716BFE230B02}UI/layouts/Spectator/Lobby_Spectator_Faction_Segment.layout", ratios);
				segment.SetName("RatioProbe" + index.ToString());
				LayoutSlot.SetFillWeight(segment, weight);
			}
			m_wRoot.Update();
		}
		if (action == "refresh")
			Refresh();
		string result = string.Format("active=%1 life=%2 faction=%3 empty=%4", m_bActive, m_iLifeFilter, m_sFaction, m_wRoot.FindAnyWidget("FactionEmpty").IsVisible());
		Widget segment = m_wRoot.FindAnyWidget("FactionRatios").GetChildren();
		while (segment)
		{
			float width, height;
			segment.GetScreenSize(width, height);
			result += string.Format(" [%1 width=%2 height=%3]", segment.GetName(), width, height);
			segment = segment.GetSibling();
		}
		Widget row = m_wRoot.FindAnyWidget("FactionRoster").GetChildren();
		while (row)
		{
			result += " " + row.GetName();
			row = row.GetSibling();
		}
		return result;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFactionRoster(string action)
	{
		if (action == "open")
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_SpectatorMenu, 0, true);
			return "Opened production spectator menu for presentation test";
		}
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		Widget root = menu.GetRootWidget();
		Widget panel = root.FindAnyWidget("SpectatorFactions");
		if (!panel)
			return "UNAVAILABLE faction panel";
		if (action == "select")
		{
			Widget playerRow = panel.FindAnyWidget("FactionPlayer_1");
			if (playerRow)
				GC_VoicePlayerRowComponent.Cast(playerRow.FindHandler(GC_VoicePlayerRowComponent)).OnClick(playerRow, 0, 0, 0);
		}
		if (action == "events")
		{
			m_OnVoiceAssignmentChanged.Invoke(1);
			m_OnPlayerRoleChanged.Invoke(1);
		}
		if (action == "label")
		{
			Widget playerRow = panel.FindAnyWidget("FactionPlayer_1");
			if (!playerRow)
				return "UNAVAILABLE player row";
			return TextWidget.Cast(playerRow.FindAnyWidget("GroupCallsign")).GetText();
		}
		if (action == "dead" || action == "alive")
		{
			GC_Slot slot = GetSlotByPlayerId(GetGame().GetPlayerController().GetPlayerId());
			if (!slot || !Replication.IsServer())
				return "UNAVAILABLE assigned host slot";
			if (action == "dead")
				slot.SetState(GC_SlotState.KIA);
			else
				slot.SetState(GC_SlotState.FREE);
		}
		return GC_SpectatorFactionListComponent.Cast(panel.FindHandler(GC_SpectatorFactionListComponent)).GC_TestFactionState(action);
	}
}
#endif
