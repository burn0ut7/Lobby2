//! Native chat uses these channels only in Lobby2's chat entity prefab.
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

class GC_LobbySpectatorChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.IsSpectator(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool ProcessMessage(BaseChatComponent sender, string message, bool isAuthority)
	{
		return GC_LobbyChatMembership.IsSpectator(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		return GC_LobbyChatMembership.IsSpectator(sender) && GC_LobbyChatMembership.IsSpectator(receiver);
	}
}

class GC_LobbyFactionChatChannel : FactionChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.IsSpectator(sender) || super.IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		if (GC_LobbyChatMembership.IsSpectator(sender) || GC_LobbyChatMembership.IsSpectator(receiver))
			return true;

		return super.IsDelivering(sender, receiver);
	}
}

class GC_LobbyGroupChatChannel : GroupChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.IsSpectator(sender) || super.IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		if (GC_LobbyChatMembership.IsSpectator(sender) || GC_LobbyChatMembership.IsSpectator(receiver))
			return true;

		return super.IsDelivering(sender, receiver);
	}
}

//! Native VehicleChatChannel transport rejects non-occupants despite these callbacks.
//! Use native compartment membership directly so spectators can participate.
class GC_LobbyVehicleChatChannel : BaseChatChannel
{
	//------------------------------------------------------------------------------------------------
	protected IEntity GetVehicle(BaseChatComponent component)
	{
		GC_LobbyPlayerController player = GC_LobbyChatMembership.GetPlayer(component);
		if (!player)
			return null;
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || !character.IsInVehicle())
			return null;
		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access)
			return null;

		return access.GetVehicleIn(character);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.IsSpectator(sender) || GetVehicle(sender) != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		if (GC_LobbyChatMembership.IsSpectator(sender) || GC_LobbyChatMembership.IsSpectator(receiver))
			return true;

		IEntity senderVehicle = GetVehicle(sender);
		return senderVehicle && senderVehicle == GetVehicle(receiver);
	}
}

class GC_LobbyLocalChatChannel : LocalChatChannel
{
	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(BaseChatComponent sender)
	{
		return GC_LobbyChatMembership.IsSpectator(sender) || super.IsAvailable(sender);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsDelivering(BaseChatComponent sender, BaseChatComponent receiver)
	{
		if (GC_LobbyChatMembership.IsSpectator(sender) || GC_LobbyChatMembership.IsSpectator(receiver))
			return true;

		return super.IsDelivering(sender, receiver);
	}
}
