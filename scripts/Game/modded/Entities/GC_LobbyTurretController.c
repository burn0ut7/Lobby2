modded class SCR_TurretControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (player && freeze)
			freeze.FilterCombatActions(am);

		super.OnPrepareControls(owner, am, dt, player);
	}
}
