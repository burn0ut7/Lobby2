//! Explicit, time-bounded native movement input for hard-boundary regression checks.
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_AOTestWalkUntil;

	//------------------------------------------------------------------------------------------------
	void GC_TestAOWalk(float seconds)
	{
		m_fGC_AOTestWalkUntil = GetGame().GetWorld().GetWorldTime() + seconds * 1000;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (GC_LobbyGameMode.GetGameMode() && player && GetGame().GetPlayerController()
			&& GetGame().GetPlayerController().GetControlledEntity() == owner
			&& GetGame().GetWorld().GetWorldTime() < m_fGC_AOTestWalkUntil)
			am.SetActionValue("CharacterForward", 1);
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAOContinuousWalk(float seconds)
	{
		if (seconds < 0 || seconds > 20)
			return "REJECTED duration";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		controller.GC_TestAOWalk(seconds);
		return string.Format("walkSeconds=%1 position=%2", seconds, character.GetOrigin());
	}
}
