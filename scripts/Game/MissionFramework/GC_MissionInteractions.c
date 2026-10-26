//! Native user actions perform the same checks again on authority.
class GC_MissionInteraction : ScriptedUserAction
{
	[Attribute("Interact", UIWidgets.EditBox, "Text shown to the player.")]
	string m_sTitle;
	[Attribute("", UIWidgets.EditBox, "Allowed faction keys. Empty allows everyone.")]
	ref array<string> m_aFactions;
	[Attribute("", UIWidgets.EditBox, "Optional flag that must be set before this interaction is available.")]
	string m_sRequiredFlag;

	//------------------------------------------------------------------------------------------------
	protected bool IsAvailable(IEntity user)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || manager.GetLobbyPhase() != SCR_EGameModeState.GAME || !user || !GetOwner()
			|| !SCR_AIDamageHandling.IsAlive(user) || vector.Distance(user.GetOrigin(), GetOwner().GetOrigin()) > 5)
			return false;
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user);
		if (playerId <= 0 || (!m_sRequiredFlag.IsEmpty() && !manager.GC_IsMissionFlagSet(m_sRequiredFlag)))
			return false;
		if (m_aFactions && !m_aFactions.IsEmpty())
		{
			SCR_FactionManager factions = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = factions.GetPlayerFaction(playerId);
			if (!faction || !m_aFactions.Contains(faction.GetFactionKey()))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return IsAvailable(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return IsAvailable(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = m_sTitle;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return true;
	}
}

class GC_SetFlagInteraction : GC_MissionInteraction
{
	[Attribute("", UIWidgets.EditBox, "Flag to set. Must be declared in Mission Flags.")]
	string m_sFlag;
	[Attribute("1", UIWidgets.CheckBox, "Hide while this flag is already set. Clearing the flag makes the interaction available again.")]
	bool m_bSingleUse;
	[Attribute("0", UIWidgets.CheckBox, "After success, delete this interaction's parent and its children.")]
	bool m_bDeleteParent;

	//------------------------------------------------------------------------------------------------
	override protected bool IsAvailable(IEntity user)
	{
		if (!super.IsAvailable(user))
			return false;
		return !m_bSingleUse || !GC_LobbyManager.GetManager().GC_IsMissionFlagSet(m_sFlag);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || pOwnerEntity != GetOwner() || !IsAvailable(pUserEntity))
			return;
		GC_MissionRuntime runtime = GC_LobbyManager.GetManager().GC_GetMissionRuntime();
		bool applied = runtime && runtime.SetFlag(m_sFlag, true);
		PrintFormat("GC | Mission Framework | interaction: title=%1 flag=%2 applied=%3", m_sTitle, m_sFlag, applied);
		if (applied && m_bDeleteParent && pOwnerEntity.GetParent())
		{
			GC_LobbyMissionContext context = new GC_LobbyMissionContext(GC_LobbyManager.GetManager());
			context.Bind(runtime);
			if (context.CanDelete(pOwnerEntity.GetParent()))
				SCR_EntityHelper.DeleteEntityAndChildren(pOwnerEntity.GetParent());
		}
	}
}

class GC_TeleportInteraction : GC_MissionInteraction
{
	[Attribute("", UIWidgets.EditBox, "Name of the destination entity.")]
	string m_sDestination;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || pOwnerEntity != GetOwner() || !IsAvailable(pUserEntity))
			return;
		IEntity destination = GetGame().GetWorld().FindEntityByName(m_sDestination);
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		bool applied = destination && controller;
		if (applied)
			controller.GC_ApplyMissionTeleport(destination.GetOrigin());
		PrintFormat("GC | Mission Framework | teleport interaction: destination=%1 player=%2 dispatched=%3", m_sDestination, playerId, applied);
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	void GC_ApplyMissionTeleport(vector position)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity character = GetControlledEntity();
		if (!Replication.IsServer() || !manager || manager.GetLobbyPhase() != SCR_EGameModeState.GAME || !character)
			return;
		Rpc(GC_RpcDo_MissionTeleport, Replication.FindItemId(character), position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_MissionTeleport(RplId characterId, vector position)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity character = GetControlledEntity();
		if (GetGame().GetPlayerController() != this || !manager || manager.GetLobbyPhase() != SCR_EGameModeState.GAME
			|| !character || characterId == RplId.Invalid() || Replication.FindItemId(character) != characterId)
			return;
		SCR_Global.TeleportPlayer(GetPlayerId(), position);
	}
}
