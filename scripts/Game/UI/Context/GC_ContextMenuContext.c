enum GC_EContextTargetType
{
	FACTION,
	GROUP,
	SLOT,
	PLAYER,
	ENTITY,
	VOICE_CHANNEL
}

//! Ephemeral, client-local context for one menu invocation. References point
//! back to authoritative base-game or Lobby2 objects and are never replicated.
class GC_ContextMenuContext
{
	protected GC_EContextTargetType m_eTargetType;
	protected FactionKey m_FactionKey;
	protected SCR_AIGroup m_Group;
	protected GC_Slot m_Slot;
	protected int m_iPlayerId = -1;
	protected IEntity m_Entity;
	protected GC_VoiceChannelType m_eVoiceChannelType;
	protected UUID m_VoicePersonalOwnerIdentity;
	protected int m_iVoicePersonalOwnerPlayerId = -1;
	protected int m_iRequesterPlayerId = -1;

	protected void CaptureRequester()
	{
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
		context.CaptureRequester();
		return context;
	}

	static GC_ContextMenuContext ForPlayer(int playerId, IEntity entity = null)
	{
		if (playerId < 0)
			return null;

		GC_ContextMenuContext context = new GC_ContextMenuContext();
		context.m_eTargetType = GC_EContextTargetType.PLAYER;
		context.m_iPlayerId = playerId;
		context.m_Entity = entity;
		context.CaptureRequester();
		return context;
	}

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

	GC_Slot GetSlot()
	{
		return m_Slot;
	}

	int GetPlayerId()
	{
		return m_iPlayerId;
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
			case GC_EContextTargetType.ENTITY:
				return "ENTITY";
			case GC_EContextTargetType.VOICE_CHANNEL:
				return "VOICE CHANNEL";
		}

		return "CONTEXT";
	}

	string GetTargetLabel()
	{
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
//! must unregister them when detached; the print action is the built-in slice.
class GC_ContextMenuActionRegistry
{
	protected static ref array<ref GC_ContextMenuAction> s_aActions;

	protected static void EnsureInitialized()
	{
		if (s_aActions)
			return;

		s_aActions = {};
		s_aActions.Insert(new GC_ContextMenuPrintTestAction());
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
		foreach (GC_ContextMenuAction action : s_aActions)
		{
			if (action && action.IsVisible(context))
				actions.Insert(action);
		}
	}
}
