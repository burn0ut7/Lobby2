//! Owns native movement-control restrictions installed by this mission.
class GC_MissionAISettings
{
	protected ref map<CharacterControllerComponent, bool> m_mHeld = new map<CharacterControllerComponent, bool>();

	//------------------------------------------------------------------------------------------------
	bool HoldPosition(IEntity entity, bool hold)
	{
		if (!entity)
			return false;
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
		AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!controller || !control || !control.GetAIAgent())
			return false;
		if (!hold)
		{
			if (m_mHeld.Contains(controller) && !m_mHeld.Get(controller))
				controller.SetDisableMovementControls(false);
			m_mHeld.Remove(controller);
			return true;
		}
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity) > 0)
			return false;
		if (m_mHeld.Contains(controller))
			return true;
		m_mHeld.Set(controller, controller.GetDisableMovementControls());
		controller.SetDisableMovementControls(true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A player taking over an AI must not inherit this mission's movement hold.
	void ReleasePlayerControlled()
	{
		array<CharacterControllerComponent> released = {};
		foreach (CharacterControllerComponent controller, bool previouslyDisabled : m_mHeld)
		{
			if (!controller)
			{
				released.Insert(controller);
				continue;
			}
			if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(controller.GetOwner()) <= 0)
				continue;
			if (!previouslyDisabled)
				controller.SetDisableMovementControls(false);
			released.Insert(controller);
		}
		foreach (CharacterControllerComponent controller : released)
			m_mHeld.Remove(controller);
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		foreach (CharacterControllerComponent controller, bool previouslyDisabled : m_mHeld)
		{
			if (controller && !previouslyDisabled)
				controller.SetDisableMovementControls(false);
		}
		m_mHeld.Clear();
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set AI Hold Position")]
class GC_SetAIHoldPositionAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of AI characters. Disables native movement controls; AI perception and weapon controls remain active.")]
	ref array<string> m_aEntities;
	[Attribute("1", UIWidgets.CheckBox, "Hold position. Off releases the hold. Pre-existing movement restrictions are preserved.")]
	bool m_bHold;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aEntities && !m_aEntities.IsEmpty())
			return true;
		error = "Set AI Hold Position: add AI character names.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive() || !context.GetRuntime())
			return false;
		bool applied = true;
		foreach (string name : m_aEntities)
		{
			if (!context.GetRuntime().SetAIHoldPosition(GetGame().GetWorld().FindEntityByName(name), m_bHold))
				applied = false;
		}
		return applied;
	}
}
