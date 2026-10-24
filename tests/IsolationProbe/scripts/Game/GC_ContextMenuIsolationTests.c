#ifdef WORKBENCH
modded class SCR_ContextMenuActionsEditorUIComponent
{
	//------------------------------------------------------------------------------------------------
	//! Observe the native rendered actions without depending on any Lobby2 types.
	string GC_TestIsolationMenu(IEntity entity, bool heal)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(entity);
		SCR_BaseEditableEntityFilter hover = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.HOVER);
		SCR_BaseEditableEntityFilter selected = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.SELECTED);
		if (!editable || !hover || !selected)
			return "UNAVAILABLE native editable filters";
		hover.Replace(editable);
		selected.Replace(editable);
		OnOpenActionsMenuDown();
		OpenContextMenu();
		string result = string.Format("menuOpen=%1 actions=", IsContextMenuOpen());
		Widget healButton;
		foreach (Widget button, SCR_BaseEditorAction action : m_ButtonActions)
		{
			result += action.Type().ToString() + ";";
			if (SCR_HealEntitiesContextAction.Cast(action))
				healButton = button;
		}
		if (heal)
		{
			if (!healButton || !healButton.IsEnabled())
				return result + " UNAVAILABLE stock heal";
			OnContextMenuOptionClicked(healButton, 1, EActionTrigger.DOWN);
			result += " stockHealDispatched=1";
		}
		return result;
	}
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string ContextMenu(string operation)
	{
		if (operation == "editor")
			return string.Format("editorOpen=%1", SCR_EditorManagerEntity.OpenInstance());
		IEntity entity = GetGame().GetWorld().FindEntityByName("IsolationCharacter");
		if (!entity)
			return "UNAVAILABLE character";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "UNAVAILABLE damage component";
		if (operation == "damage")
			damage.GetDefaultHitZone().SetHealthScaled(0.6);
		if (operation == "damage" || operation == "snapshot")
			return string.Format("health=%1", damage.GetHealthScaled());
		array<Widget> widgets = {GetGame().GetWorkspace()};
		for (int i; i < widgets.Count(); i++)
		{
			Widget widget = widgets[i];
			SCR_ContextMenuActionsEditorUIComponent context = SCR_ContextMenuActionsEditorUIComponent.Cast(widget.FindHandler(SCR_ContextMenuActionsEditorUIComponent));
			if (context)
				return context.GC_TestIsolationMenu(entity, operation == "heal");
			Widget child = widget.GetChildren();
			while (child)
			{
				widgets.Insert(child);
				child = child.GetSibling();
			}
		}
		return "UNAVAILABLE native menu";
	}
}
#endif
