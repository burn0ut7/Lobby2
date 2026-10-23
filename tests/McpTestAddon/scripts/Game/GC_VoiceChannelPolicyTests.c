//! Optional MCP checks against live players and production channel policy.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPersonalChannelJoin(int playerId, int ownerId)
	{
		if (GetManager() != this || !Replication.IsServer())
			return "UNAVAILABLE authority";
		bool accepted = TrySetPlayerVoiceChannel(playerId, GC_VoiceChannelType.PERSONAL,
			"", RplId.Invalid(), GetPlayerVoiceIdentity(ownerId));
		return string.Format("accepted=%1 %2", accepted, GC_TestVoiceChannelSnapshot(playerId));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestVoiceListOrder()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "UNAVAILABLE menu";
		return GC_TestFindVoiceList(menu.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestFindVoiceList(Widget widget)
	{
		GC_VoiceChannelListComponent list = GC_VoiceChannelListComponent.Cast(widget.FindHandler(GC_VoiceChannelListComponent));
		if (list)
			return list.GC_TestChannelOrder();
		Widget child = widget.GetChildren();
		while (child)
		{
			string result = GC_TestFindVoiceList(child);
			if (!result.IsEmpty())
				return result;
			child = child.GetSibling();
		}
		return "";
	}
}

modded class GC_VoiceChannelListComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestChannelOrder()
	{
		string result;
		foreach (string key, Widget widget : m_mChannelWidgets)
		{
			float x, y;
			widget.GetScreenPos(x, y);
			TextWidget label = TextWidget.Cast(widget.FindAnyWidget("ChannelName"));
			GC_VoiceChannelWidgetComponent card = GC_VoiceChannelWidgetComponent.Cast(widget.FindHandler(GC_VoiceChannelWidgetComponent));
			string expected;
			if (card && card.GetGroup())
				expected = GC_GroupCallsignHelper.GetDisplayName(card.GetGroup());
			result += string.Format("[%1 order=%2 y=%3 label='%4' currentCallsign='%5'] ", key, widget.GetZOrder(), y, label.GetText(), expected);
		}
		return result;
	}
}
