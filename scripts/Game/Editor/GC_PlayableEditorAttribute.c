//! Adds Lobby2 slot registration to the existing Game Master properties dialog.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class GC_PlayableEditorAttribute : SCR_BaseEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override bool IsSerializable()
	{
		// Claims and slot entities belong to the lobby session, not editor saves.
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		GC_LobbyManager lobby = GC_LobbyManager.GetManager();
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!lobby || !editable)
			return null;

		array<IEntity> characters;
		if (!lobby.GetEditorPlayableCharacters(editable.GetOwner(), characters))
			return null;

		foreach (IEntity character : characters)
		{
			if (!lobby.GetSlotByEntity(character))
				return SCR_BaseEditorAttributeVar.CreateBool(false);
		}
		return SCR_BaseEditorAttributeVar.CreateBool(true);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		GC_LobbyManager lobby = GC_LobbyManager.GetManager();
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!lobby || !Replication.IsServer() || !editable || !var || !manager)
			return;

		SCR_EditorManagerEntity editor = manager.GetManager();
		if (!editor)
			return;

		// Do not authorize from the playerID argument carried in the native RPC.
		lobby.ExecuteEditorPlayable(editor.GetPlayerID(), editable.GetOwner(), var.GetBool());
	}
}
