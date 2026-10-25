enum GC_EContextTargetType
{
	FACTION,
	GROUP,
	SLOT,
	PLAYER,
	ENTITY,
	VOICE_CHANNEL,
	MODERATION_PLAYER
}

//! Ephemeral, client-local context for one menu invocation. References point
//! back to authoritative base-game or Lobby2 objects and are never replicated.
class GC_ContextMenuContext
{
	protected GC_EContextTargetType m_eTargetType;
	protected FactionKey m_FactionKey;
	protected SCR_AIGroup m_Group;
	protected GC_Slot m_Slot;
	protected int m_iLotteryId;
	protected int m_iPlayerId = -1;
	protected UUID m_PlayerIdentity;
	protected string m_sModerationName;
	protected GC_AdminModerationComponent m_ModerationForm;
	protected IEntity m_Entity;
	protected GC_VoiceChannelType m_eVoiceChannelType;
	protected UUID m_VoicePersonalOwnerIdentity;
	protected int m_iVoicePersonalOwnerPlayerId = -1;
	protected int m_iRequesterPlayerId = -1;
	protected int m_iSelectedPlayerId = -1;
	protected UUID m_SelectedIdentity;

	protected void CaptureRequester()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			m_iSelectedPlayerId = manager.GetAdminSelectedPlayer();
			m_SelectedIdentity = manager.GetPlayerVoiceIdentity(m_iSelectedPlayerId);
		}
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
			m_iRequesterPlayerId = playerController.GetPlayerId();
	}

	static GC_ContextMenuContext ForFaction(FactionKey factionKey)
	{
		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.FACTION;
		context.m_FactionKey = factionKey;
		context.CaptureRequester();
		return context;
	}

	static GC_ContextMenuContext ForGroup(SCR_AIGroup group)
	{
		if (!group)
			return null;

		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.GROUP;
		context.m_Group = group;
		context.CaptureRequester();
		return context;
	}

	static GC_ContextMenuContext ForSlot(GC_Slot slot)
	{
		if (!slot)
			return null;

		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.SLOT;
		context.m_Slot = slot;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			GC_SlotLottery lottery = manager.GetLotteryForSlot(slot.GetID());
			if (lottery)
				context.m_iLotteryId = lottery.m_iId;
		}
		context.m_iPlayerId = slot.GetPlayerId();
		context.m_PlayerIdentity = slot.GetUUID();
		context.CaptureRequester();
		return context;
	}

	//------------------------------------------------------------------------------------------------
	static GC_ContextMenuContext ForPlayer(int playerId, IEntity entity = null)
	{
		if (playerId < 0)
			return null;

		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.PLAYER;
		context.m_iPlayerId = playerId;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			context.m_PlayerIdentity = manager.GetPlayerVoiceIdentity(playerId);
			context.m_Slot = manager.GetSlotByPlayerId(playerId);
		}
		context.m_Entity = entity;
		context.CaptureRequester();
		return context;
	}

	//------------------------------------------------------------------------------------------------
	static GC_ContextMenuContext ForModerationPlayer(UUID identity, string name, GC_AdminModerationComponent form)
	{
		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.MODERATION_PLAYER;
		context.m_PlayerIdentity = identity;
		context.m_sModerationName = name;
		context.m_ModerationForm = form;
		context.CaptureRequester();
		return context;
	}

	//------------------------------------------------------------------------------------------------
	GC_AdminModerationComponent GetModerationForm()
	{
		return m_ModerationForm;
	}

	//------------------------------------------------------------------------------------------------
	static GC_ContextMenuContext ForEntity(IEntity entity)
	{
		if (!entity)
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(entity);
			if (playerId > 0)
				return ForPlayer(playerId, entity);
		}

		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.ENTITY;
		context.m_Entity = entity;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			context.m_Slot = manager.GetSlotByEntity(entity);
		if (context.m_Slot)
		{
			context.m_iPlayerId = context.m_Slot.GetPlayerId();
			context.m_PlayerIdentity = context.m_Slot.GetUUID();
		}
		context.CaptureRequester();
		return context;
	}

	static GC_ContextMenuContext ForVoiceChannel(GC_VoiceChannelType channelType, FactionKey factionKey, SCR_AIGroup group, UUID personalOwnerIdentity, int personalOwnerPlayerId)
	{
		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.VOICE_CHANNEL;
		context.m_eVoiceChannelType = channelType;
		context.m_FactionKey = factionKey;
		context.m_Group = group;
		context.m_VoicePersonalOwnerIdentity = personalOwnerIdentity;
		context.m_iVoicePersonalOwnerPlayerId = personalOwnerPlayerId;
		context.CaptureRequester();
		return context;
	}

	//------------------------------------------------------------------------------------------------
	int GetSelectedPlayerId()
	{
		return m_iSelectedPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	UUID GetSelectedIdentity()
	{
		return m_SelectedIdentity;
	}

	GC_EContextTargetType GetTargetType()
	{
		return m_eTargetType;
	}

	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}

	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}

	//------------------------------------------------------------------------------------------------
	int GetLotteryId()
	{
		return m_iLotteryId;
	}

	//------------------------------------------------------------------------------------------------
	GC_Slot GetSlot()
	{
		return m_Slot;
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	UUID GetPlayerIdentity()
	{
		return m_PlayerIdentity;
	}

	IEntity GetEntity()
	{
		return m_Entity;
	}

	GC_VoiceChannelType GetVoiceChannelType()
	{
		return m_eVoiceChannelType;
	}

	UUID GetVoicePersonalOwnerIdentity()
	{
		return m_VoicePersonalOwnerIdentity;
	}

	int GetVoicePersonalOwnerPlayerId()
	{
		return m_iVoicePersonalOwnerPlayerId;
	}

	int GetRequesterPlayerId()
	{
		return m_iRequesterPlayerId;
	}

	//! Player whose name is embedded in this context label, if any.
	int GetLabelPlayerId()
	{
		if (m_eTargetType == GC_EContextTargetType.PLAYER)
			return m_iPlayerId;

		if (m_eTargetType == GC_EContextTargetType.VOICE_CHANNEL && m_eVoiceChannelType == GC_VoiceChannelType.PERSONAL)
			return m_iVoicePersonalOwnerPlayerId;

		return -1;
	}

	string GetTargetTypeName()
	{
		switch (m_eTargetType)
		{
			case GC_EContextTargetType.FACTION:
				return "FACTION";
			case GC_EContextTargetType.GROUP:
				return "GROUP";
			case GC_EContextTargetType.SLOT:
				return "SLOT";
			case GC_EContextTargetType.PLAYER:
				return "PLAYER";
			case GC_EContextTargetType.MODERATION_PLAYER:
				return "PLAYER";
			case GC_EContextTargetType.ENTITY:
				return "ENTITY";
			case GC_EContextTargetType.VOICE_CHANNEL:
				return "VOICE CHANNEL";
		}

		return "CONTEXT";
	}

	string GetTargetLabel()
	{
		if (m_eTargetType == GC_EContextTargetType.MODERATION_PLAYER)
			return m_sModerationName;
		switch (m_eTargetType)
		{
			case GC_EContextTargetType.FACTION:
				return m_FactionKey;
			case GC_EContextTargetType.GROUP:
				return GC_GroupCallsignHelper.GetDisplayName(m_Group);
			case GC_EContextTargetType.SLOT:
				return string.Format("SLOT %1", m_Slot.GetID());
			case GC_EContextTargetType.PLAYER:
			{
				PlayerManager playerManager = GetGame().GetPlayerManager();
				if (playerManager)
				{
					string playerName = playerManager.GetPlayerName(m_iPlayerId);
					if (!playerName.IsEmpty())
						return playerName;
				}

				return string.Format("PLAYER %1", m_iPlayerId);
			}
			case GC_EContextTargetType.ENTITY:
				return "ENTITY";
			case GC_EContextTargetType.VOICE_CHANNEL:
			{
				switch (m_eVoiceChannelType)
				{
					case GC_VoiceChannelType.GLOBAL:
						return "GLOBAL";
					case GC_VoiceChannelType.FACTION:
						if (!m_FactionKey.IsEmpty())
							return string.Format("%1 HQ", m_FactionKey);
						return "FACTION HQ";
					case GC_VoiceChannelType.GROUP:
						return GC_GroupCallsignHelper.GetDisplayName(m_Group);
					case GC_VoiceChannelType.PERSONAL:
					{
						PlayerManager playerManager = GetGame().GetPlayerManager();
						if (playerManager)
						{
							string playerName = playerManager.GetPlayerName(m_iVoicePersonalOwnerPlayerId);
							if (!playerName.IsEmpty())
								return string.Format("%1'S CHANNEL", playerName);
						}

						return "PERSONAL CHANNEL";
					}
				}

				return "VOICE CHANNEL";
			}
		}

		return string.Empty;
	}
}

