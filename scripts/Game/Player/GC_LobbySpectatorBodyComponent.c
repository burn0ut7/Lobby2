[ComponentEditorProps(category: "Lobby2/Player", description: "Makes the Lobby2 spectator body invisible on every machine")]
class GC_LobbySpectatorBodyComponentClass : ScriptComponentClass
{
}

class GC_LobbySpectatorBodyComponent : ScriptComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		owner.ClearFlags(EntityFlags.VISIBLE, true);
	}
}
