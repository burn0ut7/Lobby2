//! Keep native input, history, transport and commands. Only Lobby2 channel styles differ.
modded class SCR_ChatPanel
{
	protected ref SCR_ChatMessageStyle m_GC_AdminChannelStyle;
	protected ref SCR_ChatMessageStyle m_GC_SpectatorChannelStyle;

	//------------------------------------------------------------------------------------------------
	override void Internal_Open()
	{
		// Lobby menus can attach before the local player controller is published.
		// Initialize on the user's first open, once native chat ownership exists.
		if (GC_LobbyManager.GetManager() && !m_ActiveChannel && m_ChatEntity && GetChatComponent())
		{
			BaseChatChannel defaultChannel = m_ChatEntity.GetDefaultChannel();
			if (defaultChannel)
				SetActiveChannel(defaultChannel);
		}
		super.Internal_Open();
	}

	//------------------------------------------------------------------------------------------------
	override protected SCR_ChatMessageStyle GetChannelStyle(BaseChatChannel channel)
	{
		if (!channel || !GC_LobbyManager.GetManager())
			return super.GetChannelStyle(channel);

		switch (channel.Type())
		{
			case GC_LobbyFactionChatChannel:
				return m_FactionChannelStyle;
			case GC_LobbyGroupChatChannel:
				return m_GroupChannelStyle;
			case GC_LobbyVehicleChatChannel:
				return m_VehicleChannelStyle;
			case GC_LobbyLocalChatChannel:
				return m_LocalChannelStyle;
			case GC_LobbyAdminChatChannel:
				if (!m_GC_AdminChannelStyle)
					m_GC_AdminChannelStyle = GC_CreateChannelStyle("Admin", "6", 0.86, 0.7, 0.37);
				return m_GC_AdminChannelStyle;
			case GC_LobbySpectatorChatChannel:
				if (!m_GC_SpectatorChannelStyle)
					m_GC_SpectatorChannelStyle = GC_CreateChannelStyle("Spectator", "7", 0.57, 0.76, 0.83);
				return m_GC_SpectatorChannelStyle;
		}

		return super.GetChannelStyle(channel);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_ChatMessageStyle GC_CreateChannelStyle(string name, string prefix, float red, float green, float blue)
	{
		SCR_ChatMessageStyle style = new SCR_ChatMessageStyle();
		style.m_Color = new Color(red, green, blue, 1);
		style.m_sName = name;
		name.ToLower();
		style.m_sNameLower = name;
		style.m_sPrefix = prefix;
		style.m_bColoredBadge = true;
		style.m_bColoredIcon = true;
		style.m_bColoredPlayerName = true;
		style.m_IconImageset = m_GlobalChannelStyle.m_IconImageset;
		style.m_sIconName = m_GlobalChannelStyle.m_sIconName;
		return style;
	}
}
