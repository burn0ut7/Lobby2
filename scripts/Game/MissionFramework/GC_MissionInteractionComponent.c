[ComponentEditorProps(category: "Lobby2/Mission", description: "Reusable mission interaction. Add GC_MissionInteractionAction to an ActionsManager action slot on this same entity. No nested condition lists.")]
class GC_MissionInteractionComponentClass : ScriptComponentClass
{
}

class GC_MissionInteractionComponent : ScriptComponent
{
	[Attribute("Interact", UIWidgets.EditBox, "Text shown in the normal interaction menu, for example Activate radio.")]
	protected string m_sActionName;
	[Attribute("", UIWidgets.EditBox, "Optional faction keys allowed to use this interaction. Empty means all factions. Only living, conscious player-controlled characters can use it; spectators cannot.")]
	protected ref array<string> m_aFactions;
	[Attribute("", UIWidgets.EditBox, "Optional Mission Flag that must currently be On. Keep complex requirements on that flag in GC Lobby Manager, not on this object. Empty means no flag requirement. Available during Match only.")]
	protected string m_sRequiredFlag;
	[Attribute("", UIWidgets.EditBox, "Optional Mission Flag to call after use. Its own conditions and repeatability still apply; a rejected flag call does not undo the interaction. Leave empty when using Interaction Condition only.")]
	protected string m_sCallFlag;
	[RplProp()]
	protected bool m_bAvailable;
	protected bool m_bUsing;
	protected ref ScriptInvoker m_OnUsed = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	string GetActionName()
	{
		return m_sActionName;
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnUsed()
	{
		return m_OnUsed;
	}

	//------------------------------------------------------------------------------------------------
	bool CanUse(IEntity user)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || manager.GetLobbyPhase() != SCR_EGameModeState.GAME || m_bUsing || !GetOwner().FindComponent(RplComponent))
			return false;
		if (Replication.IsServer())
			RefreshAvailability();
		if (!m_bAvailable)
			return false;
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(user);
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return false;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.IsDead() || controller.IsUnconscious())
			return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;
		int playerId = players.GetPlayerIdFromControlledEntity(user);
		if (playerId <= 0 || !players.IsPlayerConnected(playerId))
			return false;
		return !m_aFactions || m_aFactions.IsEmpty() || m_aFactions.Contains(character.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	//! Called only through a native server-verified user action, never a custom client RPC.
	bool TryUse(IEntity user)
	{
		if (!Replication.IsServer() || !CanUse(user))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		m_bUsing = true;
		m_OnUsed.Invoke();
		manager.NotifyMissionInteraction(m_sCallFlag);
		m_bUsing = false;
		RefreshAvailability();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshAvailability()
	{
		if (!Replication.IsServer())
			return;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		bool available = manager && manager.AreMissionInteractionsActive() && !m_sActionName.IsEmpty()
			&& (m_sRequiredFlag.IsEmpty() || manager.GetFlag(m_sRequiredFlag));
		if (available == m_bAvailable)
			return;
		m_bAvailable = available;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (SCR_Global.IsEditMode())
			return;
		if (Replication.IsServer())
		{
			GC_LobbyManager.GetMissionStateChanged().Insert(RefreshAvailability);
			RefreshAvailability();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GC_LobbyManager.GetMissionStateChanged().Remove(RefreshAvailability);
		super.OnDelete(owner);
	}
}
