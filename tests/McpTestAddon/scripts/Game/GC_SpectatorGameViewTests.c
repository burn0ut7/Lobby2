#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected IEntity m_GC_GameViewOriginalBody;
	protected IEntity m_GC_GameViewNormalBody;

	//------------------------------------------------------------------------------------------------
	string GC_TestGameView(string action)
	{
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!player || !player.GetControlledEntity())
			return "UNAVAILABLE local body";
		MenuManager menus = GetGame().GetMenuManager();
		if (action == "normal")
		{
			if (!Replication.IsServer() || m_GC_GameViewNormalBody)
				return "UNAVAILABLE authority or fixture already active";
			m_GC_GameViewOriginalBody = player.GetControlledEntity();
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			m_GC_GameViewOriginalBody.GetWorldTransform(parameters.Transform);
			m_GC_GameViewNormalBody = GetGame().SpawnEntityPrefab(Resource.Load("{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et"), GetGame().GetWorld(), parameters);
			if (!player.SetLobbyControlledEntity(m_GC_GameViewNormalBody, false))
				return "FAIL normal possession";
		}
		else if (action == "restore")
		{
			if (!m_GC_GameViewOriginalBody || !player.SetLobbyControlledEntity(m_GC_GameViewOriginalBody, true))
				return "FAIL spectator restoration";
			player.GC_ReturnToGameView();
			SCR_EntityHelper.DeleteEntityAndChildren(m_GC_GameViewNormalBody);
			m_GC_GameViewNormalBody = null;
			m_GC_GameViewOriginalBody = null;
		}
		else if (action == "lobby")
			menus.OpenMenu(ChimeraMenuPreset.GC_Lobby_SlottingMenu, 0, true);
		else if (action == "debrief")
			menus.OpenMenu(ChimeraMenuPreset.GC_Lobby_DebriefMenu, 0, true);
		else if (action == "inventory")
		{
			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.GetControlledEntity().FindComponent(SCR_InventoryStorageManagerComponent));
			if (!inventory)
				return "UNAVAILABLE inventory";
			inventory.Action_OpenInventory();
		}
		return player.GC_TestGameViewState();
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestGameViewState()
	{
		IEntity body = GetControlledEntity();
		CharacterControllerComponent controls = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		BaseRadioComponent radio = GC_LobbyRadio.FindRadio(body);
		MenuManager menus = GetGame().GetMenuManager();
		string menu = "none";
		if (menus.GetTopMenu())
			menu = menus.GetTopMenu().Type().ToString();
		string result = string.Format("phase=%1 state=%2 requested=%3 spectatorBody=%4 cameraExists=%5 cameraCurrent=%6 menu=%7 inventoryOpen=%8",
			GC_LobbyManager.GetManager().GetLobbyPhase(), GetLobbyPlayerState(), m_bSpectatorPresentationRequested,
			body.FindComponent(GC_LobbySpectatorBodyComponent) != null, m_SpectatorCamera != null,
			m_SpectatorCamera && GetGame().GetCameraManager().CurrentCamera() == m_SpectatorCamera,
			menu, menus.FindMenuByPreset(ChimeraMenuPreset.Inventory20Menu) != null);
		if (controls)
			result += string.Format(" movementBlocked=%1 weaponsBlocked=%2 viewBlocked=%3", controls.GetDisableMovementControls(), controls.GetDisableWeaponControls(), controls.GetDisableViewControls());
		result += string.Format(" radio=%1 powered=%2", radio != null, radio && radio.IsPowered());
		if (m_SpectatorCamera)
			result += " cameraPosition=" + m_SpectatorCamera.GetOrigin().ToString();
		return result + " " + GC_LobbyPlayerController.Cast(this).GC_TestSpectatorSystems();
	}
}
#endif
