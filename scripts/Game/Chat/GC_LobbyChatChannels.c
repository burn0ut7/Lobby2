//! Native chat uses these channels only in GC Lobby's chat entity prefab.
class GC_LobbyChatMembership
{
	//------------------------------------------------------------------------------------------------
	static GC_LobbyPlayerController GetPlayer(BaseChatComponent component)
	{
		if (!component || !GC_LobbyManager.GetManager())
			return null;

		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(component.GetOwner());
		if (!player || player.GetPlayerId() <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(player.GetPlayerId()))
			return null;

		return player;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsAdmin(BaseChatComponent component)
	{
		GC_LobbyPlayerController player = GetPlayer(component);
		return player && SCR_Global.IsAdmin(player.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	static bool IsSpectator(BaseChatComponent component)
	{
		GC_LobbyPlayerController player = GetPlayer(component);
		return player && player.GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR;
	}

	//------------------------------------------------------------------------------------------------
	static bool CanUseGlobalChat(BaseChatComponent component)
	{
		if (!GetPlayer(component))
			return false;

		SCR_EGameModeState phase = GC_LobbyManager.GetManager().GetLobbyPhase();
		return phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING
			|| phase == SCR_EGameModeState.BRIEFING || phase == SCR_EGameModeState.POSTGAME
			|| (phase == SCR_EGameModeState.GAME && IsSpectator(component));
	}

	//------------------------------------------------------------------------------------------------
	static bool CanUseFactionChat(BaseChatComponent component)
	{
		if (!GetPlayer(component))
			return false;

		SCR_EGameModeState phase = GC_LobbyManager.GetManager().GetLobbyPhase();
		return phase == SCR_EGameModeState.PREGAME || phase == SCR_EGameModeState.SLOTTING || phase == SCR_EGameModeState.BRIEFING;
	}
}

class GC_LobbyGlobalChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.CanUseGlobalChat(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return IsAvailable(sender) && IsAvailable(receiver);
	}
}

class GC_LobbyAdminChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.GetPlayer(sender) != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		if (!isAuthority)
			return IsAvailable(sender);

		GC_LobbyPlayerController player = GC_LobbyChatMembership.GetPlayer(sender);
		int playerId;
		if (player)
			playerId = player.GetPlayerId();

		string target = string.Format("channel=Admin length=%1", message.Length());
		if (!Replication.IsServer() || !player)
		{
			GC_AdminActionLogger.Log(playerId, "chat-send", target, "rejected-inactive-sender");
			return false;
		}

		// Sending is public; only the server grants the three-minute reply window.
		player.RenewAdminChatReceiveWindow();
		GC_AdminActionLogger.Log(playerId, "chat-send", target, "accepted-native-chat");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		GC_LobbyPlayerController player = GC_LobbyChatMembership.GetPlayer(receiver);
		return GC_LobbyChatMembership.GetPlayer(sender) && player && player.CanReceiveAdminChat();
	}
}

class GC_LobbyFactionChatChannel : FactionChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.CanUseFactionChat(sender) && super.IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return IsAvailable(sender) && IsAvailable(receiver) && super.IsDelivering(sender, receiver);
	}
}

//! Retain serialized channel identities and numeric prefixes, but disable unused routes.
class GC_LobbyGroupChatChannel : GroupChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return false;
	}
}

class GC_LobbyVehicleChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return false;
	}
}

class GC_LobbyLocalChatChannel : LocalChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return false;
	}
}

class GC_LobbySpectatorChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return false;
	}
}
