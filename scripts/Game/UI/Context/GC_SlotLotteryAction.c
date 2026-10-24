//! Existing context rows: administrators start a scope, players enter one shared draw.
class GC_SlotLotteryAction : GC_ContextMenuAction
{
	protected bool m_bStart;

	//------------------------------------------------------------------------------------------------
	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		if (m_bStart)
			return "ADMIN";
		return "LOTTERY";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisible(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!context || !manager)
			return false;
		SCR_EGameModeState phase = manager.GetLobbyPhase();
		if (phase != SCR_EGameModeState.SLOTTING && phase != SCR_EGameModeState.BRIEFING)
			return false;
		if (!m_bStart)
			return context.GetSlot() && manager.GetLotteryForSlot(context.GetSlot().GetID()) != null;
		return SCR_Global.IsAdmin(context.GetRequesterPlayerId())
			&& (context.GetTargetType() == GC_EContextTargetType.FACTION
				|| context.GetTargetType() == GC_EContextTargetType.GROUP
				|| context.GetTargetType() == GC_EContextTargetType.SLOT);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanExecute(GC_ContextMenuContext context)
	{
		if (!IsVisible(context))
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (m_bStart)
		{
			array<int> slots = {};
			int slotId = -1;
			RplId groupId = RplId.Invalid();
			if (context.GetSlot())
				slotId = context.GetSlot().GetID();
			if (context.GetGroup())
				groupId = Replication.FindItemId(context.GetGroup());
			manager.CollectLotterySlots(context.GetFactionKey(), groupId, slotId, slots);
			foreach (int targetId : slots)
			{
				if (manager.GetLotteryForSlot(targetId))
					return false;
			}
			return !slots.IsEmpty();
		}
		GC_SlotLottery lottery = manager.GetLotteryForSlot(context.GetSlot().GetID());
		return lottery.m_iId == context.GetLotteryId() && lottery.GetSecondsRemaining() > 0
			&& (lottery.m_aEntrants.Contains(manager.GetPlayerVoiceIdentity(context.GetRequesterPlayerId()))
				|| manager.CanEnterSlotLottery(context.GetRequesterPlayerId(), lottery.m_iId));
	}

	//------------------------------------------------------------------------------------------------
	override string GetLabel(GC_ContextMenuContext context)
	{
		if (m_bStart)
			return "Start lottery (30 seconds)";
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_SlotLottery lottery = manager.GetLotteryForSlot(context.GetSlot().GetID());
		if (lottery && lottery.m_aEntrants.Contains(manager.GetPlayerVoiceIdentity(context.GetRequesterPlayerId())))
			return "Leave lottery";
		return "Enter lottery";
	}

	//------------------------------------------------------------------------------------------------
	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		if (m_bStart)
		{
			int slotId = -1;
			RplId groupId = RplId.Invalid();
			if (context.GetSlot())
				slotId = context.GetSlot().GetID();
			if (context.GetGroup())
				groupId = Replication.FindItemId(context.GetGroup());
			controller.RequestStartSlotLottery(context.GetFactionKey(), groupId, slotId);
			return;
		}
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_SlotLottery lottery = manager.GetLotteryForSlot(context.GetSlot().GetID());
		bool entered = lottery.m_aEntrants.Contains(manager.GetPlayerVoiceIdentity(context.GetRequesterPlayerId()));
		controller.RequestEnterSlotLottery(lottery.m_iId, !entered);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SlotLotteryAction(bool start)
	{
		m_bStart = start;
	}
}