//! One executable menu entry. Role/admin policy belongs in IsVisible and
//! CanExecute; authoritative mutations must still route through their owner RPC.
class GC_ContextMenuAction
{
	//! Empty labels leave the action ungrouped. Actions sharing a non-empty label
	//! are rendered together under one subtle section header.
	string GetGroupLabel(GC_ContextMenuContext context)
	{
		return string.Empty;
	}

	string GetLabel(GC_ContextMenuContext context)
	{
		return string.Empty;
	}

	ResourceName GetIcon(GC_ContextMenuContext context)
	{
		return string.Empty;
	}

	string GetIconQuad(GC_ContextMenuContext context)
	{
		return string.Empty;
	}

	bool IsVisible(GC_ContextMenuContext context)
	{
		return context != null;
	}

	bool CanExecute(GC_ContextMenuContext context)
	{
		return context != null;
	}

	void Execute(GC_ContextMenuContext context)
	{
	}
}

class GC_ContextMenuPrintTestAction : GC_ContextMenuAction
{
	override string GetLabel(GC_ContextMenuContext context)
	{
		return "PRINT TEST";
	}

	override void Execute(GC_ContextMenuContext context)
	{
		if (!context)
			return;

		PrintFormat(
			"GC | Context | PRINT TEST | requester=%1 | target=%2 | label=%3",
			context.GetRequesterPlayerId(),
			context.GetTargetTypeName(),
			context.GetTargetLabel(),
			level: LogLevel.NORMAL
		);
	}
}

