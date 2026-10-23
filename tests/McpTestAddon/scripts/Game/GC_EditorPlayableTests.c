#ifdef WORKBENCH
modded class SCR_AttributesEditorUIComponent
{
	//------------------------------------------------------------------------------------------------
	bool GC_TestShowLobbyCategory()
	{
		int index = m_CategoryConfigs.Find("{6A24B4A21C000003}Configs/Editor/AttributeCategories/GC_Lobby.conf");
		if (index < 0)
			return false;
		m_TabViewComponent.ShowTab(index, true, false);
		return true;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestShowPlayableCategory()
	{
		array<Widget> widgets = {GetGame().GetWorkspace()};
		for (int i = 0; i < widgets.Count(); i++)
		{
			Widget widget = widgets[i];
			SCR_AttributesEditorUIComponent attributes = SCR_AttributesEditorUIComponent.Cast(widget.FindHandler(SCR_AttributesEditorUIComponent));
			if (attributes && attributes.GC_TestShowLobbyCategory())
				return "SELECTED native Lobby tab";
			Widget child = widget.GetChildren();
			while (child)
			{
				widgets.Insert(child);
				child = child.GetSibling();
			}
		}
		return "UNAVAILABLE native Lobby category";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestOpenGameMaster()
	{
		return string.Format("native editor open=%1", SCR_EditorManagerEntity.OpenInstance());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPlacePlayableGroup()
	{
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_PlacingEditorComponent));
		if (!Replication.IsServer() || !placing || GetGame().GetWorld().FindEntityByName("GC_GMPlacedGroup"))
			return "UNAVAILABLE server placement component or fixture already exists";
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = "145 1 140";
		placing.GetOnPlaceEntityServer().Insert(GC_TestPlacedPlayableGroup);
		bool dispatched = placing.CreateEntity("{43C7A28EEB660FF8}Prefabs/Groups/OPFOR/Group_USSR_Team_GL.et", SCR_EditorPreviewParams.CreateParams(transform));
		if (!dispatched)
			placing.GetOnPlaceEntityServer().Remove(GC_TestPlacedPlayableGroup);
		return string.Format("native placement dispatched=%1; verify placement audit and GC_GMPlacedGroup", dispatched);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestPlacedPlayableGroup(int prefabId, SCR_EditableEntityComponent editable, int playerId)
	{
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_PlacingEditorComponent));
		if (placing)
			placing.GetOnPlaceEntityServer().Remove(GC_TestPlacedPlayableGroup);
		if (editable && SCR_AIGroup.Cast(editable.GetOwner()))
			editable.GetOwner().SetName("GC_GMPlacedGroup");
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestOpenPlayableProperties(string groupName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(groupName);
		SCR_AttributesManagerEditorComponent attributes = SCR_AttributesManagerEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!entity || !attributes)
			return "UNAVAILABLE entity or native attribute manager";
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return "UNAVAILABLE editable entity";
		attributes.SetCurrentCategory("{6A24B4A21C000003}Configs/Editor/AttributeCategories/GC_Lobby.conf");
		attributes.StartEditing(editable);
		return "DISPATCHED native properties; inspect dialog before applying";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestConfirmPlayableProperty(bool enabled)
	{
		SCR_AttributesManagerEditorComponent attributes = SCR_AttributesManagerEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!attributes)
			return "UNAVAILABLE native attribute manager";
		SCR_BaseEditorAttributeVar current;
		if (!attributes.GetAttributeVariable(GC_PlayableEditorAttribute, current) || !current)
			return "UNAVAILABLE Playable property not present in native dialog";
		if (!attributes.SetAttributeVariable(GC_PlayableEditorAttribute, SCR_BaseEditorAttributeVar.CreateBool(enabled)))
			return "FAIL property update";
		attributes.ConfirmEditing();
		return "DISPATCHED native property confirmation; verify roster and audit";
	}

	//------------------------------------------------------------------------------------------------
	//! Use a named, newly placed Game Master group. No permissions are granted here.
	//! Exercises the real property writer and verifies entity identity and idempotence.
	string GC_TestEditorPlayable(string groupName)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(groupName));
		SCR_AttributesManagerEditorComponent attributes = SCR_AttributesManagerEditorComponent.Cast(SCR_BaseEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!Replication.IsServer() || !group || !attributes || !attributes.GetManager())
			return "UNAVAILABLE server, named group and Game Master attribute manager required";
		int requesterId = attributes.GetManager().GetPlayerID();
		if (!SCR_Global.IsAdmin(requesterId) || m_Roster.ContainsGroup(group))
			return "UNAVAILABLE authenticated admin and unregistered group required";

		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(group.FindComponent(SCR_EditableEntityComponent));
		array<IEntity> characters;
		if (!editable || !GetEditorPlayableCharacters(group, characters))
			return "UNAVAILABLE editable populated group required";
		int before = GetSlots().Count();
		array<vector> positions = {};
		array<bool> activeAI = {};
		foreach (IEntity character : characters)
		{
			if (!GetPlayableCharacterGroup(character))
				return "UNAVAILABLE all soldiers must be eligible and unslotted";
			positions.Insert(character.GetOrigin());
			AIControlComponent ai = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			activeAI.Insert(ai.IsAIActivated());
		}

		string rejected = ExecuteEditorPlayable(-1, group, true);
		if (rejected != "rejected-not-logged-in" || GetSlots().Count() != before || m_Roster.ContainsGroup(group))
			return "FAIL unauthorized request changed registration";
		foreach (int index, IEntity character : characters)
		{
			AIControlComponent ai = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			if (GetSlotByEntity(character) || ai.IsAIActivated() != activeAI[index])
				return "FAIL unauthorized request changed soldier";
		}

		GC_PlayableEditorAttribute attribute = new GC_PlayableEditorAttribute();
		attribute.WriteVariable(editable, SCR_BaseEditorAttributeVar.CreateBool(true), attributes, -1);
		if (!m_Roster.ContainsGroup(group) || GetSlots().Count() != before + characters.Count())
			return "FAIL group or slot count";
		foreach (int index, IEntity character : characters)
		{
			GC_Slot slot = GetSlotByEntity(character);
			AIControlComponent ai = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			if (!slot || slot.GetGroup() != group || slot.GetEntity() != character || ai.IsAIActivated()
				|| character.GetOrigin() != positions[index])
				return "FAIL soldier identity, group, AI or position";
		}

		attribute.WriteVariable(editable, SCR_BaseEditorAttributeVar.CreateBool(true), attributes, -1);
		SCR_BaseEditorAttributeVar current = attribute.ReadVariable(editable, attributes);
		if (!current || !current.GetBool() || GetSlots().Count() != before + characters.Count())
			return "FAIL repeated enable or property readback";
		return "PASS authority rejection, editor-owner identity, live entity adoption and repeated enable; inspect connected lobby UI and audit separately";
	}

	//------------------------------------------------------------------------------------------------
	//! Only use on an unclaimed fixture created for this test.
	string GC_TestEditorPlayableDisable(string groupName)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(groupName));
		PlayerController controller = GetGame().GetPlayerController();
		array<IEntity> characters;
		if (!Replication.IsServer() || !controller || !GetEditorPlayableCharacters(group, characters))
			return "UNAVAILABLE populated fixture and local server player required";
		string outcome = ExecuteEditorPlayable(controller.GetPlayerId(), group, false);
		if (outcome != "applied")
			return outcome;
		foreach (IEntity character : characters)
		{
			AIControlComponent ai = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
			if (GetSlotByEntity(character) || !ai || !ai.IsAIActivated())
				return "FAIL slot retained or AI inactive";
		}
		return "PASS unclaimed soldiers retained and returned to AI; verify client rows removed";
	}
}
#endif
