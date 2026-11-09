//! Put this action in any native action context on the component's entity.
class GC_MissionInteractionAction : ScriptedUserAction
{
	protected GC_MissionInteractionComponent m_Interaction;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Interaction = GC_MissionInteractionComponent.Cast(pOwnerEntity.FindComponent(GC_MissionInteractionComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_Interaction)
			return false;
		outName = m_Interaction.GetActionName();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return m_Interaction && m_Interaction.CanUse(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return m_Interaction && m_Interaction.CanUse(user);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (Replication.IsServer() && m_Interaction && pOwnerEntity == GetOwner() && IsNearContext(pUserEntity))
			m_Interaction.TryUse(pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Native actions own exact reach/visibility checks. Also reject distant requests on the server.
	//! Same origin/range bound as SCR_InteractionHandlerComponent.CanContextChange, including its tolerance.
	protected bool IsNearContext(IEntity user)
	{
		ActionsManagerComponent manager = GetActionsManager();
		if (!manager || !user)
			return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;
		PlayerController controller = players.GetPlayerController(players.GetPlayerIdFromControlledEntity(user));
		if (!controller)
			return false;
		InteractionHandlerComponent handler = InteractionHandlerComponent.Cast(controller.FindComponent(InteractionHandlerComponent));
		if (!handler)
			return false;
		array<UserActionContext> contexts = {};
		manager.GetContextList(contexts);
		foreach (UserActionContext context : contexts)
		{
			if (!context || GetContextIndex(context.GetContextName()) < 0)
				continue;
			float range = context.GetVisibilityRange(handler.GetVisibilityRange());
			if (range > 0 && vector.DistanceSq(user.GetOrigin(), context.GetOrigin()) <= range * range * 1.1)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CheckOnServerFirstScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
