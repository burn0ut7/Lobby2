[ComponentEditorProps(category: "Lobby2/Player", description: "Disables visibility and physics on the dedicated spectator body")]
class GC_LobbySpectatorBodyComponentClass : ScriptComponentClass
{
}

class GC_LobbySpectatorBodyComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	protected void ConfigureSpectatorBody()
	{
		if (!GC_LobbyManager.GetManager())
			return;
		// Replicated character initialization restores visibility after EOnInit.
		// The call queue also runs for remote proxies without frame callbacks.
		GetOwner().ClearFlags(EntityFlags.VISIBLE, true);
		// This dedicated body is a voice listener, never a physical participant.
		CharacterAnimationComponent animation = CharacterAnimationComponent.Cast(GetOwner().FindComponent(CharacterAnimationComponent));
		if (animation)
			animation.PhysicsEnableGravity(false);
		Physics physics = GetOwner().GetPhysics();
		if (physics)
		{
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
			physics.EnableGravity(false);
			physics.ChangeSimulationState(SimulationState.NONE);
			physics.SetActive(ActiveState.INACTIVE);
			physics.SetInteractionLayer(0);
			for (int i; i < physics.GetNumGeoms(); i++)
				physics.SetGeomInteractionLayer(i, 0);
		}
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
		if (damage && Replication.IsServer())
			damage.EnableDamageHandling(false);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(ConfigureSpectatorBody, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(ConfigureSpectatorBody);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
}
