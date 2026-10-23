//! Shared engine hook: no persistent state and no change outside an active
//! Lobby2 freeze. Preserve the inherited control pipeline in every mode.
modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (player && freeze && freeze.IsFreezeActive())
			freeze.FilterCombatActions(am);
		super.OnPrepareControls(owner, am, dt, player);
	}
}
