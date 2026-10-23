//! Optional native chat exercise: no identities, roles or routing are substituted.
modded class SCR_ChatPanel
{
	//------------------------------------------------------------------------------------------------
	string GC_TestNativeChatUI(string action, string channelName, string text)
	{
		if (action == "cycle")
			CycleChannels(true);
		if (action == "draft")
			m_Widgets.m_MessageEditBox.SetText(text);
		if (action == "submit")
			Callback_OnSendMessageAction();
		if (action == "prefix")
		{
			m_Widgets.m_MessageEditBox.SetText(text);
			Callback_OnPressSpaceAfterCommand();
		}
		if (action == "send")
		{
			for (int i; i < m_ChatEntity.GetChannelsCount(); i++)
			{
				BaseChatChannel channel = m_ChatEntity.GetChannel(i);
				if (channel.GetName() == channelName)
				{
					SCR_ChatPanelManager.GetInstance().OpenChatPanel(this);
					SetActiveChannel(channel);
					m_Widgets.m_MessageEditBox.SetText(text);
					Callback_OnSendMessageAction();
					break;
				}
			}
		}
		string active;
		if (m_ActiveChannel)
			active = m_ActiveChannel.GetName();
		return string.Format("open=%1 active=%2", IsOpen(), active);
	}
}

class GC_NativeChatTest
{
	//------------------------------------------------------------------------------------------------
	static string Run(string action, string channelName, string text)
	{
		PlayerController controller = GetGame().GetPlayerController();
		ScriptedChatEntity chat = ScriptedChatEntity.Cast(GetGame().GetChat());
		SCR_ChatPanelManager manager = SCR_ChatPanelManager.GetInstance();
		if (!controller || !chat || !manager)
			return "UNAVAILABLE native chat";
		BaseChatComponent component = BaseChatComponent.Cast(controller.FindComponent(BaseChatComponent));
		if (!component)
			return "UNAVAILABLE native chat component";
		if (action == "whisper")
			component.SendPrivateMessage(text, channelName.ToInt());
		if (action == "input-down")
			GetGame().GetInputManager().SetActionValue("ChatToggle", 1);
		if (action == "input-up")
			GetGame().GetInputManager().SetActionValue("ChatToggle", 0);
		if (action == "interface-down")
			GetGame().GetInputManager().SetActionValue("GC_ToggleSpectatorInterface", 1);
		if (action == "interface-up")
			GetGame().GetInputManager().SetActionValue("GC_ToggleSpectatorInterface", 0);
		if (action == "interface-show")
			GC_LobbyPlayerController.Cast(controller).SetSpectatorInterfaceVisible(true);
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		SCR_ChatPanel panel;
		if (menu && menu.GetRootWidget())
		{
			Widget panelWidget = menu.GetRootWidget().FindAnyWidget("ChatPanel");
			if (panelWidget)
				panel = SCR_ChatPanel.Cast(panelWidget.FindHandler(SCR_ChatPanel));
		}
		string result = string.Format("player=%1 admin=%2 channels=%3 panel=%4 spectator=%5", controller.GetPlayerId(), SCR_Global.IsAdmin(controller.GetPlayerId()), chat.GetChannelsCount(), panel != null, GC_LobbyChatMembership.IsSpectator(component));
		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.Cast(controller.FindComponent(SCR_PlayerControllerGroupComponent));
		Faction faction = SCR_FactionManager.SGetPlayerFaction(controller.GetPlayerId());
		if (groupComponent)
			result += string.Format(" nativeGroup=%1", groupComponent.GetGroupID());
		SCR_AIGroup group = GC_LobbyManager.GetManager().GetPlayerGroup(controller.GetPlayerId());
		if (group)
			result += string.Format(" groupFrequency=%1", group.GetRadioFrequency());
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(controller.GetControlledEntity());
		if (vehicle)
			result += string.Format(" vehicle=%1", Replication.FindItemId(vehicle));
		if (faction)
			result += " nativeFaction=" + faction.GetFactionKey();
		else
			result += " nativeFaction=none";
		if (menu)
			result += string.Format(" menu=%1 visible=%2", menu.Type(), menu.GetRootWidget().IsVisible());
		if (action == "open" && panel)
			manager.OpenChatPanel(panel);
		if (action == "close" && panel)
			manager.CloseChatPanel(panel);
		if (panel)
			result += " " + panel.GC_TestNativeChatUI(action, channelName, text);
		for (int i; i < chat.GetChannelsCount(); i++)
		{
			BaseChatChannel channel = chat.GetChannel(i);
			result += string.Format(" [%1:%2 available=%3]", i, channel.GetName(), channel.IsAvailable(component));
			if (action == "raw-send" && channel.GetName() == channelName)
				component.SendMessage(text, i);
		}
		array<ref SCR_ChatMessage> messages = manager.GetMessages();
		result += string.Format(" messages=%1", messages.Count());
		for (int j = Math.Max(0, messages.Count() - 10); j < messages.Count(); j++)
		{
			SCR_ChatMessageGeneral message = SCR_ChatMessageGeneral.Cast(messages[j]);
			if (message && message.m_Channel)
				result += string.Format(" {%1|%2|%3}", message.m_Channel.GetName(), message.m_iSenderId, message.m_sMessage);
			SCR_ChatMessagePrivate privateMessage = SCR_ChatMessagePrivate.Cast(message);
			if (privateMessage)
				result += string.Format(" receiver=%1", privateMessage.m_iReceiverId);
		}
		return result;
	}
}

