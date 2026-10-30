[ComponentEditorProps(category: "Lobby2/Player", description: "Makes the Lobby2 spectator body invisible on every machine")]
class GC_LobbySpectatorBodyComponentClass : ScriptComponentClass
{
}

class GC_LobbySpectatorBodyComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	protected void HideBody()
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
			physics.SetInteractionLayer(0);
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
		if (damage && Replication.IsServer())
			damage.EnableDamageHandling(false);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(HideBody, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(HideBody);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
}
