#ifdef WORKBENCH
//! Same stock chat widget and production callbacks in addon-free and addon-loaded runs.
modded class SCR_ChatPanel
{
	//------------------------------------------------------------------------------------------------
	string GC_IsolationChatInspect(bool send)
	{
		if (send)
		{
			m_Widgets.m_MessageEditBox.SetText("Native isolation chat");
			Callback_OnSendMessageAction();
		}
		string result = string.Format("open=%1", IsOpen());
		if (m_ActiveChannel)
			result += " active=" + m_ActiveChannel.GetName();
		for (int i; m_ChatEntity && i < m_ChatEntity.GetChannelsCount(); i++)
		{
			BaseChatChannel channel = m_ChatEntity.GetChannel(i);
			SCR_ChatMessageStyle style = GetChannelStyle(channel);
			result += string.Format(" [%1 available=%2 style=%3 prefix=%4]", channel.GetName(), channel.IsAvailable(GetChatComponent()), style.m_sName, style.m_sPrefix);
		}
		array<ref SCR_ChatMessage> messages = SCR_ChatPanelManager.GetInstance().GetMessages();
		result += string.Format(" messages=%1", messages.Count());
		foreach (SCR_ChatMessage message : messages)
		{
			SCR_ChatMessageGeneral general = SCR_ChatMessageGeneral.Cast(message);
			if (general && general.m_Channel)
				result += string.Format(" {%1|%2}", general.m_Channel.GetName(), general.m_sMessage);
		}
		return result;
	}
}

[BaseContainerProps(configRoot: true)]
modded class SCR_ChatPanelManager
{
	//------------------------------------------------------------------------------------------------
	string GC_IsolationPanelState()
	{
		string result = string.Format("panels=%1", m_aChatPanels.Count());
		foreach (SCR_ChatPanel panel : m_aChatPanels)
			result += string.Format(" {open=%1 visible=%2}", panel.IsOpen(), panel.GetWidget().IsVisible());
		return result;
	}
}

modded class GC_IsolationProbeComponent
{
	protected Widget m_wGC_ChatFrame;
	protected SCR_ChatPanel m_GC_ChatPanel;
	protected int m_iGC_JoinCommands;

	//------------------------------------------------------------------------------------------------
	string ChatJoinCommand()
	{
		m_iGC_JoinCommands = 0;
		SCR_ChatPanelManager manager = SCR_ChatPanelManager.GetInstance();
		manager.GetCommandInvoker("join").Insert(GC_OnJoinCommand);
		manager.Internal_OnChatCommand(null, "join", "");
		manager.GetCommandInvoker("join").Remove(GC_OnJoinCommand);
		return string.Format("inheritedJoinCallbacks=%1", m_iGC_JoinCommands);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_OnJoinCommand(SCR_ChatPanel panel, string data)
	{
		m_iGC_JoinCommands++;
	}

	//------------------------------------------------------------------------------------------------
	string ChatFaction(string factionKey)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player)
			return "UNAVAILABLE authority/player";
		SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(player.FindComponent(SCR_PlayerFactionAffiliationComponent));
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!affiliation || !faction)
			return "UNAVAILABLE affiliation/faction";
		bool accepted = affiliation.SetFaction_S(faction);
		return string.Format("accepted=%1 nativeFaction=%2", accepted, SCR_FactionManager.SGetPlayerFaction(player.GetPlayerId()) == faction);
	}

	//------------------------------------------------------------------------------------------------
	string Chat(string action)
	{
		if (action == "input-down")
			GetGame().GetInputManager().SetActionValue("ChatToggle", 1);
		if (action == "input-up")
			GetGame().GetInputManager().SetActionValue("ChatToggle", 0);
		if (action == "input-down" || action == "input-up" || action == "hud-snapshot")
			return SCR_ChatPanelManager.GetInstance().GC_IsolationPanelState();
		if (action == "create")
		{
			if (m_wGC_ChatFrame)
				return "REFUSED already created";
			m_wGC_ChatFrame = GetGame().GetWorkspace().CreateWidgets("{D89BBA18F99053D6}UI/layouts/HUD/Chat/ChatHud.layout");
			Widget panelWidget = m_wGC_ChatFrame.FindAnyWidget("ChatPanel");
			m_GC_ChatPanel = SCR_ChatPanel.Cast(panelWidget.FindHandler(SCR_ChatPanel));
			SetEventMask(GetOwner(), EntityEvent.FRAME);
		}
		if (!m_GC_ChatPanel)
			return "UNAVAILABLE probe chat";
		if (action == "open")
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_GC_ChatPanel);
		if (action == "close")
			SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_GC_ChatPanel);
		string report = m_GC_ChatPanel.GC_IsolationChatInspect(action == "send");
		if (action == "destroy")
		{
			SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_GC_ChatPanel);
			m_wGC_ChatFrame.RemoveFromHierarchy();
			m_wGC_ChatFrame = null;
			m_GC_ChatPanel = null;
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
		}
		return report;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (m_GC_ChatPanel)
			m_GC_ChatPanel.OnUpdateChat(timeSlice);
	}
}
#endif