modded class GC_LobbyManager
{
	protected string m_sGC_NativeChatReport;

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminChatRecipients()
	{
		if (!Replication.IsServer())
			return "UNAVAILABLE authority";
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		string result;
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (player)
				result += player.GC_TestAdminChatWindow() + " ";
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Exercise the server callback with a real connected sender, without changing roles.
	string GC_TestNativeChatAuthority(int playerId, string channelName, string text)
	{
		PlayerController player = GetGame().GetPlayerManager().GetPlayerController(playerId);
		BaseChatEntity chat = GetGame().GetChat();
		if (!Replication.IsServer() || !player || !chat)
			return "UNAVAILABLE authority/player/chat";
		BaseChatComponent component = BaseChatComponent.Cast(player.FindComponent(BaseChatComponent));
		for (int i; i < chat.GetChannelsCount(); i++)
		{
			BaseChatChannel channel = chat.GetChannel(i);
			if (channel.GetName() == channelName)
				return string.Format("player=%1 channel=%2 accepted=%3", playerId, channelName, channel.ProcessMessage(component, text, true));
		}
		return "UNAVAILABLE channel";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestNativeChat(string action, string channelName, string text)
	{
		return GC_NativeChatTest.Run(action, channelName, text);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestNativeChatPeer(int playerId, string action, string channelName, string text)
	{
		if (action == "report")
			return m_sGC_NativeChatReport;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller || !Replication.IsServer())
			return "UNAVAILABLE peer";
		m_sGC_NativeChatReport = "pending";
		controller.GC_TestNativeChatDispatch(action, channelName, text);
		return "dispatched; inspect report";
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestNativeChatReport(string report)
	{
		m_sGC_NativeChatReport = report;
		PrintFormat("GC_TEST_NATIVE_CHAT | %1", report);
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminChatWindow()
	{
		return string.Format("player=%1 admin=%2 receives=%3 nowMs=%4 deadlineMs=%5", GetPlayerId(), SCR_Global.IsAdmin(GetPlayerId()), CanReceiveAdminChat(), GetGame().GetWorld().GetWorldTime(), m_fAdminChatReceiveUntil);
	}

	//------------------------------------------------------------------------------------------------
	void GC_TestNativeChatDispatch(string action, string channelName, string text)
	{
		if (Replication.IsServer())
			Rpc(GC_TestNativeChatOwner, action, channelName, text);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_TestNativeChatOwner(string action, string channelName, string text)
	{
		if (GetGame().GetPlayerController() == this)
			Rpc(GC_TestNativeChatReply, GC_NativeChatTest.Run(action, channelName, text));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_TestNativeChatReply(string report)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.GC_TestNativeChatReport(report);
	}
}