//! Shared action catalogue. Components may register owned action scripts and
//! must unregister them when detached. Built-in actions check the active mode.
class GC_ContextMenuActionRegistry
{
	protected static ref array<ref GC_ContextMenuAction> s_aActions;

	protected static void EnsureInitialized()
	{
		if (s_aActions)
			return;

		s_aActions = {};
		s_aActions.Insert(new GC_FactionInfoAction());
		s_aActions.Insert(new GC_SlotLotteryAction(false));
		s_aActions.Insert(new GC_SlotLotteryAction(true));
		s_aActions.Insert(new GC_JoinGroupVoiceAction());
		s_aActions.Insert(new GC_AdminCopyPlayerDetailsAction());
		s_aActions.Insert(new GC_AdminRemoveBanAction());
		s_aActions.Insert(new GC_AdminMakePlayableAction());
		s_aActions.Insert(new GC_AdminOpenToolsAction());
		s_aActions.Insert(new GC_AdminSendMessageAction());
		s_aActions.Insert(new GC_AdminMoveSelectedAction());
		s_aActions.Insert(new GC_AdminRemoveFactionLockAction());
		s_aActions.Insert(new GC_AdminVoiceAction(true));
		s_aActions.Insert(new GC_AdminVoiceAction(false));
		s_aActions.Insert(new GC_AdminReserveSlotAction());
		s_aActions.Insert(new GC_AdminSlotToggleAction());
		s_aActions.Insert(new GC_AdminScopeSlotLockAction(true));
		s_aActions.Insert(new GC_AdminScopeSlotLockAction(false));
		s_aActions.Insert(new GC_AdminSlotPlayerAction(GC_EAdminSlotAction.FREE));
		s_aActions.Insert(new GC_AdminSlotPlayerAction(GC_EAdminSlotAction.HEAL_OR_RESPAWN));
	}

	static void Register(GC_ContextMenuAction action)
	{
		if (!action)
			return;

		EnsureInitialized();
		if (!s_aActions.Contains(action))
			s_aActions.Insert(action);
	}

	static void Unregister(GC_ContextMenuAction action)
	{
		if (s_aActions && action)
			s_aActions.RemoveItem(action);
	}

	static void CollectVisible(GC_ContextMenuContext context, notnull out array<ref GC_ContextMenuAction> actions)
	{
		EnsureInitialized();
		actions.Clear();
		if (context && context.GetTargetType() == GC_EContextTargetType.MODERATION_PLAYER)
		{
			foreach (GC_ContextMenuAction copy : s_aActions)
			{
				if ((GC_AdminCopyPlayerDetailsAction.Cast(copy) || GC_AdminRemoveBanAction.Cast(copy))
					&& copy.IsVisible(context) && copy.CanExecute(context))
					actions.Insert(copy);
			}
			return;
		}
		foreach (GC_ContextMenuAction action : s_aActions)
		{
			// Only actionable entries participate in menu rows, groups and sizing.
			if (action && action.IsVisible(context) && action.CanExecute(context))
				actions.Insert(action);
		}
	}
}
