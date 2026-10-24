#ifdef WORKBENCH
modded class SCR_ContextMenuActionsEditorUIComponent
{
	//------------------------------------------------------------------------------------------------
	//! Select through native filters, render the real popup, and use its normal click handler.
	string GC_TestSlotRecoveryMenu(IEntity entity, string operation)
	{
		if (operation == "open" || operation == "open-click")
		{
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(entity);
			SCR_BaseEditableEntityFilter hover = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.HOVER);
			SCR_BaseEditableEntityFilter selected = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.SELECTED);
			if (!editable || !hover || !selected)
				return "UNAVAILABLE editable entity or native filters";
			hover.Replace(editable);
			selected.Replace(editable);
			OnOpenActionsMenuDown();
			OpenContextMenu();
		}

		int count;
		Widget target;
		foreach (Widget button, SCR_BaseEditorAction action : m_ButtonActions)
		{
			if (GC_HealSlotContextAction.Cast(action))
			{
				count++;
				target = button;
			}
		}
		bool opened = IsContextMenuOpen();
		if (operation == "click" || operation == "open-click")
		{
			if (!opened || count != 1 || !target.IsEnabled())
				return "UNAVAILABLE exactly one enabled recovery button required";
			OnContextMenuOptionClicked(target, 1, EActionTrigger.DOWN);
		}
		else if (operation != "open" && operation != "inspect")
			return "REJECTED operation";
		return string.Format("menuOpen=%1 recoveryButtons=%2 operation=%3; verify state and audit", opened, count, operation);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Compare live component inventories and the two action predicates without changing entities.
	string GC_TestGameMasterEntityComparison(int slotId, string groupName)
	{
		array<IEntity> entities = {};
		GC_Slot slot = GetSlot(slotId);
		if (slot && slot.GetEntity())
			entities.Insert(slot.GetEntity());
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(groupName));
		if (group)
		{
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
				entities.Insert(agent.GetControlledEntity());
		}
		SCR_BaseEditableEntityFilter selected = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.SELECTED);
		if (selected)
		{
			set<SCR_EditableEntityComponent> selection = new set<SCR_EditableEntityComponent>();
			selected.GetEntities(selection);
			foreach (SCR_EditableEntityComponent item : selection)
			{
				if (!entities.Contains(item.GetOwner()))
					entities.Insert(item.GetOwner());
			}
		}
		string result;
		foreach (IEntity entity : entities)
		{
			if (!entity)
				continue;
			result += string.Format("\nentity=%1 prefab=%2 flags=%3", entity, entity.GetPrefabData().GetPrefabName(), entity.GetFlags());
			GC_Slot entitySlot = GetSlotByEntity(entity);
			int entitySlotId = -1;
			if (entitySlot)
				entitySlotId = entitySlot.GetID();
			result += string.Format(" slot=%1", entitySlotId);
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(entity);
			SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
			AIControlComponent ai = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (editable)
			{
				SCR_HealEntitiesContextAction stockHealAction = new SCR_HealEntitiesContextAction();
				GC_HealSlotContextAction recovery = new GC_HealSlotContextAction();
				bool vanillaShown = stockHealAction.CanBeShown(editable, vector.Zero, 0);
				bool vanillaEnabled = stockHealAction.CanBePerformed(editable, vector.Zero, 0);
				bool recoveryShown = recovery.CanBeShown(editable, vector.Zero, 0);
				bool recoveryEnabled = recovery.CanBePerformed(editable, vector.Zero, 0);
				result += string.Format(" editableType=%1 editableFlags=%2 vanillaShown=%3 vanillaEnabled=%4 recoveryShown=%5 recoveryEnabled=%6",
					editable.GetEntityType(), editable.GetEntityFlags(), vanillaShown, vanillaEnabled, recoveryShown, recoveryEnabled);
			}
			if (damage)
				result += string.Format(" health=%1 damageState=%2 canHeal=%3", damage.GetHealthScaled(), damage.GetState(), damage.CanBeHealed());
			if (ai)
				result += string.Format(" aiActive=%1", ai.IsAIActivated());
			array<Managed> components = {};
			entity.FindComponents(GenericComponent, components);
			result += string.Format(" components=%1\n", components.Count());
			foreach (Managed component : components)
				result += component.Type().ToString() + "; ";
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestGameMasterRecovery(int slotId, string operation)
	{
		GC_Slot slot = GetSlot(slotId);
		if (GetManager() != this || !slot || !slot.GetEntity())
			return "UNAVAILABLE slot entity";
		array<Widget> widgets = {GetGame().GetWorkspace()};
		for (int i; i < widgets.Count(); i++)
		{
			Widget widget = widgets[i];
			SCR_ContextMenuActionsEditorUIComponent context = SCR_ContextMenuActionsEditorUIComponent.Cast(widget.FindHandler(SCR_ContextMenuActionsEditorUIComponent));
			if (context)
				return context.GC_TestSlotRecoveryMenu(slot.GetEntity(), operation);
			Widget child = widget.GetChildren();
			while (child)
			{
				widgets.Insert(child);
				child = child.GetSibling();
			}
		}
		return "UNAVAILABLE native Game Master context menu";
	}

	//------------------------------------------------------------------------------------------------
	//! Ends admin eligibility for this disposable session; never grants permissions.
	string GC_TestGameMasterRecoveryLoggedOut(int slotId)
	{
		PlayerController controller = GetGame().GetPlayerController();
		GC_Slot slot = GetSlot(slotId);
		if (!Replication.IsServer() || GetManager() != this || !controller || !slot || !slot.GetEntity())
			return "UNAVAILABLE local authority or slot entity";
		int playerId = controller.GetPlayerId();
		PlayerManager players = GetGame().GetPlayerManager();
		IEntity originalEntity = slot.GetEntity();
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(originalEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return "UNAVAILABLE character damage";
		float health = damage.GetHealthScaled();
		players.ClearPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		players.ClearPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR);
		GC_HealSlotContextAction action = new GC_HealSlotContextAction();
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(slot.GetEntity());
		bool visible = action.CanBeShown(editable, vector.Zero, 0);
		action.Perform(editable, vector.Zero);
		string outcome = ExecuteAdminSlotAction(playerId, slotId, slot.GetUUID(), GC_EAdminSlotAction.HEAL_OR_RESPAWN);
		return string.Format("connected=%1 admin=%2 visible=%3 outcome=%4 unchanged=%5",
			players.IsPlayerConnected(playerId), SCR_Global.IsAdmin(playerId), visible, outcome,
			originalEntity == slot.GetEntity() && health == damage.GetHealthScaled());
	}
}
#endif
